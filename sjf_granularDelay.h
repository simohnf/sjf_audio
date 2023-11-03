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
#include "sjf_biquadWrapper.h"
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------

struct sjf_granDelParameters
{
    sjf_granDelParameters(){}
    ~sjf_granDelParameters(){}

    float m_wp, m_dt, m_gs, m_ct, m_tr, m_amp;
    int m_bd, m_srDiv, m_interpolation;
    bool m_rev, m_play, m_crush;
};

//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------

class sjf_gdVoice
{
private:
    static constexpr int NCHANNELS = 2;
    float m_writePos = 0;
    bool m_reverseFlag = false;
    bool m_shouldPlayFlag = false;
    float m_delayTimeSamps = 11025;
    float m_transposition = 1;
    float m_grainSizeSamps = 11025;
    float m_transposedGrainSize = m_transposition * m_grainSizeSamps;
    int m_sampleCount = 0;
    float m_amp = 1;
    
    int m_interpType = sjf_interpolators::interpolatorTypes::pureData;
    
    bool m_bitCrushFlag = false;
    std::array< std::array< float, NCHANNELS >, NCHANNELS > m_mixMatrix;
    
    static constexpr float m_windowSize = 1024;
    static constexpr sjf_hannArray< float, static_cast< int >( m_windowSize ) > m_win;
    
    std::array< float, NCHANNELS > m_samples;
    
    std::array< sjf_bitCrusher< float >, NCHANNELS > m_bitCrush;
    
