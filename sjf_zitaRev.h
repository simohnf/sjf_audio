//
//  sjf_zitaRev.h
//
//  Created by Simon Fay on 07/10/2022.
//


#ifndef sjf_zitaRev_h
#define sjf_zitaRev_h
#include <JuceHeader.h>
#include "sjf_audioUtilities.h"
#include "sjf_delayLine.h"
#include "sjf_comb.h"
#include "sjf_lpf.h"
#include "sjf_randOSC.h"
#include "sjf_phasor.h"
#include "sjf_pitchShift.h"
#include "sjf_wavetables.h"
#include <algorithm>    // std::random_erShuffle
#include <random>       // std::default_random_engine
#include <vector>
#include <time.h>
#include "/Users/simonfay/Programming_Stuff/gcem/include/gcem.hpp"
#include "sjf_compileTimeRandom.h"
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
template < typename T >
class sjf_zitaRev
{
private:
    //==============================================================================
    //==============================================================================
    template< int NUM_STAGES, int NUM_CHANNELS, int totalLengthMS >
    struct glDelayTimes
    {
        constexpr glDelayTimes( ) : dtArray()
        {
            T c = 0;
            int count = 0;
            
            for ( int s = 0; s < NUM_STAGES; s++ ) { c += gcem::pow(2, s); }
            T frac = 1.0f / c; // fraction of total length for first stage
            for ( int s = 0; s < NUM_STAGES; s++ )
            {
                T minER = 1.0f + random0to1( count ); // make sure we dont get any 0 delay times
                count ++;
                T dtC = totalLengthMS * frac * gcem::pow(2, s) / (T)NUM_CHANNELS;
                for (int c = 0; c < NUM_CHANNELS; c ++)
                {
                    // space each channel so that the are randomly spaced but with roughly even distribution
                    T dt = ( random0to1( count ) *  dtC ) + minER;
                    count ++;
                    dt += ( dtC * c );
                    dtArray[ s ][ c ] = dt;
                }
            }
        }
    
        const T& getValue ( const int& index1, const int& index2 )const { return dtArray[ index1 ][ index2 ]; }
    private:
        //----------------------------------------
        constexpr T random0to1( int count )
        {
            return (T)sjf_compileTimeRandom::get_random( count ) / (T)RAND_MAX;
        }
        //----------------------------------------
        T dtArray[ NUM_STAGES ][ NUM_CHANNELS ];
    };
    //==============================================================================
    //==============================================================================
    static constexpr int NUM_REV_CHANNELS = 8;
    static constexpr int NUM_ER_STAGES = 4; // for Geraint Luff style reverb
    static constexpr int glERTime = 300;
    static constexpr int TABSIZE = 1024;
    

    static constexpr int PHASOR_OFFSET = TABSIZE / ( NUM_REV_CHANNELS ) ; // every delay gets a slightly different phase for sinewave modulation
    static constexpr int PHASOR_OFFSET2 = TABSIZE / ( NUM_REV_CHANNELS * 2 ) ;
    static constexpr int PHASOR_OFFSET_GL = TABSIZE / ( NUM_REV_CHANNELS * NUM_ER_STAGES ) ;
    
    static constexpr T DRIVE = 1.01f;
    
    int m_nInChannels = 2, m_nOutChannels = 2;
    T m_dry = 0, m_wet = 1, m_FB = 0.85, m_size = 1, m_modD = 0.1;
    T m_shimLevel = 0, m_shimTranspose = 2;
    T m_lrLPFCutoff = 0.99, m_lrHPFCutoff = 0.001;
    T m_inputLPFCutoff = 0.7, m_inputHPFCutoff = 0.01;
    T m_diffusion = 0.6;
    bool m_fbControl = false;
    bool m_reversePreDelay = false;
    bool m_modType = false;
    bool m_monoLow = false;
    bool m_erType = false;
    
