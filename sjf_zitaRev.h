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
//#include "sjf_monoPitchShift.h"
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
    T m_SR = 44100;
    T m_shimLevel = 0, m_shimTranspose = 2;
    T m_LRCutOff = 0.7;
    T m_inputCutoff = 0.7;
    
    bool m_fbControl = false;
    
    std::array< T, NUM_REV_CHANNELS > m_outSamps;
    std::array< T, NUM_REV_CHANNELS > m_inSamps;
    std::array< sjf_comb< T >, NUM_REV_CHANNELS > m_allpass;
    std::array< sjf_delayLine< T >, NUM_REV_CHANNELS > m_delays;
    std::array< sjf_reverseDelay< T >, NUM_REV_CHANNELS > m_preDelay;
    
    std::array< sjf_randOSC< T >, NUM_REV_CHANNELS > m_ERModulator, m_LRModulator; // modulators for delayTimes
    
    std::array< T, NUM_REV_CHANNELS > m_flip, m_revSamples; // polarity flips and storage of samples
    
    sjf_pitchShift< T > shimmer; // shimmer
    sjf_lpf< T > m_shimLPF, m_shimHPF;
    
    sjf_lpf< T > m_modDSmooth, m_shimTransposeSmooth, m_FBsmooth; // dezipper variables
    std::array< sjf_lpf< T >, NUM_REV_CHANNELS > m_lowPass, m_erSizeSmooth, m_lrSizeSmooth; // dezipper individual delay times
//    std::vector< std::unique_ptr< sjf_lpf< T > > > m_inputLPF;
    std::array< sjf_lpf< T >, NUM_REV_CHANNELS > m_inputLPF;
    
    
    const std::array< T, NUM_REV_CHANNELS > m_allpassT
    {
        0.020346, 0.024421, 0.031604, 0.027333, 0.022904, 0.029291, 0.013458, 0.019123
    };
    
    const std::array< T, NUM_REV_CHANNELS > m_totalDelayT
    {
        0.153129, 0.210389, 0.127837, 0.256891, 0.174713, 0.192303, 0.125, 0.219991
    };
    
    std::array< T, NUM_REV_CHANNELS > m_erDelayTimes, m_lrDelayTimesSamples;
    
