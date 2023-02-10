//
//  sjf_zitaRev.h
//
//  Created by Simon Fay on 07/10/2022.
//


#ifndef sjf_zitaRev_h
#define sjf_zitaRev_h
#include <JuceHeader.h>
#include "sjf_audioUtilities.h"
//#include "sjf_monoDelay.h"
#include "sjf_delayLine.h"
#include "sjf_comb.h"
#include "sjf_lpf.h"
#include "sjf_randOSC.h"
#include "sjf_phasor.h"
#include "sjf_pitchShift.h"
#include <algorithm>    // std::random_erShuffle
#include <random>       // std::default_random_engine
#include <vector>
#include <time.h>

//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================

template < typename T >
class sjf_zitaRev
{
private:
    static const int NUM_REV_CHANNELS = 8;
    int m_nInChannels = 2, m_nOutChannels = 2;
    T m_dry = 0, m_wet = 1, m_FB = 0.85, m_size = 1, m_modD = 0.1;
    T m_shimLevel = 0, m_shimTranspose = 2;
    T m_lrLPFCutOff = 0.99, m_lrHPFCutOff = 0.001;
    T m_inputCutoff = 0.7;
    
    bool m_fbControl = false;
    bool m_reversePreDelay = false;
    bool m_modType = false;
    
    std::array< sjf_comb< T >, NUM_REV_CHANNELS > m_allpass;
    std::array< sjf_delayLine< T >, NUM_REV_CHANNELS > m_delays;
    std::array< sjf_reverseDelay< T >, NUM_REV_CHANNELS > m_preDelay;
    
    std::array< sjf_randOSC< T >, NUM_REV_CHANNELS > m_ERModulator, m_LRModulator; // modulators for delayTimes
    sjf_phasor< T > m_modPhasor;
    std::array< T, NUM_REV_CHANNELS > m_flip, m_revSamples, m_inSamps; // polarity flips and storage of samples
    
    sjf_pitchShift< T > shimmer; // shimmer
    sjf_lpf< T > m_shimLPF, m_shimHPF;
    
    sjf_lpf< T > m_modDSmooth, m_shimTransposeSmooth, m_FBsmooth; // dezipper variables
    std::array< sjf_lpf< T >, NUM_REV_CHANNELS > m_hiPass, m_lowPass, m_erSizeSmooth, m_lrSizeSmooth; // dezipper individual delay times
    std::array< sjf_lpf< T >, NUM_REV_CHANNELS > m_inputLPF, m_DCBlockers;
    
    
    
    std::array< T, NUM_REV_CHANNELS > m_erDelayTimesSamples, m_lrDelayTimesSamples;
    
public:
    sjf_zitaRev()
    {
        m_revSamples.fill( 0 );
    }
    ~sjf_zitaRev(){}
    
    void initialise( const int &sampleRate, const int &totalNumInputChannels, const int &totalNumOutputChannels, const int &samplesPerBlock )
    {
        m_nInChannels = totalNumInputChannels;
        m_nOutChannels = totalNumOutputChannels;
        
        setPolarityFlips( m_nInChannels );

        initialiseDelayLines( sampleRate );
        calculateDelayTimes( 1 );
        setPreDelay( 0.005 * sampleRate );
        
        initialiseModulators( sampleRate );
        
        T smoothSlewVal = calculateLPFCoefficient< float >( 1, sampleRate );
        initialiseVariableSmoothers( smoothSlewVal );
        
        initialiseShimmer( sampleRate );
        
    }
    