    std::array< sjf_allpass< T >, NUM_REV_CHANNELS > m_allpass;
    std::array< std::array< sjf_delayLine< T >, NUM_REV_CHANNELS >, NUM_ER_STAGES > m_erDelays; // for Geraint Luff style reverb
    std::array< sjf_delayLine< T >, NUM_REV_CHANNELS > m_delays;
    std::array< sjf_reverseDelay< T >, NUM_REV_CHANNELS > m_preDelay;
    
    std::array< sjf_randOSC< T >, NUM_REV_CHANNELS > m_ERRandomModulator, m_LRRandomModulator; // modulators for delayTimes
    std::array< std::array< sjf_randOSC< T >, NUM_REV_CHANNELS >, NUM_ER_STAGES > m_glERModulators; // modulators for delayTimes
    sjf_phasor< T > m_modPhasor; // phasor for sinewave modulation
    std::array< T, NUM_REV_CHANNELS > m_flip, m_revSamples, m_inOutSamps; // polarity flips and storage of samples
    
    sjf_pitchShift< T > m_shimmer; // shimmer
    sjf_lpf< T > m_shimLPF, m_shimLPF2, m_shimHPF;
    
    sjf_lpf< T > m_modDSmooth, m_shimTransposeSmooth, m_FBsmooth, m_diffusionSmooth; // dezipper variables
    std::array< sjf_lpf< T >, NUM_REV_CHANNELS > m_hiPass, m_lowPass, m_erSizeSmooth, m_lrSizeSmooth; // dezipper individual delay times
    std::array< std::array< sjf_lpf< T >, NUM_REV_CHANNELS >, NUM_ER_STAGES >
    std::array< sjf_lpf< T >, NUM_REV_CHANNELS > m_inputLPF, m_inputHPF/*, m_DCBlockers*/;
    sjf_lpf < T > m_midsideHPF;
    
    std::array< T, NUM_REV_CHANNELS > m_erDelayTimesSamples, m_lrDelayTimesSamples;

    std::array< std::array< T, NUM_REV_CHANNELS >, NUM_ER_STAGES > m_glERDelayTimesSamps;

    
public:
    //==============================================================================
    sjf_zitaRev()
    {
        m_revSamples.fill( 0 );
        setErHPFCutoff( m_inputHPFCutoff );
        setErLPFCutoff( m_inputLPFCutoff );
        setLrHPFCutoff( m_lrHPFCutoff );
        setLrLPFCutoff( m_lrLPFCutoff );
//        setGLERDelays
        
    }
    //==============================================================================
    ~sjf_zitaRev(){}
    //==============================================================================
    void initialise( const int &sampleRate, const int &totalNumInputChannels, const int &totalNumOutputChannels, const int &samplesPerBlock )
    {
        m_nInChannels = totalNumInputChannels;
        m_nOutChannels = totalNumOutputChannels;
        
        setPolarityFlips( m_nInChannels );

        initialiseDelayLines( sampleRate );
        calculateDelayTimes( 1 );
        setPreDelay( 0.005 * sampleRate );
        
        initialiseModulators( sampleRate );
        
        m_midsideHPF.setCutoff( calculateLPFCoefficient< T >( 150, sampleRate ) );
        T smoothSlewVal = calculateLPFCoefficient< T >( 1, sampleRate );
        initialiseVariableSmoothers( smoothSlewVal );
        
        initialiseShimmer( sampleRate );
        
    }
    //==============================================================================

