//
//  sjf_granularDelay.h
//
//  Created by Simon Fay on 04/10/2023.
//

#ifndef sjf_granularDelay_h
#define sjf_granularDelay_h

#include <vector>
#include <JuceHeader.h>
#include "sjf_audioUtilities.h"
#include "sjf_interpolators.h"
#include "sjf_wavetables.h"
#include "../sjf_audio/sjf_audioUtilities.h"
#include "sjf_bitCrusher.h"

//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------

// ideas :
//      repeat same grain over and over again


class sjf_gdVoice
{
private:
    static constexpr int NCHANNELS = 2;
    size_t m_writePos = 0;
    bool m_reverseFlag = false;
    bool m_shouldPlayFlag = false;
    float m_delayTimeSamps = 11025;
    float m_transposition = 1;
    float m_grainSizeSamps = 11025;
    float m_transposedGrainSize = m_transposition * m_grainSizeSamps;
    int m_sampleCount = 0;
    
    
    std::array< std::array< float, NCHANNELS >, NCHANNELS > m_mixMatrix;
    
    static constexpr float m_windowSize = 1024;
    static constexpr sjf_hannArray< float, static_cast< int >( m_windowSize ) > m_win;
    
    std::array< float, NCHANNELS > m_samples;
    
    std::array< sjf_bitCrusher< float >, NCHANNELS > m_bitCrush;
    
public:
    sjf_gdVoice()
    {
        setCrossTalkLevels( 0.0 );
    }
    ~sjf_gdVoice(){}
    
    void triggerNewGrain( size_t writePos, bool rev, float dt, float transpos, float grainSize, float crosstalk, size_t delBufSize, int bitDepth, int srDivider )
    {
        if ( m_shouldPlayFlag )
            return; // not the neatest solution but if grain is already playing don't trigger this grain
        m_writePos = writePos;
        m_reverseFlag = rev;
        m_shouldPlayFlag = true;
        m_delayTimeSamps = dt > 0 ? dt : 1;
        m_transposition = transpos;
        m_grainSizeSamps = grainSize;
        m_transposedGrainSize = m_transposition * m_grainSizeSamps;
        
        // grain will play m_transposedGrainSize samples in length of m_grainSizeSamps
        // if m_transposedGrainSize > m_grainSizeSamps
        //      then we will run past the write pointer
        //      --> make sure our write pointer is always far enough in the past to allow all samples to be read
        if ( m_transposedGrainSize > m_grainSizeSamps )
        {
            auto dif = ( m_transposedGrainSize - m_grainSizeSamps ) + 1;
            m_writePos = fastMod4< size_t > ( ( m_writePos + delBufSize - dif ), delBufSize );
        }
        
        m_sampleCount = 0;
        setCrossTalkLevels( crosstalk );
        
        for ( auto & bc : m_bitCrush )
        {
            bc.setNBits( bitDepth );
            bc.setSRDivide( srDivider );
        }
    }
    
    void process( std::array< float, NCHANNELS >& outSamples, std::array< std::vector< float >, NCHANNELS >& delayBuffers )
    {
        
        auto grainPhase = static_cast< float >( m_sampleCount ) / static_cast< float >( m_grainSizeSamps );
        
        auto readPos = m_writePos - m_delayTimeSamps;
        
        if ( m_reverseFlag )
            readPos += m_transposedGrainSize - ( m_transposedGrainSize * grainPhase );
        else
            readPos += ( m_transposedGrainSize * grainPhase );
        
        auto delBufferSize = delayBuffers[ 0 ].size();
        readPos = fastMod4< float >( readPos, delBufferSize );
        auto win = m_win.getValue( m_windowSize * grainPhase );
        auto pos1 = static_cast< int >( readPos );
        auto mu = readPos - ( static_cast< float >( pos1 ) );
        auto pos0 = fastMod4< int >( pos1 - 1, static_cast< int >( delBufferSize ) );
        auto pos2 = fastMod4< int >( pos1 + 1, static_cast< int >( delBufferSize ) );
        auto pos3 = fastMod4< int >( pos2 + 1, static_cast< int >( delBufferSize ) );
        
        for ( auto c = 0; c < NCHANNELS; c++ )
            m_samples[ c ] =  m_bitCrush[ c ].process( sjf_interpolators::fourPointInterpolatePD( mu, delayBuffers[ c ][ pos0 ], delayBuffers[ c ][ pos1 ], delayBuffers[ c ][ pos2 ], delayBuffers[ c ][ pos3 ]) ) * win;
        
        for ( auto i = 0; i < NCHANNELS; i++ )
        {
            for ( auto j = 0; j < NCHANNELS; j++ )
            {
                outSamples[ i ] += m_samples[ j ] * m_mixMatrix[ i ][ j ];
            }
        }
        m_sampleCount++;
        m_shouldPlayFlag = ( m_sampleCount >= m_grainSizeSamps ) ? false : true;
    }
    
