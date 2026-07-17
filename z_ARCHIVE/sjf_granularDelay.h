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
#include "sjf_lpf.h"
#include "sjf_ringMod.h"
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
template < typename T >
struct sjf_granDelParameters
{
    sjf_granDelParameters(){}
    ~sjf_granDelParameters(){}

    T m_wp, m_dt, m_gs, m_ct, m_tr, m_amp;
    
    T m_rmF, m_rmMix, m_rmSpread;
    bool m_rm;
    
    bool m_crush;
    int m_bd, m_srDiv;
    
    int m_interpolation;
    bool m_rev, m_play;
    
    T m_filterF, m_filterQ;
    bool m_filterActive;
    int m_filterType;
};

//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
template < typename T >
class sjf_gdVoice
{
private:
    static constexpr int NCHANNELS = 2;
    T m_writePos = 0;
    bool m_reverseFlag = false;
    bool m_shouldPlayFlag = false;
    T m_delayTimeSamps = 11025;
    T m_transposition = 1;
    T m_grainSizeSamps = 11025;
    T m_transposedGrainSize = m_transposition * m_grainSizeSamps;
    int m_sampleCount = 0;
    T m_amp = 1;
    
    int m_interpType = sjf_interpolators::interpolatorTypes::pureData;
    
    bool m_bitCrushFlag = false;
    std::array< std::array< T, NCHANNELS >, NCHANNELS > m_mixMatrix;
    
    static constexpr T m_windowSize = 1024;
    static constexpr sjf_hannArray< T, static_cast< int >( m_windowSize ) > m_win;
    
    std::array< T, NCHANNELS > m_samples;
    
    std::array< sjf_bitCrusher< T >, NCHANNELS > m_bitCrush;
    
    std::array< sjf_biquadWrapper< T >, NCHANNELS > m_filter;
    bool m_filterFlag = false;
    
    std::array< sjf_ringMod< T >, NCHANNELS > m_ringMod;
    T m_rmDry = 0, m_rmWet = 1;
    bool m_rmFlag = false;
public:
    sjf_gdVoice()
    {
        setCrossTalkLevels( 0.0 );
    }
    ~sjf_gdVoice(){}
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void initialise( T sampleRate )
    {
        for ( auto & f : m_filter )
        {
            f.clear();
            f.initialise( sampleRate );
        }
        
        for ( auto & rm : m_ringMod )
            rm.initialise( sampleRate );
    }
    void triggerNewGrain( sjf_granDelParameters< T >& gParams, size_t delBufSize )
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
        
        m_amp = gParams.m_amp;
        
        for ( auto & f : m_filter )
        {
            f.setParameters( gParams.m_filterF, gParams.m_filterQ, gParams.m_filterType, false );
            f.clear();
        }
        m_filterFlag = gParams.m_filterActive;
        
        m_ringMod[ 0 ].setModFreq( gParams.m_rmF * std::pow( 2.0, gParams.m_rmSpread ) );
        m_ringMod[ 1 ].setModFreq( gParams.m_rmF * std::pow( 2.0, -1 * gParams.m_rmSpread ) );
        m_rmDry = std::sqrt( 1.0 - gParams.m_rmMix);
        m_rmWet = std::sqrt( gParams.m_rmMix);
        m_rmFlag = gParams.m_rm;
        