    void processAudio( juce::AudioBuffer<T> &buffer )
    {
        const auto bufferSize = buffer.getNumSamples();
        const T hadScale = 1 / sqrt( NUM_REV_CHANNELS );
        
        T drySamp, wetSamp, dt, modDepthSmoothed, FBSmoothed;
        
        bool modulateDelays = true;
        
        bool shimmerOn = false;
        if ( m_shimLevel > 0 ){ shimmerOn = true; }
        T shimOutput;
        const T shimWetLevel = sqrt( m_shimLevel );
        const T shimDryLevel = sqrt( 1 - m_shimLevel );
        
//        const T inScale = hadScale * 1.0f / sqrt( m_nInChannels );
//        const T inScale = sqrt( (float)m_nInChannels / (float)NUM_REV_CHANNELS );
        const T inScale = 1.0f / sqrt( (float)m_nInChannels );
        T phasorOut;
        const T phasorOffset = 1.0f / (float)( NUM_REV_CHANNELS * 2 ) ; // every delay gets a slightly different phase for sinewave modulation
        
        static const T drive = 1.01f;
        for ( int indexThroughBuffer = 0; indexThroughBuffer < bufferSize; indexThroughBuffer++ )
        {
            // first calculate smoothed global variables
            modDepthSmoothed = m_modDSmooth.filterInput( m_modD );
            if ( modDepthSmoothed <= 0 ) { modulateDelays = false; }
            else if ( !m_modType ) { modDepthSmoothed *= 0.5; }
            FBSmoothed = m_FBsmooth.filterInput( m_FB );
            
            phasorOut = m_modPhasor.output();
            
            // PreDelay First
            for ( int i = 0; i < m_nInChannels; i++ )
            {
                m_preDelay[ i ].setSample2( buffer.getSample( i , indexThroughBuffer ) ); // feed into predelay
                if ( m_reversePreDelay ) { m_inSamps[ i ] = m_preDelay[ i ].getSampleReverse( ); }
                else { m_inSamps[ i ] = m_preDelay[ i ].getSample2( ); }
                m_inputLPF[ i ].filterInPlace( m_inSamps[ i ] );
                m_inSamps[ i ] *= inScale;
            }
            

            // distribute input across rev channels
            for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
            {
                int inChan =  fastMod( i, m_nInChannels );
                m_revSamples[ i ] += ( m_inSamps[ inChan ]  * m_flip[ i ] );
            }
            
            // SHIMMER
            if ( shimmerOn )
            {
                shimmer.setSample( indexThroughBuffer, m_revSamples[  NUM_REV_CHANNELS - 1 ] ); // no need to pitchShift Everything because it all gets mixed anyway
                shimOutput = shimmer.pitchShiftOutput( indexThroughBuffer, m_shimTransposeSmooth.filterInput( m_shimTranspose ) ) ;
                shimOutput -= m_shimHPF.filterInput( shimOutput );
                m_shimLPF.filterInPlace( shimOutput );
                m_revSamples[ NUM_REV_CHANNELS - 1 ] = ( shimOutput * shimWetLevel ) + ( m_revSamples[  NUM_REV_CHANNELS - 1 ] * shimDryLevel );
            }
            


            // allpass diffussion
            for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
            {
                // calculate allpass delay times
                dt = m_erSizeSmooth[ i ].filterInput( m_erDelayTimesSamples[ i ] );
                if ( modulateDelays )
                {
                    if ( m_modType )
                    { dt += ( m_ERModulator[ i ].output() * dt * modDepthSmoothed ); }
                    else
                    {
                        dt += ( juce::dsp::FastMathApproximations::sin( (2.0f*phasorOut - 1.0f) * PI ) * dt * modDepthSmoothed );
//                        DBG( "er " << i << " " << phasorOut << " " << phasorOffset);
                        phasorOut += phasorOffset;
                        if ( phasorOut >= 1 ) { phasorOut -= 1; }
                    }
                }
                m_allpass[ i ].setDelayTimeSamps( dt );
                // then feed through allpass
                m_allpass[ i ].filterInPlace( m_revSamples[ i ] );
            }
            
            // mix
            Hadamard< T, NUM_REV_CHANNELS >::inPlace( m_revSamples.data(), hadScale );
            
            // copy to output
            for ( int i = 0; i < m_nOutChannels; i++ )
            {
                wetSamp = m_revSamples[ i ] * m_wet;
                drySamp = buffer.getSample(  fastMod2( i, m_nInChannels ) , indexThroughBuffer ) * m_dry;
                buffer.setSample( i, indexThroughBuffer, wetSamp + drySamp );
            }
            
            // LONG LASTING --> Late reflections / reverb cluster
            for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
            {
                // filter things
                // ?????
                m_lowPass[ i ].setCutoff( m_lrLPFCutOff );
                m_lowPass[ i ].filterInPlace( m_revSamples[ i ] );
                
                m_hiPass[ i ].setCutoff( m_lrHPFCutOff );
//                m_hiPass[ i ].filterInput( m_revSamples[ i ] );
                m_delays[ i ].setSample2( m_revSamples[ i ] - m_hiPass[ i ].filterInput( m_revSamples[ i ] ) ); // feed through delay lines
            }
            // save delayed values for next sample
            for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
            {
                dt = m_lrSizeSmooth[ i ].filterInput( m_lrDelayTimesSamples[ i ] );
                if ( modulateDelays )
                {
                    if ( m_modType )
                    { dt += ( dt * m_LRModulator[ i ].output() * modDepthSmoothed ); }
                    else
                    {
                        dt += ( juce::dsp::FastMathApproximations::sin( (2.0f*phasorOut - 1.0f) * PI ) * dt * modDepthSmoothed );
//                        DBG( "lr " << i << " " << phasorOut << " " << phasorOffset);
                        phasorOut += phasorOffset;
                        if ( phasorOut >= 1 ) { phasorOut -= 1; }
                    }
                }
                m_delays[ i ].setDelayTimeSamps( dt );
                T val = m_delays[ i ].getSample2();
                if ( val > 1.0f || val < -1.0f ) { DBG( "BIG VALUE!!!!" );}
                m_revSamples[ i ] = m_delays[ i ].getSample2()  * FBSmoothed ; // only modulate first channel;
                if ( m_fbControl )
                {
                    m_revSamples[ i ] = juce::dsp::FastMathApproximations::tanh( m_revSamples[ i ] * drive );
                    m_revSamples[ i ] -= m_DCBlockers[ i ].filterInput( m_revSamples[ i ] );
                }
            }
//            Householder< T, NUM_REV_CHANNELS >::mixInPlace( m_revSamples );
//            Hadamard< T, NUM_REV_CHANNELS >::inPlace( m_revSamples.data(), hadScale );
        }
    }
    