    bool getIsPlaying()
    {
        return m_shouldPlayFlag;
    }
    
private:
    void setCrossTalkLevels( float crosstalk )
    {
        for ( auto c = 0; c < NCHANNELS; c++ )
         {
             m_mixMatrix[ c ][ c ] =  0.5 + ( 0.5 * std::cos( crosstalk * M_PI ) );
             m_mixMatrix[ c ][ fastMod4( c + 1, NCHANNELS ) ] =  0.5 + ( 0.5 * std::cos( ( 1.0 - crosstalk ) * M_PI ) );
         }
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_gdVoice )
};



//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------

template < int NVOICES >
class sjf_granularDelay
{
public:
    sjf_granularDelay()
    {
#ifndef NDEBUG
        assert ( NVOICES % 2 == 0 ); // ensure that number of voices is a multiple of 2 for crossfading
#endif
        initialise( m_SR );
        
        
        m_gParams.m_wp = m_writePos;
        m_gParams.m_dt = m_delayTimeSamps;
        m_gParams.m_gs = m_deltaTimeSamps * 2.0;
        m_gParams.m_ct = m_crosstalk;
        m_gParams.m_tr = m_transposition;
        m_gParams.m_bd = m_bitDepth;
        m_gParams.m_srDiv = m_srDivider;
        m_gParams.m_rev = false;
        m_gParams.m_play = true;
    }
    
    ~sjf_granularDelay(){}
    
    void initialise( double sampleRate )
    {
        m_SR = sampleRate;
        
        for ( auto & buf : m_buffers )
        {
            buf.resize( m_SR * 10 );
            for ( auto & s : buf )
                s = 0;
        }
        
        m_sampleCount = 0;
        m_deltaTimeSamps = std::round( m_SR / m_rateHz );
        
        m_fbSmoother.reset( m_SR, 0.1 );
        m_fbSmoother.setCurrentAndTargetValue( 0 );
        m_drySmoother.reset( m_SR, 0.1 );
        m_drySmoother.setCurrentAndTargetValue( 0 );
        m_wetSmoother.reset( m_SR, 0.1 );
        m_wetSmoother.setCurrentAndTargetValue( 1 );
        
    }
    
    
    void process( juce::AudioBuffer<float>& buffer )
    {
        auto numOutChannels = buffer.getNumChannels();
        auto blockSize = buffer.getNumSamples();
        
        std::array< float, NCHANNELS > samples;

        float fbSmooth, drySmooth, wetSmooth, outSamp;
        auto delBufSize = m_buffers[ 0 ].size();
        
        bool activeVoices = false;
        for ( int indexThroughBuffer = 0; indexThroughBuffer < blockSize; indexThroughBuffer++ )
        {
            fbSmooth = m_fbSmoother.getNextValue();
            drySmooth = m_drySmoother.getNextValue();
            wetSmooth = m_wetSmoother.getNextValue();
            m_writePos = fastMod( m_writePos, delBufSize );
            for ( auto & s : samples )
                s = 0;
            
            if ( m_sampleCount >= m_deltaTimeSamps )
            {
                for ( auto v = 0; v < NVOICES; v++ )
                {
                    if ( !m_delays[ v ].getIsPlaying() ) // choose the first available voice
                    {
//                        vNum = v;
//                        auto dt = m_delayTimeSamps + ( m_delayTimeSamps * rand01() * m_delayTimeJitter );
//                        bool rev = rand01() < m_reverseChance ? true : false;
//                        auto pJit = ( rand01()* 2.0f - 1 ) * m_transpositionJitter;
//                        auto tr = std::pow( 2, m_transposition + pJit );
//                        bool shouldPlay = m_density >= rand01() ? true : false;
//                        auto grainSize = m_deltaTimeSamps * 2.0;
//                        auto wp = rand01() < m_repeatChance ? m_lastwritePos : m_writePos;
                        if ( rand01() >= m_repeatChance )
                            randomiseAllGrainParameters();
                        
                        if ( m_gParams.m_play )
                            m_delays[ v ].triggerNewGrain( m_gParams.m_wp, m_gParams.m_rev, m_gParams.m_dt, m_gParams.m_tr, m_gParams.m_gs, m_gParams.m_ct, delBufSize, m_gParams.m_bd, m_gParams.m_srDiv );
                        m_sampleCount = 0;
                        break;
                    }
                }
            }
            
            activeVoices = false;
            // run through each grain channel and add output to samples
            for ( auto &  v : m_delays )
            {
                if ( v.getIsPlaying() )
                {
                    activeVoices = true;
                    v.process( samples, m_buffers );
                }
            }

            for ( auto & buf : m_buffers )
                buf[ m_writePos ] = 0;
            for ( auto c = 0; c < NCHANNELS; c++ )
            {
                
                auto bufChan = fastMod4< int >( c, numOutChannels );
                auto inSamp = buffer.getSample( bufChan, indexThroughBuffer );
                m_buffers[ c ][ m_writePos ] += inSamp + ( samples[ c ] * fbSmooth );
                
                switch ( m_outMode )
                {
                    case outputModes::mix:
                        outSamp = ( samples[ c ] * wetSmooth ) + ( inSamp * drySmooth );
                        buffer.setSample( bufChan, indexThroughBuffer, outSamp );
                        break;
                    case outputModes::insert:
                        if( activeVoices )
                            buffer.setSample( bufChan, indexThroughBuffer, samples[ c ] );
                        break;
                    case outputModes::gate:
                        buffer.setSample( bufChan, indexThroughBuffer, samples[ c ] );
                        break;
                    default:
                        outSamp = ( samples[ c ] * wetSmooth ) + ( inSamp * drySmooth );
                        buffer.setSample( bufChan, indexThroughBuffer, outSamp );
                        break;
                }
                
            }

            m_writePos++;
            
            m_sampleCount++;
        }
    }
    
    
    void setRate( float rate )
    {
#ifndef NDEBUG
        assert ( rate > 0 );
#endif

        m_rateHz = rate; // / static_cast< float >( NVOICES );
        m_deltaTimeSamps = std::round( m_SR / m_rateHz );
    }
    
