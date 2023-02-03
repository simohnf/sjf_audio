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
    int m_nInChannels, m_nOutChannels;
    T m_dry = 0, m_wet = 1, m_FB = 0.85, m_size = 1, m_modD = 0.1;
    T m_SR = 44100;
    std::vector< T > m_outSamps;
//    std::vector< T > m_inSamps;
    std::array< sjf_comb< T >, NUM_REV_CHANNELS > m_allpass;
    std::array< sjf_delayLine< T >, NUM_REV_CHANNELS > m_delays;
    std::array< T, NUM_REV_CHANNELS > m_flip, m_revSamples;
    std::array< sjf_lpf< T >, NUM_REV_CHANNELS > m_lowPass, m_erSizeSmooth, m_lrSizeSmooth;
    
    sjf_lpf< T > m_modDSmooth;
    
    const std::array< T, NUM_REV_CHANNELS > m_allpassT
    {
        0.020346, 0.024421, 0.031604, 0.027333, 0.022904, 0.029291, 0.013458, 0.019123
    };
    
    const std::array< T, NUM_REV_CHANNELS > m_totalDelayT
    {
        0.153129, 0.210389, 0.127837, 0.256891, 0.174713, 0.192303, 0.125, 0.219991
    };
    
    std::array< T, NUM_REV_CHANNELS > m_erDelayTimes, m_lrDelayTimesSamples;
    
    std::array< sjf_randOSC< T >, NUM_REV_CHANNELS > m_ERModulator, m_LRModulator;
public:
    sjf_zitaRev()
    {
        m_revSamples.fill( 0 );
        bool flipped = false;
        for ( int i = 0; i < NUM_REV_CHANNELS; i += 2 )
        {
            T val;
            if ( flipped ) { val = -1; }
            else { val = 1; }
            m_flip[ i ] = val;
            m_flip[ i + 1 ] = val;
            flipped = !flipped;
        }
        for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
        {
            DBG( "flipp " << i << " " << m_flip[ i ] );
        }
        
    }
    ~sjf_zitaRev(){}
    
    void initialise( const int &sampleRate, const int &totalNumInputChannels, const int &totalNumOutputChannels, const int &samplesPerBlock )
    {
        m_SR = sampleRate;
        m_nInChannels = totalNumInputChannels;
        m_nOutChannels = totalNumOutputChannels;
        m_outSamps.resize( m_nOutChannels );
//        m_inSamps.resize( m_nInChannels );
        DBG("samplerate " << sampleRate );
        for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
        {
            T dt = m_SR * m_allpassT[ i ];
            m_allpass[ i ].initialise( 2 * round( dt ) );
            m_allpass[ i ].setCoefficients( 0.6, 1, -0.6 );
            
            dt = ( m_SR * m_totalDelayT[ i ]) - dt;
            m_delays[ i ].initialise( 2 * round( dt ) );
            
            m_lowPass[ i ].setCutoff( 1 );
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
        
        m_modDSmooth.setCutoff( 0.001 );
    }
    
    void processAudio( juce::AudioBuffer<T> &buffer )
    {
        auto bufferSize = buffer.getNumSamples();
        T hadScale = 1 / sqrt( NUM_REV_CHANNELS );
        
        T drySamp, wetSamp, dt, modDepth;
        for ( int indexThroughBuffer = 0; indexThroughBuffer < bufferSize; indexThroughBuffer++ )
        {
            modDepth = m_modDSmooth.filterInput( m_modD );
            
            for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
            {
                // first distribute input across rev channels
                m_revSamples[ i ] += buffer.getSample( ( i % m_nInChannels ) , indexThroughBuffer ) * m_flip[ i ];
                // then feed through allpass
                dt = m_erSizeSmooth[ i ].filterInput( m_erDelayTimes[ i ] );
                dt += ( m_ERModulator[ i ].output() * dt * modDepth );
//                DBG("er " << i << " dt " << dt);
                m_allpass[ i ].setDelayTimeSamps( dt );
//                m_revSamples[ i ] = m_allpass[ i ].filterInputRoundedIndex( m_revSamples[ i ] );
                m_revSamples[ i ] = m_allpass[ i ].filterInput( m_revSamples[ i ] );
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
            for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
            {
                // filter things
                // ?????
                m_lowPass[ i ].filterInput( m_revSamples[ i ] );
                // feed through delay lines
                // store last values
                m_delays[ i ].setSample2( m_revSamples[ i ] );
            }
            // save delayed values for next sample

            for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
            {
                dt = m_lrSizeSmooth[ i ].filterInput( m_lrDelayTimesSamples[ i ] );
                dt += ( dt * m_LRModulator[ i ].output() * modDepth );
//                DBG("lr " << i << " dt " << dt);
                m_delays[ i ].setDelayTimeSamps( dt );
                m_revSamples[ i ] = m_delays[ i ].getSample2()  * m_FB ; // only modulate first channel;
            }
//            Householder< T, NUM_REV_CHANNELS >::mixInPlace( m_revSamples );
//            Hadamard< T, NUM_REV_CHANNELS >::inPlace( m_revSamples.data(), hadScale );
        }
    }
    
    void setSize( const T &newSize )
    {
        
        m_size = newSize;//( newSize * 0.01 ); // convert to 0 --> 1
//        m_size *= m_size;
        T size = m_size;
        size *= 0.01;
        size *= 0.9;
        size += 0.1;
        size *= 0.5;
//        m_size = m_size * 0.9 + 0.1; // size of delaylines is 2 * max delay
        
        calculateDelayTimes( size );
//        m_erSize = ( m_size * 0.5 ) + 0.5;
        DBG( "Size " << newSize << " " << m_size );
        
    }
    void setModulation( const T &newModDepth )
    {
        m_modD = newModDepth * 0.01;
    }
    void setDecay( const T &newDecay )
    {
        m_FB = newDecay * 0.01;
    }
    void setMix( const T &newMix )
    {
    }
    void setLrCutOff( const T &newLRCutOff )
    {
    }
    void setErCutOff( const T &newERCutOff )
    {
    }
    void setShimmer( const T &newShimmerLevel, const T &newShimmerTranspsition )
    {
    }
    void setInterpolationType( const int &newInterpolation )
    {
        for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
        {
            m_allpass[ i ].setInterpolationType( newInterpolation );
            m_delays[ i ].setInterpolationType( newInterpolation );
        }
    }
    void setFeedbackControl( const T &newFeedback )
    {
    }
private:
    
    void calculateDelayTimes( const float &proportionOfMaxSize )
    {
        for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
        {
//            T dt = m_SR * m_allpassT[ i ];
//            m_erDelayTimes[ i ] = dt * proportionOfMaxSize;
            m_erDelayTimes[ i ] = m_allpass[ i ].size() * proportionOfMaxSize;
//            DBG("er " << i << " dt " << dt << " allpassT " << m_allpassT[ i ] << " m_erDelayTimes " << m_erDelayTimes[ i ] );
//            DBG("er " << i << " dt " << " m_erDelayTimes " << m_erDelayTimes[ i ] );

//            dt = ( m_SR * m_totalDelayT[ i ] ) - dt;
//            m_lrDelayTimesSamples[ i ] = dt * proportionOfMaxSize;
            m_lrDelayTimesSamples[ i ] = m_delays[ i ].size() * proportionOfMaxSize;
        }
        
    }
    
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_zitaRev )
};



#endif /* sjf_zitaRev_h */