    void setSize( const T &newSize )
    {
        if ( m_size != newSize )
        {
            m_size = newSize;
            calculateDelayTimes( sjf_scale< T >( m_size, 0, 100, 0.1, 1.0 ) );
        }
    }
    
    void setModulationRate( const T& newModRate )
    {
        m_modPhasor.setFrequency( newModRate );
        
        for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
        {
            m_ERModulator[ i ].setFrequency( newModRate );
            m_LRModulator[ i ].setFrequency( newModRate );
        }
    }
    void setModulationDepth( const T &newModDepth )
    {
        m_modD = sjf_scale< float > ( newModDepth, 0, 100, -0.00001, 0.999f );
//        m_modD *= m_modD * m_modD;
    }
    
    void setModulationType( const bool& trueForRandomFalseForSin )
    {
        m_modType = trueForRandomFalseForSin;
    }
    
    void setDecay( const T &newDecay )
    {
        m_FB = sjf_scale< float > ( newDecay, 0, 100, 0, 1. );// * 0.01;
    }
    
    void setMix( const T &newMix )
    {
        m_dry = sqrt( 1 - ( newMix * 0.01 ) );
        m_wet = sqrt( newMix * 0.01 );
    }
    
    void setLrLPFCutOff( const T &newCutOff )
    {
        m_lrLPFCutOff = pow( newCutOff, 3 );
    }
    
    void setLrHPFCutOff( const T &newCutOff )
    {
        m_lrHPFCutOff = pow( newCutOff, 3 );
    }
    
    void setErCutOff( const T &newInputCutOff )
    {
        m_inputCutoff = pow( newInputCutOff, 3 );
        
        for ( int i = 0; i < m_nInChannels; i++ )
        {
            m_inputLPF[ i ].setCutoff( m_inputCutoff );
        }
    }
    
    void setShimmerLevel( const T &newShimmerLevel )
    {
        m_shimLevel = sjf_scale< T >( newShimmerLevel, 0, 100, 0, 1 );
        m_shimLevel *= m_shimLevel * m_shimLevel;
        m_shimLevel *= 0.5;
    }
    
    void setShimmerTransposition( const T &newShimmerTransposition )
    {
        m_shimTranspose = pow( 2.0f, ( newShimmerTransposition / 12.0f ) );
    }
    