    void setFeedback( float fbPercentage )
    {
#ifndef NDEBUG
        assert ( fbPercentage >= 0 && fbPercentage <= 100 );
#endif
        m_fbSmoother.setTargetValue( fbPercentage * 0.01f );
    }
    
    void setCrossTalk( float ct )
    {
#ifndef NDEBUG
        assert ( ct >= 0 && ct <= 100 );
#endif
        m_crosstalk = ct * 0.01f;
    }
    
    void setDensity( float densityPercentage )
    {
#ifndef NDEBUG
        assert ( densityPercentage >= 0 && densityPercentage <= 100 );
#endif
        m_density = densityPercentage * 0.01f;
    }
    
    void setDelayTimeSamps( float dtSamps )
    {
        m_delayTimeSamps = dtSamps > 0 ? dtSamps : 1;
    }
    
    void setDelayTimeJitter( float dtJitPercentage )
    {
#ifndef NDEBUG
        assert ( dtJitPercentage >= 0 && dtJitPercentage <= 100 );
#endif
        m_delayTimeJitter = dtJitPercentage * 0.01f;
    }
    
    void setTransposition( float trSemitones )
    {
        m_transposition = trSemitones / 12.0f;
    }
    
    void setTranspositionJitter( float trJitPercentage )
    {
#ifndef NDEBUG
        assert ( trJitPercentage >= 0 && trJitPercentage <= 100 );
#endif
        m_transpositionJitter = trJitPercentage * 0.01f;
    }
    
    void setReverseChance( float revPercentage )
    {
#ifndef NDEBUG
        assert ( revPercentage >= 0 && revPercentage <= 100 );
#endif
        m_reverseChance = revPercentage * 0.01f;
    }
    
    
    void setRepeat( float repeatPercentage )
    {
#ifndef NDEBUG
        assert ( repeatPercentage >= 0 && repeatPercentage <= 100 );
#endif
        m_repeatChance = repeatPercentage * 0.01f;
    }
    
    void setBitDepth( int bitDepth )
    {
#ifndef NDEBUG
        assert ( bitDepth >= 0 );
#endif
        m_bitDepth = bitDepth;
    }
    