        for ( auto & rm : m_ringMod )
            rm.setInterpolationType( m_interpType );
        
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void process( std::array< T, NCHANNELS >& outSamples, std::array< std::vector< T >, NCHANNELS >& delayBuffers )
    {
        
        auto grainPhase = static_cast< T >( m_sampleCount ) / static_cast< T >( m_grainSizeSamps );
        
        auto readPos = m_writePos - m_delayTimeSamps;
        readPos += m_reverseFlag ?  -1.0 * ( m_transposedGrainSize * grainPhase ) : ( m_transposedGrainSize * grainPhase );

        auto win = m_win.getValue( m_windowSize * grainPhase );
        
        writeToSamplesToInternalBuffer( readPos, delayBuffers );
        
        if ( m_bitCrushFlag )
            for ( auto c = 0; c < NCHANNELS; c++ )
                m_samples[ c ] = m_bitCrush[ c ].process( m_samples[ c ] );
        if ( m_rmFlag )
            for ( auto c = 0; c < NCHANNELS; c++ )
                m_samples[ c ] = ( m_ringMod[ c ].process( m_samples[ c ] ) * m_rmWet ) + ( m_samples[ c ] * m_rmDry );
        if ( m_filterFlag )
            for ( auto c = 0; c < NCHANNELS; c++ )
                m_samples[ c ] = m_filter[ c ].filterInput( m_samples[ c ] );
        
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
    void writeToSamplesToInternalBuffer( T readPos, std::array< std::vector< T >, NCHANNELS >& delayBuffers )
    {
        auto delBufferSize = delayBuffers[ 0 ].size();
        readPos = fastMod4< T >( readPos, delBufferSize );
        auto pos1 = static_cast< int >( readPos );
        auto mu = readPos - ( static_cast< T >( pos1 ) );
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
    void setCrossTalkLevels( T crosstalk )
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

template < typename T, int NVOICES, int MAX_N_HARMONIES = 4 >
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
        setGrainParams( m_gParams, m_writePos, m_delayTimeSamps, m_deltaTimeSamps * 2.0, m_crosstalk, m_transposition, m_bitDepth, m_srDivider, false, true, m_shouldCrushBits, m_interpType, 1.0, m_filterF, m_filterQ, m_filterType, m_filterFlag, m_rmFreq, m_rmSpread, m_rmMix, m_rmFlag );
        m_harmParams[ 0 ].setParams( true, 0 );
        m_dryGainInsertEnv.reset();
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    ~sjf_granularDelay(){}
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void initialise( T sampleRate )
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
        
        for ( auto & g : m_delays )
            g.initialise( m_SR );
        
        for ( auto & f : m_dcBlock )
            f.setCoefficient( calculateLPFCoefficient< T >( 15, m_SR ) );
        
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void process( juce::AudioBuffer<T>& buffer )
    {
        auto numOutChannels = buffer.getNumChannels();
        auto blockSize = buffer.getNumSamples();
        
        std::array< T, NCHANNELS > outSamples;

        T fbSmooth, drySmooth, wetSmooth, outSmooth, inSamp;
        auto delBufSize = m_buffers[ 0 ].size();
        int bufChan = 0;
        for ( int indexThroughBuffer = 0; indexThroughBuffer < blockSize; indexThroughBuffer++ )
        {
            fbSmooth = m_fbSmoother.getNextValue();
            drySmooth = m_drySmoother.getNextValue();
            wetSmooth = m_wetSmoother.getNextValue();
            outSmooth = m_outLevelSmoother.getNextValue();
            m_writePos = fastMod( m_writePos, delBufSize );
            for ( auto & s : outSamples )
                s = 0;
            if ( m_sampleCount >= m_deltaTimeSamps )
            {
                m_deltaTimeSamps = std::round( m_SR / m_rateHz );
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
                    v.process( outSamples, m_buffers );
            for ( auto & buf : m_buffers )
                buf[ m_writePos ] = 0;
            auto dryInsertEnv = m_dryGainInsertEnv.getValue();

            for ( auto c = 0; c < NCHANNELS; c++ )
            {
                bufChan = fastMod4< int >( c, numOutChannels );
                inSamp = buffer.getSample( bufChan, indexThroughBuffer );
                m_buffers[ c ][ m_writePos ] += inSamp;
                m_buffers[ c ][ m_writePos ] += m_shouldControlFB ? ( juce::dsp::FastMathApproximations::tanh( outSamples[ c ] ) * fbSmooth ) : ( outSamples[ c ] * fbSmooth );
                outSamples[ c ] = calculateOutputSample( outSamples[ c ], inSamp, wetSmooth, drySmooth, dryInsertEnv, outSmooth );
                buffer.setSample( bufChan, indexThroughBuffer, outSamples[ c ] );
                // apply dcBlock before next sample
                for ( auto c = 0; c < NCHANNELS ; c++ )
                    m_dcBlock[ c ].filterInPlaceHP( m_buffers[ c ][ m_writePos ] );
            }
            m_writePos++;
            m_sampleCount++;
        }
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void setRate( T rate )
    {
#ifndef NDEBUG
        assert ( rate > 0 );
#endif
        m_rateHz = rate; // / static_cast< T >( NVOICES );
        
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void setFeedback( T fbPercentage )
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
    void setCrossTalk( T ct )
    {
#ifndef NDEBUG
        assert ( ct >= 0 && ct <= 100 );
#endif
        m_crosstalk = ct * 0.01f;
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void setDensity( T densityPercentage )
    {
#ifndef NDEBUG
        assert ( densityPercentage >= 0 && densityPercentage <= 100 );
#endif
        m_density = densityPercentage * 0.01f;
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void setDelayTimeSamps( T dtSamps )
    {
        m_delayTimeSamps = dtSamps > 0 ? dtSamps : 1;
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void setDelayTimeJitter( T dtJitPercentage )
    {
#ifndef NDEBUG
        assert ( dtJitPercentage >= 0 && dtJitPercentage <= 100 );
#endif
        m_delayTimeJitter = dtJitPercentage * 0.01f;
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void setHarmony( int harmNum, bool isActive, T transpositionSemitones )
    {
        m_harmParams[ harmNum + 1 ].setParams( isActive, transpositionSemitones / 12.0f );
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void setTransposition( T trSemitones )
    {
        m_transposition = trSemitones / 12.0f;
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void setTranspositionJitter( T trJitPercentage )
    {
#ifndef NDEBUG
        assert ( trJitPercentage >= 0 && trJitPercentage <= 100 );
#endif
        m_transpositionJitter = trJitPercentage * 0.01f;
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void setReverseChance( T revPercentage )
    {
#ifndef NDEBUG
        assert ( revPercentage >= 0 && revPercentage <= 100 );
#endif
        m_reverseChance = revPercentage * 0.01f;
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void setRepeat( T repeatPercentage )
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
    void setMix( T wetPercentage )
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
    void setSyncedDelayJitterValues( T divisionSamps, int nDivisions )
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
    void setFilter( T freq, T q, int type, bool filterIsActive )
    {
        m_filterF = freq;
        m_filterQ = q;
        m_filterType = type;
        m_filterFlag = filterIsActive;
    }
    void setFilterJitter( T filtJit )
    {
#ifndef NDEBUG
        assert ( filtJit >= 0 && filtJit <= 100 );
#endif
        m_filterJit = filtJit * 0.01;
    }
    
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void setRingMod( T f, T spread, T mix, bool ringmodIsActive )
    {
#ifndef NDEBUG
        assert( f > 0 );
        assert( spread >= 0 && spread <= 100 );
        assert( mix >= 0 && mix <= 100 );
#endif
        m_rmFreq = f;
        m_rmSpread = spread * 0.01;
        m_rmMix = mix * 0.01;
        m_rmFlag = ringmodIsActive;
    }
    
    void setRingModJitter( T ringModJitter )
    {
#ifndef NDEBUG
        assert ( ringModJitter >= 0 && ringModJitter <= 100 );
#endif
        m_rmJit = ringModJitter * 0.01;
    }
    
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    
    void setOutputLevel( T outLevelDB )
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
        T getValue()
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
        static constexpr T m_windowSize = 1024;
        static constexpr sjf_hannArray< T, static_cast< int >( m_windowSize ) > m_win;
        
        bool m_lastTriggeredGrain = false;
        std::array< T, NWINDOWS > m_count{ 0, 0 };
        std::array< T, NWINDOWS > m_gLengthSamps{ 22050, 22050 };
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
        
        void setParams( bool isActive, T trans )
        {
            m_active = isActive;
            m_transposition = trans;
        }
        
        bool isActive()
        {
            return m_active;
        }
        
        T getTransposition()
        {
            return m_transposition;
        }
    private:
        bool m_active = false;
        T m_transposition = 0;
    };
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void setGrainParams( sjf_granDelParameters< T >& param, T wp, T dt, T gs, T ct, T tr, int bd, int srDiv, bool rev, bool play, bool crush, int interp, T amp, T filterF, T filterQ, int filterType, bool filterActive, T ringModF, T ringModSpread, T ringModMix, bool ringModActive )
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
        param.m_filterF = filterF;
        param.m_filterQ = m_filterQ;
        param.m_filterType = filterType;
        param.m_filterActive = filterActive;
        
        param.m_rm = ringModActive;
        param.m_rmF = ringModF;
        param.m_rmSpread = ringModSpread;
        param.m_rmMix = ringModMix;
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
        auto trJit = ( rand01()* 2.0f - 1 ) * m_transpositionJitter / 12.0;
        auto tr = std::pow( 2.0, m_transposition + trJit );
        bool shouldPlay = m_density >= rand01() ? true : false;
        auto grainSize = m_deltaTimeSamps * 2.0;
        auto vCount = 0;
        for ( auto & h : m_harmParams )
            vCount += h.isActive() ? 1 : 0;
        auto amp = std::sqrt( 1.0 / vCount );
        
        auto filtF = m_filterF;
        auto fJit = m_filterJit*( ( rand01() * 4.0 ) - 2.0 ); // max two octave variation
        fJit = std::pow( 2.0, fJit );
        filtF += fJit * filtF;
        filtF = (filtF < 20) ? 20 : ( (filtF > 20000) ? 20000 : filtF);
        
        auto r = ( rand01() * 2.0 ) - 1.0;
        r *= m_rmJit;
        auto rmF = m_rmFreq * std::pow( 2.0, r );
        
        setGrainParams( m_gParams, m_writePos, dt, grainSize, m_crosstalk, tr, m_bitDepth, m_srDivider, rev, shouldPlay, m_shouldCrushBits, m_interpType, amp,  filtF, m_filterQ, m_filterType, m_filterFlag, rmF, m_rmSpread, m_rmMix, m_rmFlag );
//        setGrainParams( m_gParams, m_writePos, dt, grainSize, m_crosstalk, tr, m_bitDepth, m_srDivider, rev, shouldPlay, m_shouldCrushBits, m_interpType, amp,  filtF, m_filterQ, m_filterType, m_filterFlag );
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void triggerNewGrains( size_t delBufSize )
    {
        T baseTransposition = m_gParams.m_tr;
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
    void writeOutputToBuffer( juce::AudioBuffer<T>& buffer, int indexThroughBuffer, int bufChan, T wetSample, T inSamp, T wetSmooth, T drySmooth, T dryInsertEnv, T outSmooth  )
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
    T calculateOutputSample( T wetSample, T drySamp, T wetSmooth, T drySmooth, T dryInsertEnv, T outSmooth  )
    {
        switch ( m_outMode )
        {
            case outputModes::mix:
            {
                return ( ( wetSample * wetSmooth ) + ( drySamp * drySmooth ) ) * outSmooth;
                break;
            }
            case outputModes::insert:
            {
                return (wetSample + ( drySamp * dryInsertEnv )) * outSmooth ;
                break;
            }
            case outputModes::gate:
            {
                return wetSample * outSmooth;
                break;
            }
            default:
            {
                return (( wetSample * wetSmooth ) + ( drySamp * drySmooth )) * outSmooth;
                break;
            }
        }
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    
    
    static constexpr int NCHANNELS = 2;
    
    sjf_granDelParameters< T > m_gParams;
    
    std::array< sjf_gdVoice< T >, NVOICES > m_delays;
    std::array< std::vector< T >, NCHANNELS > m_buffers;
    
    size_t m_writePos = 0;
    T m_rateHz = 1; // num grains triggered per second
    T m_crosstalk = 0;
    T m_density = 1;
    T m_delayTimeSamps = 22050;
    T m_delayTimeJitter = 0;
    
    bool m_shouldControlFB = false;
    T m_reverseChance = 0.25;
    T m_repeatChance = 0;
    
    std::array< harmonyParams, MAX_N_HARMONIES + 1 > m_harmParams;
    T m_transposition = 0;
    T m_transpositionJitter = 0;
    
    bool m_shouldCrushBits = false;
    int m_bitDepth = 32;
    int m_srDivider = 1;
    int m_outMode = outputModes::mix;
    T m_SR = 44100;
    
    bool m_jitterSync = false;
    T m_syncedJitterDivSamps = m_delayTimeSamps / 8.0;
    T m_syncedJitterDivMax = 8;
    
    long long m_sampleCount = 0;
    long long m_deltaTimeSamps = std::round( m_SR / m_rateHz );
    
    int m_interpType = sjf_interpolators::interpolatorTypes::pureData;
    int m_insertStateCount = 0;
    
    juce::SmoothedValue< T > m_fbSmoother, m_drySmoother, m_wetSmoother, m_outLevelSmoother;
    
    insertDryGrainEnv m_dryGainInsertEnv;
    
    bool m_filterFlag = false;
    T m_filterF = 1000;
    T m_filterQ = 1;
    T m_filterJit = 0;
    int m_filterType = sjf_biquadCalculator<T>::filterType::lowpass;
    
    T m_rmFreq = 100, m_rmSpread = 0, m_rmMix = 1;
    bool m_rmFlag = false;
    T m_rmJit = 0;
    std::array< sjf_lpf< T >, NCHANNELS > m_dcBlock;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_granularDelay )
};

#endif /* sjf_granularDelay_h */