    void setInterpolationType( const int &newInterpolation )
    {
        for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
        {
            m_allpass[ i ].setInterpolationType( newInterpolation );
            m_delays[ i ].setInterpolationType( newInterpolation );
        }
        shimmer.setInterpolationType( newInterpolation );
    }
    
    void setFeedbackControl( const bool& shouldLimitFeedback )
    {
        m_fbControl = shouldLimitFeedback;
    }
    
    void setPreDelay( const T& preDelayInSamps )
    {
        for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
        { m_preDelay[ i ].setDelayTimeSamps( preDelayInSamps ); }
    }
    
    void reversePredelay( const bool& trueIfReversed )
    {
        m_reversePreDelay = trueIfReversed;
    }
private:
    
    void setPolarityFlips( const int& nInChannels )
    {
        bool flipped = false;
        for ( int i = 0; i < NUM_REV_CHANNELS; i += nInChannels )
        {
            T val;
            if ( flipped ) { val = -1; }
            else { val = 1; }
            int nSteps = std::min( (int)NUM_REV_CHANNELS, nInChannels );
            for ( int j = 0; j < nSteps; j++ ) { m_flip[ i + j ] = val; }
            flipped = !flipped;
        }
    }
    
    void calculateDelayTimes( const float &proportionOfMaxSize )
    {
        T sizeFactor = proportionOfMaxSize * 0.5; // max delay vector size is double max delay time!!!
        for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
        {
            m_erDelayTimesSamples[ i ] = m_allpass[ i ].size() * sizeFactor;
            m_lrDelayTimesSamples[ i ] = m_delays[ i ].size() * sizeFactor;
        }
    }
    
    void initialiseModulators( const T& sampleRate )
    {
        m_modPhasor.initialise( sampleRate, 1 );
        for ( int i = 0 ; i < NUM_REV_CHANNELS; i++ )
        {
            m_ERModulator[ i ].initialise( sampleRate );
            m_ERModulator[ i ].setFrequency( 0.1 );
            
            m_LRModulator[ i ].initialise( sampleRate );
            m_LRModulator[ i ].setFrequency( 0.1 );
        }
    }
    
    void initialiseShimmer( const T& sampleRate )
    {
        shimmer.initialise( sampleRate, 100.0f );
        shimmer.setDelayTimeSamps( 2 );
        
        m_shimLPF.setCutoff( calculateLPFCoefficient< float >( 150000, sampleRate ) );
        m_shimHPF.setCutoff( calculateLPFCoefficient< float >( 20, sampleRate ) );
    }
    
    void initialiseDelayLines( const T& sampleRate )
    {
        T dt;
        
        const std::array< T, NUM_REV_CHANNELS > allpassT
        { 0.020346, 0.024421, 0.031604, 0.027333, 0.022904, 0.029291, 0.013458, 0.019123 };
        
        const std::array< T, NUM_REV_CHANNELS > totalDelayT
        { 0.153129, 0.210389, 0.127837, 0.256891, 0.174713, 0.192303, 0.125, 0.219991 };
        
        for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
        {
            dt = sampleRate * allpassT[ i ];
            m_allpass[ i ].initialise( 2 * round( dt ) );
            m_allpass[ i ].setCoefficients( 0.6, 1, -0.6 );
            
            dt = ( sampleRate * totalDelayT[ i ]) - dt;
            m_delays[ i ].initialise( 2 * round( dt ) );
            
            m_preDelay[ i ].initialise( sampleRate, sampleRate * 0.001);
//            m_preDelay[ i ].setRampLength( sampleRate * 0.001 );
        }
    }
    
    void initialiseVariableSmoothers( const T& smoothSlewVal )
    {
        m_shimTransposeSmooth.setCutoff( smoothSlewVal );
        
        m_modDSmooth.setCutoff( smoothSlewVal );
        m_FBsmooth.setCutoff( smoothSlewVal );
        
        for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
        {
            m_erSizeSmooth[ i ].setCutoff( smoothSlewVal );
            m_lrSizeSmooth[ i ].setCutoff( smoothSlewVal );
            
            m_DCBlockers[ i ].setCutoff( smoothSlewVal );
        }
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_zitaRev )
};



#endif /* sjf_zitaRev_h */