public:
    sjf_zitaRev()
    {
        m_revSamples.fill( 0 );
    }
    ~sjf_zitaRev(){}
    
    void initialise( const int &sampleRate, const int &totalNumInputChannels, const int &totalNumOutputChannels, const int &samplesPerBlock )
    {
        m_SR = sampleRate;
        m_nInChannels = totalNumInputChannels;
        m_nOutChannels = totalNumOutputChannels;
        
        setPolarityFlips( m_nInChannels );

//        for ( int i = m_inputLPF.size(); i < m_nInChannels; i++ )
//        {
//            auto filt = std::make_unique< sjf_lpf< float > >();
//            m_inputLPF.push_back( std::move( filt ) );
//            m_inputLPF[ i ]->setCutoff( 0.99 );
//        }

        for( int i = 0; i < NUM_REV_CHANNELS; i++ )
        {
            m_inputLPF[ i ].setCutoff( 0.99 );
        }

//        for ( int i = m_preDelay.size(); i < m_nInChannels; i++ )
//        {
//            auto del = std::make_unique< m_preDelay< float > >();
//            m_inputLPF.push_back( std::move( del ) );
//            m_preDelay.initialise( 0.02 * m_SR );
//        }
        for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
        {
            m_preDelay[ i ].initialise( m_SR );
            m_preDelay[ i ].setDelayTimeSamps( 0.1 * m_SR );
        }
//
//        m_outSamps.resize( m_nOutChannels );
        DBG("samplerate " << sampleRate );
        for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
        {
            T dt = m_SR * m_allpassT[ i ];
            m_allpass[ i ].initialise( 2 * round( dt ) );
            m_allpass[ i ].setCoefficients( 0.6, 1, -0.6 );
            
            dt = ( m_SR * m_totalDelayT[ i ]) - dt;
            m_delays[ i ].initialise( 2 * round( dt ) );
            
            m_lowPass[ i ].setCutoff( 0.7 );
        }
        calculateDelayTimes( 1 );
        
        for ( int i = 0 ; i < NUM_REV_CHANNELS; i++ )
        {
            m_ERModulator[ i ].initialise( m_SR );
            m_ERModulator[ i ].setFrequency( 0.1 );
            
            m_LRModulator[ i ].initialise( m_SR );
            m_LRModulator[ i ].setFrequency( 0.1 );
            
            m_erSizeSmooth[ i ].setCutoff( 0.001 );
            m_lrSizeSmooth[ i ].setCutoff( 0.001 );
        }
        
        shimmer.initialise( m_SR, 100.0f );
        shimmer.setDelayTimeSamps( 2 );
        m_shimTransposeSmooth.setCutoff( 0.001 );
        
        m_modDSmooth.setCutoff( 0.001 );
        m_FBsmooth.setCutoff( 0.001 );
        
        m_shimLPF.setCutoff( 0.99 );
        m_shimHPF.setCutoff( 0.01 );
    }
    
    void processAudio( juce::AudioBuffer<T> &buffer )
    {
//        std::array< T, m_nInChannels > inSamps;
//        std::array< T, 8 > inSamps;
        auto bufferSize = buffer.getNumSamples();
        T hadScale = 1 / sqrt( NUM_REV_CHANNELS );
        
        T drySamp, wetSamp, dt, modDepthSmoothed, FBSmoothed;
        
        bool reversePreDelay = false;
        bool shimmerOn = false;
        if ( m_shimLevel > 0 ){ shimmerOn = true; }
        T shimOutput;
        T shimWetLevel = sqrt( m_shimLevel );
        T shimDryLevel = sqrt( 1 - m_shimLevel );
        
        T inScale = 1/ sqrt( m_nInChannels );
        
//        T inSamp;
        for ( int i = 0; i < m_nInChannels; i++ )
        {
            m_inputLPF[ i ].setCutoff( m_inputCutoff );
        }
        
        for ( int indexThroughBuffer = 0; indexThroughBuffer < bufferSize; indexThroughBuffer++ )
        {
            // first calculate smoothed global variables
            modDepthSmoothed = m_modDSmooth.filterInput( m_modD );
            FBSmoothed = m_FBsmooth.filterInput( m_FB );
            
            // PreDelay First
            for ( int i = 0; i < m_nInChannels; i++ )
            {
                m_preDelay[ i ].setSample2( buffer.getSample( i , indexThroughBuffer ) );
                if ( reversePreDelay )
                {
                    m_inSamps[ i ] = m_preDelay[ i ].getSampleReverse( );
                }
                else
                {
                    m_inSamps[ i ] = m_preDelay[ i ].getSample2( );
                }
                m_inSamps[ i ] *= inScale;
//                m_inSamps[ i ] = buffer.getSample( i , indexThroughBuffer ) * inScale;
            }
            

            // SHIMMER
            if ( shimmerOn )
            {
                shimmer.setSample( indexThroughBuffer, m_revSamples[ 0 ] ); // no need to pitchShift Everything because it all gets mixed anyway
                shimOutput = shimmer.pitchShiftOutput( indexThroughBuffer, m_shimTransposeSmooth.filterInput( m_shimTranspose ) ) ;
                shimOutput -= m_shimHPF.filterInput( shimOutput );
                m_shimLPF.filterInPlace( shimOutput );
                m_revSamples[ 0 ] = ( shimOutput * shimWetLevel ) +  ( m_revSamples[ 0 ] * shimDryLevel );
            }
            

            // distribute input across rev channels
            for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
            {
                int inChan =  fastMod( i, m_nInChannels );
                m_revSamples[ i ] += (  m_inputLPF[ inChan ].filterInput( m_inSamps[ inChan ] ) * m_flip[ i ] );
            }
            // allpass diffussion
            for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
            {
                // calculate allpass delay times
                dt = m_erSizeSmooth[ i ].filterInput( m_erDelayTimes[ i ] );
                dt += ( m_ERModulator[ i ].output() * dt * modDepthSmoothed );
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
                drySamp = buffer.getSample( ( fastMod2( i, m_nInChannels ) ) , indexThroughBuffer ) * m_dry;
                buffer.setSample( i, indexThroughBuffer, wetSamp + drySamp );
            }
            
            // LONG LASTING --> Late reflections / reverb cluster
            for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
            {
                // filter things
                // ?????
                m_lowPass[ i ].setCutoff( m_LRCutOff );
                m_lowPass[ i ].filterInPlace( m_revSamples[ i ] );
                m_delays[ i ].setSample2( m_revSamples[ i ] ); // feed through delay lines
            }
            // save delayed values for next sample
            for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
            {
                dt = m_lrSizeSmooth[ i ].filterInput( m_lrDelayTimesSamples[ i ] );
                dt += ( dt * m_LRModulator[ i ].output() * modDepthSmoothed );
//                DBG("lr " << i << " dt " << dt);
                m_delays[ i ].setDelayTimeSamps( dt );
                m_revSamples[ i ] = m_delays[ i ].getSample2()  * FBSmoothed ; // only modulate first channel;
                if ( m_fbControl )
                {
                    m_revSamples[ i ] = tanh( m_revSamples[ i ] );
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
    
    void setModulation( const T &newModDepth )
    {
        m_modD = newModDepth * 0.01;
    }
    
    void setDecay( const T &newDecay )
    {
        m_FB = sjf_scale< float > ( newDecay, 0, 100, 0, 1 );// * 0.01;
    }
    
    void setMix( const T &newMix )
    {
        m_dry = sqrt( 1 - ( newMix * 0.01 ) );
        m_wet = sqrt( newMix * 0.01 );
    }
    
    void setLrCutOff( const T &newLRCutOff )
    {
        m_LRCutOff = pow( newLRCutOff, 3 );
    }
    
    void setErCutOff( const T &newInputCutOff )
    {
        m_inputCutoff = pow( newInputCutOff, 3 );
    }
    
    void setShimmer( const T &newShimmerLevel, const T &newShimmerTransposition )
    {
        m_shimLevel = sjf_scale< T >( newShimmerLevel, 0, 100, 0, 1 );
        m_shimLevel *= m_shimLevel;
        m_shimLevel *= 0.5;
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
        for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
        {
            DBG( "flipp " << i << " " << m_flip[ i ] );
        }
    }
    
    void calculateDelayTimes( const float &proportionOfMaxSize )
    {
        T sizeFactor = proportionOfMaxSize * 0.5; // max delay vector size is double max delay time!!!
        for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
        {
            m_erDelayTimes[ i ] = m_allpass[ i ].size() * sizeFactor;
            m_lrDelayTimesSamples[ i ] = m_delays[ i ].size() * sizeFactor;
        }
        
    }
    
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_zitaRev )
};



#endif /* sjf_zitaRev_h */