    void setSampleRateDivider( int srDivider )
    {
#ifndef NDEBUG
        assert ( srDivider >= 1 );
#endif
        m_srDivider = srDivider;
    }
    
    
    void setMix( float wetPercentage )
    {
#ifndef NDEBUG
        assert ( wetPercentage >= 0 && wetPercentage <= 100 );
#endif
        auto wet = wetPercentage * 0.01;
        wet = wet > 0.0f ? std::sqrt( wet ) : 0;
        auto dry = 1.0f - wet;
        dry = dry > 0.0f ? std::sqrt( dry ) : 0;
        
        m_drySmoother.setTargetValue( dry );
        m_wetSmoother.setTargetValue( wet );
    }
    

    
    void setJitterSync( bool jitterShouldBeSynced )
    {
        m_jitterSync = jitterShouldBeSynced;
    }
    
    void setSyncedDelayJitterValues( float divisionSamps, int nDivisions )
    {
#ifndef NDEBUG
        assert( divisionSamps > 0 );
        assert( nDivisions >= 1 );
#endif
        
        m_syncedJitterDivSamps = divisionSamps;
        m_syncedJitterDivMax = nDivisions;
    }
    
    void setOutputMode( int mode )
    {
        m_outMode = mode;
    }
    
    enum outputModes
    {
        mix, insert, gate
    };
    
private:
    
    struct granDelParameters
    {
        granDelParameters(){}
        ~granDelParameters(){}

        float m_wp, m_dt, m_gs, m_ct, m_tr;
        int m_bd, m_srDiv;
        bool m_rev, m_play;
    };
    
    
    void randomiseAllGrainParameters()
    {
        auto dt = m_delayTimeSamps;
        auto dtJ = ( ( 2.0 * rand01() ) - 1.0 ) * m_delayTimeJitter;
        if ( !m_jitterSync )
        {
            dt += ( m_delayTimeSamps * dtJ );
        }
        else
        {
            dtJ = std::round( dtJ * m_syncedJitterDivMax );
            dt += m_syncedJitterDivSamps * dtJ;
        }
        dt = ( dt < 0 ) ? ( -1.0 * dt ) : ( ( dt == 0 ) ? 1 : dt );
        bool rev = rand01() < m_reverseChance ? true : false;
        auto trJit = ( rand01()* 2.0f - 1 ) * m_transpositionJitter;
        auto tr = std::pow( 2, m_transposition + trJit );
        bool shouldPlay = m_density >= rand01() ? true : false;
        auto grainSize = m_deltaTimeSamps * 2.0;
//        auto wp = rand01() < m_repeatChance ? m_lastwritePos : m_writePos;
        
        m_gParams.m_wp = m_writePos;
        m_gParams.m_dt = dt;
        m_gParams.m_gs = grainSize;
        m_gParams.m_ct = m_crosstalk;
        m_gParams.m_tr = tr;
        m_gParams.m_bd = m_bitDepth;
        m_gParams.m_srDiv = m_srDivider;
        m_gParams.m_rev = rev;
        m_gParams.m_play = shouldPlay;

    }
    
    static constexpr int NCHANNELS = 2;
    
    granDelParameters m_gParams;
    
    std::array< sjf_gdVoice, NVOICES > m_delays;
    std::array< std::vector< float >, NCHANNELS > m_buffers;
    
    size_t m_writePos = 0;
    size_t m_lastwritePos = 0;
    float m_rateHz = 1; // num grains triggered per second
    float m_crosstalk = 0;
    float m_density = 1;
    float m_delayTimeSamps = 22050;
    float m_delayTimeJitter = 0;
    float m_reverseChance = 0.25;
    float m_repeatChance = 0;
    float m_transposition = 0;
    float m_transpositionJitter = 0;
    int m_bitDepth = 32;
    int m_srDivider = 1;
    int m_outMode = outputModes::mix;
    float m_SR = 44100;
    
    bool m_jitterSync = false;
    float m_syncedJitterDivSamps = m_delayTimeSamps / 8.0;
    float m_syncedJitterDivMax = 8;
    
    long long m_sampleCount = 0;
    long long m_deltaTimeSamps = std::round( m_SR / m_rateHz );
    
    juce::SmoothedValue< float > m_fbSmoother, m_drySmoother, m_wetSmoother;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_granularDelay )
};

#endif /* sjf_granularDelay_h */