    std::array< sjf_interpolators::sjf_allpassInterpolator<float>, NCHANNELS > m_apInterps;
public:
    sjf_gdVoice()
    {
        setCrossTalkLevels( 0.0 );
    }
    ~sjf_gdVoice(){}
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void triggerNewGrain( sjf_granDelParameters& gParams, size_t delBufSize )
    {
        if ( m_shouldPlayFlag )
            return; // not the neatest solution but if grain is already playing don't trigger this grain
        m_writePos = gParams.m_wp;
        m_reverseFlag = gParams.m_rev;
        m_shouldPlayFlag = gParams.m_play;
        m_delayTimeSamps = std::fmax( std::abs( gParams.m_dt ), 1.0 );
        m_transposition = gParams.m_tr;
        m_grainSizeSamps = gParams.m_gs;
        m_interpType = gParams.m_interpolation;
        
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
        setCrossTalkLevels( gParams.m_ct );
        
        for ( auto & bc : m_bitCrush )
        {
            bc.setNBits( gParams.m_bd );
            bc.setSRDivide( gParams.m_srDiv );
            bc.resetCount();
        }
        m_bitCrushFlag = gParams.m_crush;
        
        for ( auto & ap : m_apInterps )
        {
            ap.reset();
        }
        
        m_amp = gParams.m_amp;
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void process( std::array< float, NCHANNELS >& outSamples, std::array< std::vector< float >, NCHANNELS >& delayBuffers )
    {
        
        auto grainPhase = static_cast< float >( m_sampleCount ) / static_cast< float >( m_grainSizeSamps );
        
        auto readPos = m_writePos - m_delayTimeSamps;
        readPos += m_reverseFlag ?  -1.0 * ( m_transposedGrainSize * grainPhase ) : ( m_transposedGrainSize * grainPhase );

        auto win = m_win.getValue( m_windowSize * grainPhase );
        
        writeToSamplesToInternalBuffer( readPos, delayBuffers );
        
        if ( m_bitCrushFlag )
            for ( auto c = 0; c < NCHANNELS; c++ )
                m_samples[ c ] = m_bitCrush[ c ].process( m_samples[ c ] );

        for ( auto & s : m_samples )
            s *= win;
        
        for ( auto i = 0; i < NCHANNELS; i++ )
            for ( auto j = 0; j < NCHANNELS; j++ )
                outSamples[ i ] += m_samples[ j ] * m_mixMatrix[ i ][ j ] * m_amp;
        
        m_sampleCount++;
        m_shouldPlayFlag = ( m_sampleCount >= m_grainSizeSamps ) ? false : true;
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    bool getIsPlaying()
    {
        return m_shouldPlayFlag;
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
private:
    void writeToSamplesToInternalBuffer( float readPos, std::array< std::vector< float >, NCHANNELS >& delayBuffers )
    {
        auto delBufferSize = delayBuffers[ 0 ].size();
        readPos = fastMod4< float >( readPos, delBufferSize );
        auto pos1 = static_cast< int >( readPos );
        auto mu = readPos - ( static_cast< float >( pos1 ) );
        auto pos2 = fastMod4< int >( pos1 + 1, static_cast< int >( delBufferSize ) );
//        { linear = 1, cubic, pureData, fourthOrder, godot, hermite, allpass };
        switch ( m_interpType )
        {
            case -1 :
            {
                for ( auto c = 0; c < NCHANNELS; c++ )
                    m_samples[ c ] = delayBuffers[ c ][ pos1 ];
                break;
            }
            case sjf_interpolators::interpolatorTypes::linear :
            {
                for ( auto c = 0; c < NCHANNELS; c++ )
                    m_samples[ c ] = sjf_interpolators::linearInterpolate( mu, delayBuffers[ c ][ pos1 ], delayBuffers[ c ][ pos2 ] );
                break;
            }
            case sjf_interpolators::interpolatorTypes::cubic :
            {
                auto pos0 = fastMod4< int >( pos1 - 1, static_cast< int >( delBufferSize ) );
                auto pos3 = fastMod4< int >( pos2 + 1, static_cast< int >( delBufferSize ) );
                for ( auto c = 0; c < NCHANNELS; c++ )
                    m_samples[ c ] = sjf_interpolators::cubicInterpolate( mu, delayBuffers[ c ][ pos0 ], delayBuffers[ c ][ pos1 ], delayBuffers[ c ][ pos2 ], delayBuffers[ c ][ pos3 ] );
                break;
            }
            case sjf_interpolators::interpolatorTypes::pureData :
            {
                auto pos0 = fastMod4< int >( pos1 - 1, static_cast< int >( delBufferSize ) );
                auto pos3 = fastMod4< int >( pos2 + 1, static_cast< int >( delBufferSize ) );
                for ( auto c = 0; c < NCHANNELS; c++ )
                    m_samples[ c ] = sjf_interpolators::fourPointInterpolatePD( mu, delayBuffers[ c ][ pos0 ], delayBuffers[ c ][ pos1 ], delayBuffers[ c ][ pos2 ], delayBuffers[ c ][ pos3 ] );
                break;
            }
            case sjf_interpolators::interpolatorTypes::fourthOrder :
            {
                auto pos0 = fastMod4< int >( pos1 - 1, static_cast< int >( delBufferSize ) );
                auto pos3 = fastMod4< int >( pos2 + 1, static_cast< int >( delBufferSize ) );
                for ( auto c = 0; c < NCHANNELS; c++ )
                    m_samples[ c ] = sjf_interpolators::fourPointFourthOrderOptimal( mu, delayBuffers[ c ][ pos0 ], delayBuffers[ c ][ pos1 ], delayBuffers[ c ][ pos2 ], delayBuffers[ c ][ pos3 ] );
                break;
            }
            case sjf_interpolators::interpolatorTypes::godot :
            {
                auto pos0 = fastMod4< int >( pos1 - 1, static_cast< int >( delBufferSize ) );
                auto pos3 = fastMod4< int >( pos2 + 1, static_cast< int >( delBufferSize ) );
                for ( auto c = 0; c < NCHANNELS; c++ )
                    m_samples[ c ] = sjf_interpolators::cubicInterpolateGodot( mu, delayBuffers[ c ][ pos0 ], delayBuffers[ c ][ pos1 ], delayBuffers[ c ][ pos2 ], delayBuffers[ c ][ pos3 ] );
                break;
            }
            case sjf_interpolators::interpolatorTypes::hermite :
            {
                auto pos0 = fastMod4< int >( pos1 - 1, static_cast< int >( delBufferSize ) );
                auto pos3 = fastMod4< int >( pos2 + 1, static_cast< int >( delBufferSize ) );
                for ( auto c = 0; c < NCHANNELS; c++ )
                    m_samples[ c ] = sjf_interpolators::cubicInterpolateHermite( mu, delayBuffers[ c ][ pos0 ], delayBuffers[ c ][ pos1 ], delayBuffers[ c ][ pos2 ], delayBuffers[ c ][ pos3 ] );
                break;
            }
            case sjf_interpolators::interpolatorTypes::allpass :
            {
                for ( auto c = 0; c < NCHANNELS; c++ )
                    m_samples[ c ] = m_apInterps[ c ].process( delayBuffers[ c ][ pos1 ], mu );
                break;
            }
                
            default:
            {
                for ( auto c = 0; c < NCHANNELS; c++ )
                    m_samples[ c ] = sjf_interpolators::linearInterpolate( mu, delayBuffers[ c ][ pos1 ], delayBuffers[ c ][ pos2 ] );
                break;
            }
        }
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void setCrossTalkLevels( float crosstalk )
    {
        for ( auto c = 0; c < NCHANNELS; c++ )
         {
             m_mixMatrix[ c ][ c ] =  0.5 + ( 0.5 * std::cos( crosstalk * M_PI ) );
             m_mixMatrix[ c ][ fastMod4( c + 1, NCHANNELS ) ] =  0.5 + ( 0.5 * std::cos( ( 1.0 - crosstalk ) * M_PI ) );
         }
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_gdVoice )
};

//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------

template < int NVOICES, int MAX_N_HARMONIES = 4 >
class sjf_granularDelay
{
public:
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    sjf_granularDelay()
    {
#ifndef NDEBUG
        assert ( NVOICES % 2 == 0 ); // ensure that number of voices is a multiple of 2 for crossfading
#endif
        
        srand (time(NULL));
        
        initialise( m_SR );
        setGrainParams( m_gParams, m_writePos, m_delayTimeSamps, m_deltaTimeSamps * 2.0, m_crosstalk, m_transposition, m_bitDepth, m_srDivider, false, true, m_shouldCrushBits, m_interpType, 1.0 );
//        setGrainParams( m_defaultGParams, m_writePos, 0, m_deltaTimeSamps * 2.0, 0, 1, 16, 1, false, true, false, -1 );
        m_harmParams[ 0 ].setParams( true, 0 );
        m_dryGainInsertEnv.reset();
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    ~sjf_granularDelay(){}
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
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
        
        m_outLevelSmoother.reset( m_SR, 0.1 );
        m_outLevelSmoother.setCurrentAndTargetValue( 1 );
        
        for ( auto & f : m_filter )
        {
            f.clear();
            f.initialise( m_SR );
        }
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void process( juce::AudioBuffer<float>& buffer )
    {
        auto numOutChannels = buffer.getNumChannels();
        auto blockSize = buffer.getNumSamples();
        
        std::array< float, NCHANNELS > samples;

        float fbSmooth, drySmooth, wetSmooth, outSmooth;
        auto delBufSize = m_buffers[ 0 ].size();
        
        auto newTrigger = false;
        
        for ( int indexThroughBuffer = 0; indexThroughBuffer < blockSize; indexThroughBuffer++ )
        {
            fbSmooth = m_fbSmoother.getNextValue();
            drySmooth = m_drySmoother.getNextValue();
            wetSmooth = m_wetSmoother.getNextValue();
            outSmooth = m_outLevelSmoother.getNextValue();
            m_writePos = fastMod( m_writePos, delBufSize );
            for ( auto & s : samples )
                s = 0;
            newTrigger = false;
            if ( m_sampleCount >= m_deltaTimeSamps )
            {
                m_deltaTimeSamps = std::round( m_SR / m_rateHz );
                newTrigger = true;
                if ( rand01() >= m_repeatChance )
                    randomiseAllGrainParameters();
                if ( m_gParams.m_play )
                    triggerNewGrains( delBufSize );
                else if( m_outMode == outputModes::insert )
                    m_dryGainInsertEnv.triggerNewGrain( m_gParams.m_gs );
                m_sampleCount = 0;
            }
            
            // run through each grain channel and add output to samples
            for ( auto &  v : m_delays )
                if ( v.getIsPlaying() )
                    v.process( samples, m_buffers );
            
            for ( auto & buf : m_buffers )
                buf[ m_writePos ] = 0;
            auto dryInsertEnv = m_dryGainInsertEnv.getValue();
            if ( m_filterFlag )
            {
                for ( auto c = 0; c < NCHANNELS; c++ )
                    samples[ c ] = m_filter[ c ].filterInput( samples[ c ] );
            }
            for ( auto c = 0; c < NCHANNELS; c++ )
            {
                auto bufChan = fastMod4< int >( c, numOutChannels );
                auto inSamp = buffer.getSample( bufChan, indexThroughBuffer );
                if ( m_shouldControlFB )
                    m_buffers[ c ][ m_writePos ] += inSamp + ( juce::dsp::FastMathApproximations::tanh( samples[ c ] ) * fbSmooth );
                else
                    m_buffers[ c ][ m_writePos ] += inSamp + ( samples[ c ] * fbSmooth );
                
                writeOutputToBuffer( buffer, indexThroughBuffer, bufChan, samples[ c ], inSamp, wetSmooth, drySmooth, dryInsertEnv, outSmooth );
            }
            m_writePos++;
            m_sampleCount++;
        }
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void setRate( float rate )
    {
#ifndef NDEBUG
        assert ( rate > 0 );
#endif
        m_rateHz = rate; // / static_cast< float >( NVOICES );
        
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void setFeedback( float fbPercentage )
    {
#ifndef NDEBUG
        assert ( fbPercentage >= 0 && fbPercentage <= 100 );
#endif
        m_fbSmoother.setTargetValue( fbPercentage * 0.01f );
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void setFeedbackControl( bool shouldControlFeedback )
    {
        m_shouldControlFB = shouldControlFeedback;
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void setCrossTalk( float ct )
    {
#ifndef NDEBUG
        assert ( ct >= 0 && ct <= 100 );
#endif
        m_crosstalk = ct * 0.01f;
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void setDensity( float densityPercentage )
    {
#ifndef NDEBUG
        assert ( densityPercentage >= 0 && densityPercentage <= 100 );
#endif
        m_density = densityPercentage * 0.01f;
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void setDelayTimeSamps( float dtSamps )
    {
        m_delayTimeSamps = dtSamps > 0 ? dtSamps : 1;
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void setDelayTimeJitter( float dtJitPercentage )
    {
#ifndef NDEBUG
        assert ( dtJitPercentage >= 0 && dtJitPercentage <= 100 );
#endif
        m_delayTimeJitter = dtJitPercentage * 0.01f;
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void setHarmony( int harmNum, bool isActive, float transpositionSemitones )
    {
        m_harmParams[ harmNum + 1 ].setParams( isActive, transpositionSemitones / 12.0f );
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void setTransposition( float trSemitones )
    {
        m_transposition = trSemitones / 12.0f;
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void setTranspositionJitter( float trJitPercentage )
    {
#ifndef NDEBUG
        assert ( trJitPercentage >= 0 && trJitPercentage <= 100 );
#endif
        m_transpositionJitter = trJitPercentage * 0.01f;
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void setReverseChance( float revPercentage )
    {
#ifndef NDEBUG
        assert ( revPercentage >= 0 && revPercentage <= 100 );
#endif
        m_reverseChance = revPercentage * 0.01f;
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void setRepeat( float repeatPercentage )
    {
#ifndef NDEBUG
        assert ( repeatPercentage >= 0 && repeatPercentage <= 100 );
#endif
        m_repeatChance = repeatPercentage * 0.01f;
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void setBitDepth( int bitDepth )
    {
#ifndef NDEBUG
        assert ( bitDepth >= 0 );
#endif
        m_bitDepth = bitDepth;
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void setSampleRateDivider( int srDivider )
    {
#ifndef NDEBUG
        assert ( srDivider >= 1 );
#endif
        m_srDivider = srDivider;
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void setBitCrushOn( bool shouldCrushBits )
    {
        m_shouldCrushBits = shouldCrushBits;
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
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
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void setJitterSync( bool jitterShouldBeSynced )
    {
        m_jitterSync = jitterShouldBeSynced;
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void setSyncedDelayJitterValues( float divisionSamps, int nDivisions )
    {
#ifndef NDEBUG
        assert( divisionSamps > 0 );
        assert( nDivisions >= 1 );
#endif
        
        m_syncedJitterDivSamps = divisionSamps;
        m_syncedJitterDivMax = nDivisions;
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void setOutputMode( int mode )
    {
        if ( m_outMode != mode )
            m_dryGainInsertEnv.reset();
        m_outMode = mode;
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    enum outputModes
    {
        mix, insert, gate
    };
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void setInterpolationType( int interpolationType )
    {
        m_interpType = interpolationType;
    }
    
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void setFilter( float freq, float q, int type, bool trueIfFirstOrder, bool filterIsActive )
    {
        for ( auto & f : m_filter )
        {
            f.setOrder( trueIfFirstOrder );
            f.setFrequency( freq );
            f.setQFactor( q );
            f.setFilterType( type );
        }
        m_filterFlag = filterIsActive;
    }
    
    
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void setOutputLevel( float outLevelDB )
    {
        auto gain = std::pow( 10.0, outLevelDB / 20.0 );
        m_outLevelSmoother.setTargetValue( gain );
    }
private:
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    class insertDryGrainEnv
    {
    public:
        insertDryGrainEnv(){}
        ~insertDryGrainEnv(){}
        //-----------------------------------------------------------------------------------
        //-----------------------------------------------------------------------------------
        //-----------------------------------------------------------------------------------
        void triggerNewGrain( int grainSizeSamps )
        {
            m_lastTriggeredGrain = !m_lastTriggeredGrain;
            auto window = m_lastTriggeredGrain ? 1 : 0;
            m_count[ window ] = 0;
            m_gLengthSamps[ window ] = grainSizeSamps;
            m_shouldRun[ window ] = true;
        }
        //-----------------------------------------------------------------------------------
        //-----------------------------------------------------------------------------------
        //-----------------------------------------------------------------------------------
        float getValue()
        {
            auto outVal = 0.0;
            for ( auto i = 0; i < NWINDOWS; i++ )
            {
                if ( m_shouldRun[ i ] && m_count[ i ] < m_gLengthSamps[ i ] )
                {
                    outVal += m_win.getValue( m_windowSize * m_count[ i ] / m_gLengthSamps[ i ] );
                    m_count[ i ] += 1;
                }
                else
                    m_shouldRun[ i ] = false;
            }
            return outVal;
        }
        //-----------------------------------------------------------------------------------
        //-----------------------------------------------------------------------------------
        //-----------------------------------------------------------------------------------
        void reset()
        {
            m_lastTriggeredGrain = false;
            m_count = { 0, 0 };
            m_gLengthSamps = { 22050, 22050 };
            m_shouldRun = { false, false };
        }
        //-----------------------------------------------------------------------------------
        //-----------------------------------------------------------------------------------
        //-----------------------------------------------------------------------------------
    private:
        static constexpr int NWINDOWS =  2;
        static constexpr float m_windowSize = 1024;
        static constexpr sjf_hannArray< float, static_cast< int >( m_windowSize ) > m_win;
        
        bool m_lastTriggeredGrain = false;
        std::array< float, NWINDOWS > m_count{ 0, 0 };
        std::array< float, NWINDOWS > m_gLengthSamps{ 22050, 22050 };
        std::array< bool, NWINDOWS > m_shouldRun{ false, false };
    };
    
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    struct harmonyParams
    {
    public:
        harmonyParams(){}
        ~harmonyParams(){}
        
        void setParams( bool isActive, float trans )
        {
            m_active = isActive;
            m_transposition = trans;
        }
        
        bool isActive()
        {
            return m_active;
        }
        
        float getTransposition()
        {
            return m_transposition;
        }
    private:
        bool m_active = false;
        float m_transposition = 0;
    };
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void setGrainParams( sjf_granDelParameters& param, float wp, float dt, float gs, float ct, float tr, int bd, int srDiv, bool rev, bool play, bool crush, int interp, float amp )
    {
        param.m_wp = wp;
        param.m_dt = dt;
        param.m_gs = gs;
        param.m_ct = ct;
        param.m_tr = tr;
        param.m_bd = bd;
        param.m_srDiv = srDiv;
        param.m_rev = rev;
        param.m_play = play;
        param.m_crush = crush;
        param.m_interpolation = interp;
        param.m_amp = amp;
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void randomiseAllGrainParameters()
    {
        auto dt = m_delayTimeSamps;
        auto dtJ = ( ( 2.0 * rand01() ) - 1.0 ) * m_delayTimeJitter;
        if ( !m_jitterSync )
            dt += ( m_delayTimeSamps * dtJ );
        else if ( m_delayTimeJitter >= rand01() )
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
        auto vCount = 0;
        for ( auto & h : m_harmParams )
            vCount += h.isActive() ? 1 : 0;
        auto amp = std::sqrt( 1.0 / vCount );
        setGrainParams( m_gParams, m_writePos, dt, grainSize, m_crosstalk, tr, m_bitDepth, m_srDivider, rev, shouldPlay, m_shouldCrushBits, m_interpType, amp );
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void triggerNewGrains( size_t delBufSize )
    {
        float baseTransposition = m_gParams.m_tr;
        for ( auto h = 0; h < MAX_N_HARMONIES; h++ )
        {
            if ( m_harmParams[ h ].isActive() )
            {
                m_gParams.m_tr = baseTransposition * std::pow( 2.0, m_harmParams[ h ].getTransposition() );
                for ( auto v = 0; v < NVOICES; v++ )
                {
                    if ( !m_delays[ v ].getIsPlaying() ) // choose the first available voice
                    {
                        m_delays[ v ].triggerNewGrain( m_gParams, delBufSize );
                        break;
                    }
                }
            }
        }
        m_gParams.m_tr = baseTransposition;
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void writeOutputToBuffer( juce::AudioBuffer<float>& buffer, int indexThroughBuffer, int bufChan, float wetSample, float inSamp, float wetSmooth, float drySmooth, float dryInsertEnv, float outSmooth  )
    {
        switch ( m_outMode )
        {
            case outputModes::mix:
            {
                buffer.setSample( bufChan, indexThroughBuffer, ( ( wetSample * wetSmooth ) + ( inSamp * drySmooth ) ) * outSmooth);
                break;
            }
            case outputModes::insert:
            {
                buffer.setSample( bufChan, indexThroughBuffer, (wetSample + ( inSamp * dryInsertEnv ))*outSmooth );
                break;
            }
            case outputModes::gate:
            {
                buffer.setSample( bufChan, indexThroughBuffer, wetSample * outSmooth);
                break;
            }
            default:
            {
                buffer.setSample( bufChan, indexThroughBuffer, (( wetSample * wetSmooth ) + ( inSamp * drySmooth ))*outSmooth );
                break;
            }
        }
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    
    
    static constexpr int NCHANNELS = 2;
    
    sjf_granDelParameters m_gParams;//, m_defaultGParams;
    
    std::array< sjf_gdVoice, NVOICES > m_delays;
    std::array< std::vector< float >, NCHANNELS > m_buffers;
    
    size_t m_writePos = 0;
    float m_rateHz = 1; // num grains triggered per second
    float m_crosstalk = 0;
    float m_density = 1;
    float m_delayTimeSamps = 22050;
    float m_delayTimeJitter = 0;
    
    bool m_shouldControlFB = false;
    float m_reverseChance = 0.25;
    float m_repeatChance = 0;
    
    std::array< harmonyParams, MAX_N_HARMONIES + 1 > m_harmParams;
    float m_transposition = 0;
    float m_transpositionJitter = 0;
    
    bool m_shouldCrushBits = false;
    int m_bitDepth = 32;
    int m_srDivider = 1;
    int m_outMode = outputModes::mix;
    float m_SR = 44100;
    
    bool m_jitterSync = false;
    float m_syncedJitterDivSamps = m_delayTimeSamps / 8.0;
    float m_syncedJitterDivMax = 8;
    
    long long m_sampleCount = 0;
    long long m_deltaTimeSamps = std::round( m_SR / m_rateHz );
    
    int m_interpType = sjf_interpolators::interpolatorTypes::pureData;
    int m_insertStateCount = 0;
    
    juce::SmoothedValue< float > m_fbSmoother, m_drySmoother, m_wetSmoother, m_outLevelSmoother;
    
    insertDryGrainEnv m_dryGainInsertEnv;
    
    std::array< sjf_biquadWrapper< float >, NCHANNELS > m_filter;
    bool m_filterFlag = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_granularDelay )
};

#endif /* sjf_granularDelay_h */