    //==============================================================================
    void processAudio( juce::AudioBuffer<T> &buffer )
    {
        const auto bufferSize = buffer.getNumSamples();
        static constexpr T hadScale = 1.0f / gcem::sqrt( NUM_REV_CHANNELS );
        static constexpr auto sinTab = sinArray< T, TABSIZE >();
        
        T modDepthSmoothed, FBSmoothed, diffusionSmoothed, phasorOut/*, drySamp, wetSamp, dt,  shimOutput,  mid, side*/;
        
        bool modulateDelays = true;
        
        bool shimmerOn = ( m_shimLevel > 0 ) ? true : false;
        if ( !shimmerOn ) { m_shimmer.clearDelayline(); }
        const T shimWetLevel = sqrt( m_shimLevel );
        const T shimDryLevel = sqrt( 1 - m_shimLevel );
        
        const T inScale = 1.0f / sqrt( (T)m_nInChannels );
        
        for ( int indexThroughBuffer = 0; indexThroughBuffer < bufferSize; indexThroughBuffer++ )
        {
            // first calculate smoothed global variables
            modDepthSmoothed = m_modType ? m_modDSmooth.filterInput( m_modD ) : m_modDSmooth.filterInput( m_modD ) * 0.5f; // sine modulation seems more extreme than random so if using sine temper it somewhat
            modulateDelays = ( modDepthSmoothed <= 0 ) ? false : true;
            
            FBSmoothed = m_FBsmooth.filterInput( m_FB );
            diffusionSmoothed = m_diffusionSmooth.filterInput( m_diffusion );
            
            phasorOut = m_modPhasor.output() * TABSIZE;
            
            // PreDelay First
            preDelay( buffer, indexThroughBuffer );
            
            // early reflections
            setAllpassCoefficients( diffusionSmoothed );
            // calculate allpass delay times
            setAllPassDelayTimes( sinTab, modDepthSmoothed, modulateDelays, phasorOut /*, dt */);
            // different early reflection types
            if ( !m_erType ) { earlyReflectionsType1( inScale ); }
            else { earlyReflectionsType3( inScale ); }

            
            // mix
            Hadamard< T, NUM_REV_CHANNELS >::inPlace( m_revSamples.data(), hadScale );
            
            // copy to output
            output( buffer, indexThroughBuffer );
            
            // LONG LASTING --> Late reflections / reverb cluster
            for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
            {
                // filter things
                m_lowPass[ i ].filterInPlace( m_revSamples[ i ] );
                m_hiPass[ i ].filterInPlaceHP( m_revSamples[ i ] );
                m_delays[ i ].setSample2( m_revSamples[ i ] ); // feed through delay lines
            }

            setLateReflectionDelayTimes( sinTab, modDepthSmoothed, modulateDelays, phasorOut /*, dt */);

            for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
            {
                m_revSamples[ i ] = m_delays[ i ].getSample2()  * FBSmoothed ;
                
                if ( m_fbControl )
                {
                    m_revSamples[ i ] = juce::dsp::FastMathApproximations::tanh( m_revSamples[ i ] * DRIVE );
//                    m_revSamples[ i ] -= m_DCBlockers[ i ].filterInput( m_revSamples[ i ] );
                }
            }
//            Householder< T, NUM_REV_CHANNELS >::mixInPlace( m_revSamples );
            // SHIMMER
            if ( shimmerOn ) { processShimmer( shimDryLevel, shimWetLevel ); }
        }
    }
    //==============================================================================
    void setSize( const T &newSize )
    {
        if ( m_size != newSize )
        {
            m_size = newSize;
            calculateDelayTimes( sjf_scale< T >( m_size, 0, 100, 0.1, 1.0 ) );
        }
    }
    //==============================================================================
    void setModulationRate( const T& newModRate )
    {
        m_modPhasor.setFrequency( newModRate );
        
        for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
        {
            m_ERRandomModulator[ i ].setFrequency( newModRate );
            m_LRRandomModulator[ i ].setFrequency( newModRate );
        }
    }
    //==============================================================================
    void setModulationDepth( const T &newModDepth )
    {
        m_modD = sjf_scale< T > ( newModDepth, 0, 100, -0.00001, 0.999f );
    }
    //==============================================================================
    void setModulationType( const bool& trueForRandomFalseForSin )
    {
        m_modType = trueForRandomFalseForSin;
    }
    //==============================================================================
    void setDecay( const T &newDecay )
    {
        m_FB = sjf_scale< T > ( newDecay, 0, 100, 0, 1. );// * 0.01;
    }
    //==============================================================================
    void setMix( const T &newMix )
    {
        m_dry = sqrt( 1 - ( newMix * 0.01 ) );
        m_wet = sqrt( newMix * 0.01 );
    }
    //==============================================================================
    void setLrLPFCutoff( const T &newCutoff )
    {
        m_lrLPFCutoff = newCutoff;
        for ( int i = 0; i < NUM_REV_CHANNELS; i++ ) { m_lowPass[ i ].setCutoff( m_lrLPFCutoff ); }
    }
    //==============================================================================
    void setLrHPFCutoff( const T &newCutoff )
    {
        m_lrHPFCutoff = newCutoff;
        for ( int i = 0; i < NUM_REV_CHANNELS; i++ ) { m_hiPass[ i ].setCutoff( m_lrHPFCutoff ); }
    }
    //==============================================================================
    void setErLPFCutoff( const T &newCutoff )
    {
        m_inputLPFCutoff =  newCutoff;
        for ( int i = 0; i < m_nInChannels; i++ ) { m_inputLPF[ i ].setCutoff( m_inputLPFCutoff ); }
    }
    //==============================================================================
    void setErHPFCutoff( const T &newCutoff )
    {
        m_inputHPFCutoff = newCutoff;
        for ( int i = 0; i < m_nInChannels; i++ ) { m_inputHPF[ i ].setCutoff( m_inputHPFCutoff ); }
    }
    //==============================================================================
    void setShimmerLevel( const T &newShimmerLevel )
    {
        m_shimLevel = sjf_scale< T >( newShimmerLevel, 0, 100, 0, 1 );
        m_shimLevel *= m_shimLevel * m_shimLevel;
        m_shimLevel *= 0.5;
    }
    //==============================================================================
    void setShimmerTransposition( const T &newShimmerTransposition )
    {
        m_shimTranspose = pow( 2.0f, ( newShimmerTransposition / 12.0f ) );
    }
    //==============================================================================
    void setInterpolationType( const int &newInterpolation )
    {
        for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
        {
            m_allpass[ i ].setInterpolationType( newInterpolation );
            m_delays[ i ].setInterpolationType( newInterpolation );
        }
        m_shimmer.setInterpolationType( newInterpolation );
    }
    //==============================================================================
    void setFeedbackControl( const bool& shouldLimitFeedback )
    {
        m_fbControl = shouldLimitFeedback;
    }
    //==============================================================================
    void setPreDelay( const T& preDelayInSamps )
    {
        for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
        { m_preDelay[ i ].setDelayTimeSamps( preDelayInSamps ); }
    }
    //==============================================================================
    void reversePredelay( const bool& trueIfReversed )
    {
        m_reversePreDelay = trueIfReversed;
    }
    //==============================================================================
    void setDiffusion( const T& diffusion )
    {
        m_diffusion = sjf_scale< T >( diffusion, 0, 100, 0.001, 0.6 );
    }
    //==============================================================================
    void setMonoLow( const bool& trueIfMonoLow )
    {
        m_monoLow = trueIfMonoLow;
    }
    //==============================================================================
    void setEarlyReflectionType ( const bool& erType )
    {
        m_erType = erType;
    }
private:
    //==============================================================================
    inline void preDelay( juce::AudioBuffer<T> &buffer, const int& indexThroughBuffer )
    {
        for ( int i = 0; i < m_nInChannels; i++ )
        {
            m_preDelay[ i ].setSample2( buffer.getSample( i , indexThroughBuffer ) ); // feed into predelay
            m_inOutSamps[ i ] = m_reversePreDelay ? m_preDelay[ i ].getSampleReverse( ) : m_preDelay[ i ].getSample2( );
            
            m_inputLPF[ i ].filterInPlace( m_inOutSamps[ i ] );
            m_inputHPF[ i ].filterInPlaceHP( m_inOutSamps[ i ] );
        }
    }
    //==============================================================================
    inline void setAllpassCoefficients( const T& diffusionSmoothed )
    {
        for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
        {
            m_allpass[ i ].setCoefficient( diffusionSmoothed );
        }
    }
    //==============================================================================
    inline void setGLDelayTimes( const sinArray< T, TABSIZE >& sinTab, const T& modDepthSmoothed, const bool& modulateDelays, T& phasorOut/*, T& dt*/ )
    {
        T dt;
        for ( int s = 0; s < NUM_ER_STAGES; s++ )
        {
            for ( int c = 0; c < NUM_REV_CHANNELS; c++ )
            {
                // calculate allpass delay times
                dt = m_erSizeSmooth[ i ].filterInput( m_erDelayTimesSamples[ i ] );
                if ( modulateDelays )
                {
                    if ( m_modType )
                    { dt += ( m_ERRandomModulator[ i ].output() * dt * modDepthSmoothed ); }
                    else
                    {
                        dt += ( sinTab.getValue( phasorOut ) * dt * modDepthSmoothed );
                        phasorOut += PHASOR_OFFSET;
                        fastMod3< T >( phasorOut, TABSIZE);
                    }
                }
                m_allpass[ i ].setDelayTimeSamps( dt );
            }
        }
        
    }
    //==============================================================================
    inline void setAllPassDelayTimes( const sinArray< T, TABSIZE >& sinTab, const T& modDepthSmoothed, const bool& modulateDelays, T& phasorOut/*, T& dt*/ )
    {
        T dt;
        for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
        {
            // calculate allpass delay times
            dt = m_erSizeSmooth[ i ].filterInput( m_erDelayTimesSamples[ i ] );
            if ( modulateDelays )
            {
                if ( m_modType )
                { dt += ( m_ERRandomModulator[ i ].output() * dt * modDepthSmoothed ); }
                else
                {
                    dt += ( sinTab.getValue( phasorOut ) * dt * modDepthSmoothed );
                    phasorOut += PHASOR_OFFSET;
                    fastMod3< T >( phasorOut, TABSIZE);
                }
            }
            m_allpass[ i ].setDelayTimeSamps( dt );
        }
    }
    //==============================================================================
    inline void earlyReflectionsType1( const T& inScale )
    {
        // based on zita rev
        // distribute input across rev channels
        for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
        { m_revSamples[ i ] += ( m_inOutSamps[ fastMod( i, m_nInChannels ) ]  * m_flip[ i ] * inScale ); }
        
        
        // allpass diffussion
        for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
        {
            // then feed through allpass filterInPlace2Multiply
            m_allpass[ i ].filterInPlace( m_revSamples[ i ] );
        }
    }
    //==============================================================================
    inline void earlyReflectionsType2( const T& inScale )
    {
        // parallel stereo allpass with hadamard mixing only fed to channels 1 & 2
        static constexpr int NUM_CHANNELS = 2;
        // allpass diffussion
        for ( int i = 0; i < NUM_REV_CHANNELS / 2; i++)
        {
            Hadamard< T, NUM_CHANNELS >::inPlace( m_inOutSamps.data(), inScale );
            for ( int j = 0; j < NUM_CHANNELS; j++ )
            {
                m_allpass[ (i * NUM_CHANNELS) + j ].filterInPlace( m_inOutSamps[ j ] );
            }
        }
        for ( int i = 0; i < NUM_CHANNELS; i++ )
        {
            m_revSamples[ i ] += m_inOutSamps[ i ];
        }
    }
    //==============================================================================
    inline void earlyReflectionsType3( const T& inScale )
    {
        // early reflections based on geraint luff -> let's write a reverb https://signalsmith-audio.co.uk/writing/2021/lets-write-a-reverb/
        std::array< T, NUM_REV_CHANNELS > temp;
        static constexpr T glScale = 1.0f / gcem::sqrt( NUM_ER_STAGES );
        static constexpr T hadScale = 1.0f / gcem::sqrt( NUM_REV_CHANNELS );
        
        // distribute input across rev channels
        for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
        { temp[ i ] = ( m_inOutSamps[ fastMod( i, m_nInChannels ) ]  * m_flip[ i ] * inScale ); }
        
        for ( int s = 0; s < NUM_ER_STAGES; s++ )
        {
            for ( int c = 0; c < NUM_REV_CHANNELS; c++ )
            {
                m_erDelays[ s ][ c ].setDelayTimeSamps( m_glERDelayTimesSamps[ s ][ c ] );
            }
            for ( int c = 0; c < NUM_REV_CHANNELS; c++ )
            {
                m_erDelays[ s ][ c ].setSample2( temp[ c ] );
                temp[ c ] = m_erDelays[ s ][ c ].getSample2();
            }
            Hadamard< T, NUM_REV_CHANNELS >::inPlace( temp.data(), hadScale );
            for ( int c = 0; c < NUM_REV_CHANNELS; c++ )
            {
                m_revSamples[ c ] += temp[ c ] /* * glScale */;
            }
        }
        
        
    }
    //==============================================================================
    inline void output( juce::AudioBuffer<T> &buffer, const int& indexThroughBuffer )
    {
        // copy to output
        for ( int i = 0; i < m_nOutChannels; i++ )
        {
            m_inOutSamps[ i ] = m_revSamples[ i ] * m_wet;
        }
        if ( m_monoLow && m_nOutChannels == 2 )
        {
            T mid = m_inOutSamps[ 0 ] + m_inOutSamps[ 1 ];  // sum for mid
            T side = m_inOutSamps[ 0 ] - m_inOutSamps[ 1 ];
            // filter
            m_midsideHPF.filterInPlaceHP( side );
            
            m_inOutSamps[ 0 ] = ( mid + side ) * 0.5;
            m_inOutSamps[ 1 ] = ( mid - side ) * 0.5;
        }
        T drySamp;
        for ( int i = 0; i < m_nOutChannels; i++ )
        {
            drySamp = buffer.getSample(  fastMod( i, m_nInChannels ) , indexThroughBuffer ) * m_dry;
            buffer.setSample( i, indexThroughBuffer, drySamp + m_inOutSamps[ i ] );
        }
    }
    //==============================================================================
    inline void setLateReflectionDelayTimes( const sinArray< T, TABSIZE >& sinTab, const T& modDepthSmoothed, const bool& modulateDelays, T& phasorOut )
    {
        T dt;
        phasorOut += PHASOR_OFFSET2; // shift latereflection offset so they're misaligned with allpass
        fastMod3< T >( phasorOut, TABSIZE);
        // save delayed values for next sample
        for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
        {
            dt = m_lrSizeSmooth[ i ].filterInput( m_lrDelayTimesSamples[ i ] );
            if ( modulateDelays )
            {
                if ( m_modType )
                { dt += ( dt * m_LRRandomModulator[ i ].output() * modDepthSmoothed ); }
                else
                {
                    dt += ( sinTab.getValue( phasorOut ) * dt * modDepthSmoothed );
                    phasorOut += PHASOR_OFFSET;
                    fastMod3< T >( phasorOut, TABSIZE);
                }
            }
            m_delays[ i ].setDelayTimeSamps( dt );
        }
    }
    //==============================================================================
    inline void processShimmer( const T& shimDryLevel, const T& shimWetLevel )
    {
        constexpr int lastChannel = NUM_REV_CHANNELS - 1;
        T shimOutput;
        
        m_shimmer.setSample( m_shimLPF.filterInput( m_revSamples[ lastChannel ] ) ); // no need to pitchShift Everything because it all gets mixed anyway
        shimOutput = m_shimmer.pitchShiftOutput( m_shimTransposeSmooth.filterInput( m_shimTranspose ) ) ;
        m_shimHPF.filterInPlaceHP( shimOutput );
        m_shimLPF2.filterInPlace( shimOutput );
        m_revSamples[ lastChannel ] *= shimDryLevel;
        m_revSamples[ lastChannel ] += ( shimOutput * shimWetLevel );
    }
    //==============================================================================
    void setPolarityFlips( const int& nInChannels )
    {
        bool flipped = false;
        for ( int i = 0; i < NUM_REV_CHANNELS; i += nInChannels )
        {
            T val = flipped ? -1.0f : 1.0f;
            int nSteps = std::min( (int)NUM_REV_CHANNELS, nInChannels );
            for ( int j = 0; j < nSteps; j++ ) { m_flip[ i + j ] = val; }
            flipped = !flipped;
        }
    }
    //==============================================================================
    void calculateDelayTimes( const T &proportionOfMaxSize )
    {
        T sizeFactor = proportionOfMaxSize * 0.5; // max delay vector size is double max delay time!!!
        for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
        {
            m_erDelayTimesSamples[ i ] = m_allpass[ i ].size() * sizeFactor;
            m_lrDelayTimesSamples[ i ] = m_delays[ i ].size() * sizeFactor;
        }
    }
    //==============================================================================
    void initialiseModulators( const T& sampleRate )
    {
        m_modPhasor.initialise( sampleRate, 1 );
        for ( int i = 0 ; i < NUM_REV_CHANNELS; i++ )
        {
            m_ERRandomModulator[ i ].initialise( sampleRate );
            m_ERRandomModulator[ i ].setFrequency( 0.1 );
            
            m_LRRandomModulator[ i ].initialise( sampleRate );
            m_LRRandomModulator[ i ].setFrequency( 0.1 );
        }
    }
    //==============================================================================
    void initialiseShimmer( const T& sampleRate )
    {
        m_shimmer.initialise( sampleRate, 100.0f );
        m_shimmer.setDelayTimeSamps( 2 );
        
        m_shimLPF.setCutoff( calculateLPFCoefficient< T >( 2000, sampleRate ) );
        m_shimLPF2.setCutoff( calculateLPFCoefficient< T >( 2000, sampleRate ) );
        m_shimHPF.setCutoff( calculateLPFCoefficient< T >( 20, sampleRate ) );
    }
    //==============================================================================
    void initialiseDelayLines( const T& sampleRate )
    {
        T dt;
        
        static constexpr std::array< T, NUM_REV_CHANNELS > allpassT
        { 0.020346, 0.024421, 0.031604, 0.027333, 0.022904, 0.029291, 0.013458, 0.019123 };
        
        static constexpr std::array< T, NUM_REV_CHANNELS > totalDelayT
        { 0.153129, 0.210389, 0.127837, 0.256891, 0.174713, 0.192303, 0.125, 0.219991 };
        
        for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
        {
            dt = sampleRate * allpassT[ i ];
            m_allpass[ i ].initialise( 2 * round( dt ) );
            m_allpass[ i ].setCoefficient( m_diffusion );
            
            dt = ( sampleRate * totalDelayT[ i ]) - dt;
            m_delays[ i ].initialise( 2 * round( dt ) );
            
            m_preDelay[ i ].initialise( sampleRate, sampleRate * 0.001);
        }
        
        static constexpr auto m_glERDelayTimesMS = glDelayTimes< NUM_ER_STAGES, NUM_REV_CHANNELS, glERTime >();
        for ( int s = 0; s < NUM_ER_STAGES; s++ )
        {
            for ( int c = 0; c < NUM_REV_CHANNELS; c++ )
            {
                m_glERDelayTimesSamps[ s ][ c ] = m_glERDelayTimesMS.getValue( s, c ) * sampleRate * 0.001;
                m_erDelays[ s ][ c ].initialise( 2 * m_glERDelayTimesSamps[ s ][ c ] );
            }
        }
    }
    //==============================================================================
    void initialiseVariableSmoothers( const T& smoothSlewVal )
    {
        m_shimTransposeSmooth.setCutoff( smoothSlewVal );
        
        m_modDSmooth.setCutoff( smoothSlewVal );
        m_FBsmooth.setCutoff( smoothSlewVal );
        m_diffusionSmooth.setCutoff( smoothSlewVal );
        
        for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
        {
            m_erSizeSmooth[ i ].setCutoff( smoothSlewVal );
            m_lrSizeSmooth[ i ].setCutoff( smoothSlewVal );
            
//            m_DCBlockers[ i ].setCutoff( smoothSlewVal );
        }
    }
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_zitaRev )
};



#endif /* sjf_zitaRev_h */




