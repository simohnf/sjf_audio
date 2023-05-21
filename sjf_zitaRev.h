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
#include "../gcem/include/gcem.hpp"
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
    template< int NUM_CHANNELS, int NUM_TAPS, int totalLengthMS >
    struct velveltDelays
    {
        constexpr velveltDelays( ) : dtArray()
        {
            int count = 1;
            T frac = 1.0f / NUM_TAPS; // fraction of total length for first stage
            T minER = 1.0f + random0to1( count ); // make sure we dont get any 0 delay times
            count ++;
            T dtC = totalLengthMS * frac;
            for ( int t = 0; t < NUM_TAPS; t++ )
            {
                for ( int c = 0; c < NUM_CHANNELS; c++ )
                {
                    // space each channel so that the are randomly spaced but with roughly even distribution
                    T dt = ( random0to1( count ) *  dtC ) + minER;
                    count ++;
                    dt += ( dtC * t );
                    dtArray[ c ][ t ] = dt;
                }
            }
        }
        
        const T& getValue ( const int& channel, const int& tap )const { return dtArray[ channel ][ tap ]; }
    private:
        //----------------------------------------
        constexpr T random0to1( int count )
        {
            return (T)sjf_compileTimeRandom::get_random( count ) / (T)RAND_MAX;
        }
        //----------------------------------------
        T dtArray[ NUM_CHANNELS ][ NUM_TAPS ];
    };
    //==============================================================================
    //==============================================================================
    template< int NUM_STAGES, int NUM_CHANNELS, int totalLengthMS >
    struct glVelvetDelayTimes
    {
        constexpr glVelvetDelayTimes( ) : dtArray()
        {
            T c = 0;
            int count = 1;
            
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
    template< int NUM_ROWS, int NUM_COLUMNS >
    struct matrixOfRandomFloats
    {
        constexpr matrixOfRandomFloats( ) : floatArray()
        {
            int count = 1;
            
            for ( int r = 0; r < NUM_ROWS; r++ )
            {
                for (int c = 0; c < NUM_COLUMNS; c ++)
                {
                    floatArray[ r ][ c ] = random0to1( count );
                    count ++;
                }
            }
        }
        
        const T& getValue ( const int& index1, const int& index2 )const { return floatArray[ index1 ][ index2 ]; }
    private:
        //----------------------------------------
        constexpr T random0to1( int count )
        {
            return (T)sjf_compileTimeRandom::get_random( count ) / (T)RAND_MAX;
        }
        //----------------------------------------
        T floatArray[ NUM_ROWS ][ NUM_COLUMNS ];
    };
    //==============================================================================
    template< int NUM_ROWS, int NUM_COLUMNS >
    struct matrixOfRandomBools
    {
        constexpr matrixOfRandomBools( ) : boolArray()
        {

            int count = 1;
            for ( int r = 0; r < NUM_ROWS; r ++)
            {
                for ( int c = 0; c < NUM_COLUMNS; c ++)
                {
                    boolArray[ r ][ c ] = ( random0to1( count ) >= 0.5 ) ? false : true;
                    count ++;
                }
            }
        }
        
        const bool& getValue ( const int& index1, const int& index2 )const { return boolArray[ index1 ][ index2 ]; }
    private:
        //----------------------------------------
        constexpr T random0to1( int count )
        {
            return (T)sjf_compileTimeRandom::get_random( count ) / (T)RAND_MAX;
        }
        //----------------------------------------
        bool boolArray[ NUM_ROWS ][ NUM_COLUMNS ];
    };
    //==============================================================================
    //==============================================================================
    static constexpr int NUM_REV_CHANNELS = 8;
    static constexpr int NUM_ER_STAGES = 4; // for Geraint Luff style reverb
    static constexpr int MAX_ER_TIME = 300;
    static constexpr int TABSIZE = 1024;
    static constexpr int NUM_TAPS = NUM_REV_CHANNELS * NUM_ER_STAGES;

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
    T m_SR = 44100;
    bool m_fbDrive = false;
    bool m_reversePreDelay = false;
    bool m_modType = false;
    bool m_monoLow = false;
    int m_erType = 1;
    
    std::array< sjf_allpass< T >, NUM_REV_CHANNELS > m_allpass;
    std::array< std::array< sjf_delayLine< T >, NUM_REV_CHANNELS >, NUM_ER_STAGES > m_erGLDelays; // for Geraint Luff style reverb
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
    
    std::array< sjf_lpf< T >, NUM_REV_CHANNELS > m_inputLPF, m_inputHPF/*, m_DCBlockers*/;
    sjf_lpf < T > m_midsideHPF;
    
    std::array< T, NUM_REV_CHANNELS > m_erDelayTimesSamples, m_lrDelayTimesSamples;

    std::array< std::array< sjf_lpf< T >, NUM_REV_CHANNELS >, NUM_ER_STAGES > m_glERSizeSmooth;
    std::array< std::array< bool, NUM_REV_CHANNELS >, NUM_ER_STAGES > m_glERFlip;
    std::array< std::array< T, NUM_REV_CHANNELS >, NUM_ER_STAGES > m_glERDelayTimesSamps;

    std::array< sjf_delayLine< T >, NUM_REV_CHANNELS > m_multitapErDelays;
    std::array< std::array< T, NUM_TAPS >, NUM_REV_CHANNELS > m_multitapERDelayTimesSamps, m_multitapERScaling;
    
    enum m_erTypes { zitaRev = 1, zitaRevType2, geraintLuff, multitap };
public:
    //==============================================================================
    sjf_zitaRev()
    {
        m_revSamples.fill( 0 );
        setinputHPFCutoff( m_inputHPFCutoff );
        setinputLPFCutoff( m_inputLPFCutoff );
        setLrHPFCutoff( m_lrHPFCutoff );
        setLrLPFCutoff( m_lrLPFCutoff );
//        setGLERDelays
        
    }
    //==============================================================================
    ~sjf_zitaRev(){}
    //==============================================================================
    void initialise( const int &sampleRate, const int &totalNumInputChannels, const int &totalNumOutputChannels, const int &samplesPerBlock )
    {
        m_SR = sampleRate;
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
            setERGLDelayTimes(); // no smoothing to save cpu????
            setMultitapScaling( diffusionSmoothed );
            // different early reflection types
            switch( m_erType )
            {
                case m_erTypes::zitaRev:
                    earlyReflectionsType1( inScale );
                    break;
                case m_erTypes::zitaRevType2:
                    earlyReflectionsType2( inScale );
                    break;
                case m_erTypes::geraintLuff:
                    earlyReflectionsType3( inScale );
                    break;
                case m_erTypes::multitap:
                    earlyReflectionsType4( inScale );
                    break;
                default:
                    earlyReflectionsType1( inScale );
                    break;
            }

            
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
                
                if ( m_fbDrive )
                {
                    m_revSamples[ i ] = juce::dsp::FastMathApproximations::tanh( m_revSamples[ i ] * DRIVE );
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
    void setinputLPFCutoff( const T &newCutoff )
    {
        m_inputLPFCutoff =  newCutoff;
        for ( int i = 0; i < m_nInChannels; i++ ) { m_inputLPF[ i ].setCutoff( m_inputLPFCutoff ); }
    }
    //==============================================================================
    void setinputHPFCutoff( const T &newCutoff )
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
            
            for ( int s = 0; s < NUM_ER_STAGES; s++ )
            { m_erGLDelays[ s ][ i ].setInterpolationType( newInterpolation ); }
            
            m_multitapErDelays[ i ].setInterpolationType( newInterpolation );
        }
        m_shimmer.setInterpolationType( newInterpolation );
    }
    //==============================================================================
    void setfeedbackDrive( const bool& shouldLimitFeedback )
    {
        m_fbDrive = shouldLimitFeedback;
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
    void setEarlyReflectionType ( const int& erType )
    {
        m_erType = erType;
        if ( m_erType != m_erTypes::zitaRev && m_erType != m_erTypes::zitaRevType2 )
        {
            for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
            {
                m_allpass[ i ].clearDelayline();
            }
        }
        if ( m_erType != m_erTypes::geraintLuff )
        {
            for ( int s = 0; s < NUM_ER_STAGES; s++ )
            {
                for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
                {
                    
                    m_erGLDelays[ s ][ i ].clearDelayline();
                }
            }
        }
        if ( m_erType != m_erTypes::multitap )
        {
            for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
            {
                m_multitapErDelays[ i ].clearDelayline();
            }
        }
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
        for ( int i = 0; i < NUM_REV_CHANNELS; i++ ) { m_allpass[ i ].setCoefficient( diffusionSmoothed ); }
    }
    //==============================================================================
    inline void setMultitapScaling( const T& diffusionSmoothed )
    {
        static constexpr auto randomMatrix = matrixOfRandomFloats< NUM_REV_CHANNELS, NUM_TAPS>( );
        for ( int t = 0; t < NUM_TAPS; t++ )
        {
            for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
            {
                m_multitapERScaling[ i ][ t ] = sjf_scale< T >( randomMatrix.getValue( i, t ), 0, 1, 0.4, 0.9 );
                if ( t % NUM_REV_CHANNELS != 0 ){ m_multitapERScaling[ i ][ t ] *= diffusionSmoothed; }
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
    inline void setERGLDelayTimes()
    {
        for ( int s = 0; s < NUM_ER_STAGES; s++ )
        {
            for ( int c = 0; c < NUM_REV_CHANNELS; c++ )
            {
                m_erGLDelays[ s ][ c ].setDelayTimeSamps( m_glERSizeSmooth[ s ][ c ].filterInput(m_glERDelayTimesSamps[ s ][ c ] ) );
            }
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
        static constexpr int NUM_ALLPASS_CHANNELS = 2;
        static constexpr int NUM_ALLPASS_STAGES = NUM_REV_CHANNELS / NUM_ALLPASS_CHANNELS;
        static constexpr int LAST_ALLPASS_STAGE = NUM_ALLPASS_STAGES - 1;
        // allpass diffussion
        for ( int i = 0; i < LAST_ALLPASS_STAGE; i++)
        {
            for ( int j = 0; j < NUM_ALLPASS_CHANNELS; j++ )
            {
                m_allpass[ (i * NUM_ALLPASS_CHANNELS) + j ].filterInPlace( m_inOutSamps[ j ] );
            }
            Hadamard< T, NUM_ALLPASS_CHANNELS >::inPlace( m_inOutSamps.data(), inScale );
        }
        for ( int j = 0; j < NUM_ALLPASS_CHANNELS; j++ )
        {
            m_allpass[ (LAST_ALLPASS_STAGE * NUM_ALLPASS_CHANNELS) + j ].filterInPlace( m_inOutSamps[ j ] );
        }
        for ( int i = 0; i < NUM_ALLPASS_CHANNELS; i++ )
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
        static constexpr auto flipMatrix = matrixOfRandomBools< NUM_ER_STAGES, NUM_REV_CHANNELS >();
        // distribute input across rev channels
        for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
        { temp[ i ] = ( m_inOutSamps[ fastMod( i, m_nInChannels ) ]  * m_flip[ i ] * inScale ); }
        
        // pass through a series of multichannel delays with polrity flips and hadmard mixing
        for ( int s = 0; s < NUM_ER_STAGES; s++ )
        {
            for ( int c = 0; c < NUM_REV_CHANNELS; c++ )
            {
                m_erGLDelays[ s ][ c ].setSample2( temp[ c ] );
                temp[ c ] = m_erGLDelays[ s ][ c ].getSample2();
                if ( flipMatrix.getValue( s, c ) ){ temp[ c ] *= -1.0f; }
            }
            Hadamard< T, NUM_REV_CHANNELS >::inPlace( temp.data(), hadScale );
            for ( int c = 0; c < NUM_REV_CHANNELS; c++ )
            {
                m_revSamples[ c ] += temp[ c ] * glScale ;
            }
        }
    }
    //==============================================================================
    inline void earlyReflectionsType4( const T& inScale )
    {
        // early reflections based on multitap delayLine... a little as per Moorer "About this reverberation Business"
        static constexpr auto flipMatrix = matrixOfRandomBools< NUM_REV_CHANNELS, NUM_TAPS >();
        for ( int i = 0; i < m_nInChannels; i++ )
        {
            m_multitapErDelays[ i ].setSample2( m_inOutSamps[ i ] * inScale ); // write input to delay lines
        }

        for ( int i = 0; i < m_nInChannels; i++ )
        {
            for ( int t = 0; t < NUM_TAPS; t++ ) // count through taps
            {
                m_multitapErDelays[ i ].setDelayTimeSamps( m_multitapERDelayTimesSamps[ i ][ t ] ); // set delay Time for each tap
//                m_revSamples[ i ] += ( flipMatrix.getValue( i, t ) ) ? m_multitapErDelays[ i ].getSample2( ) * m_multitapERScaling[ i ][ t ] : m_multitapErDelays[ i ].getSample2( ) * m_multitapERScaling[ i ][ t ] * -1.0f; // copy output to buffer with amplitude scale and random polarity flips...
                m_revSamples[ i ] += ( flipMatrix.getValue( i, t ) ) ? m_multitapErDelays[ i ].getSampleRoundedIndex2( ) * m_multitapERScaling[ i ][ t ] : m_multitapErDelays[ i ].getSampleRoundedIndex2( ) * m_multitapERScaling[ i ][ t ] * -1.0f; // copy output to buffer with amplitude scale and random polarity flips...

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
            int nSteps = std::min( static_cast<int>(NUM_REV_CHANNELS), nInChannels );
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
            for ( int s = 0; s < NUM_ER_STAGES; s++ )
            {
                m_glERDelayTimesSamps[ s ][ i ] = m_erGLDelays[ s ][ i ].size() * sizeFactor;
            }
            const T sampsPerMS = m_SR * 0.001;
//            static constexpr std::array< glVelvetDelayTimes< 1, NUM_TAPS, MAX_ER_TIME >, NUM_REV_CHANNELS > multiTapTimes;
            static constexpr auto multiTapTimes = velveltDelays< NUM_REV_CHANNELS, NUM_TAPS, MAX_ER_TIME / 2 >();
            for ( int t = 0; t < NUM_TAPS; t ++ )
            {
                m_multitapERDelayTimesSamps[ i ][ t ] = multiTapTimes.getValue( i, t ) * sampsPerMS * sizeFactor;
                DBG( i << " " << t << " m_multitapERDelayTimesSamps " << m_multitapERDelayTimesSamps[ i ][ t ] << " m_multitapERScaling " << m_multitapERScaling[ i ][ t ] );
            }
            DBG("");
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
        
        static constexpr auto m_glERDelayTimesMS = glVelvetDelayTimes< NUM_ER_STAGES, NUM_REV_CHANNELS, MAX_ER_TIME >();
        const T sampsPerMS = sampleRate * 0.001;
        for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
        {
            dt = sampleRate * allpassT[ i ];
            m_allpass[ i ].initialise( 2 * round( dt ) );
            m_allpass[ i ].setCoefficient( m_diffusion );
            
            
            for ( int s = 0; s < NUM_ER_STAGES; s++ )
            {
                m_erGLDelays[ s ][ i ].initialise( 2 * m_glERDelayTimesMS.getValue( s, i ) * sampsPerMS );
            }
            
            m_multitapErDelays[ i ].initialise( 2 * MAX_ER_TIME * sampsPerMS );
            
            dt = ( sampleRate * totalDelayT[ i ]) - dt;
            m_delays[ i ].initialise( 2 * round( dt ) );
            
            m_preDelay[ i ].initialise( sampleRate, sampleRate * 2);
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
            
            for ( int s = 0; s < NUM_ER_STAGES; s++ )
            {
                m_glERSizeSmooth[ s ][ i ].setCutoff( smoothSlewVal );
            }
        }
        
    }
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_zitaRev )
};



#endif /* sjf_zitaRev_h */




