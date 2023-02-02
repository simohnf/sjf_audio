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
    T m_dry = 0, m_wet = 1, m_FB = 0.85, m_size = 1, m_erSize, m_modD = 0.1;
    
//    std::vector< T > m_outSamps;
//    std::vector< T > m_inSamps;
    std::array< sjf_comb< T >, NUM_REV_CHANNELS > m_allpass;
    std::array< sjf_delayLine< T >, NUM_REV_CHANNELS > m_delays;
    std::array< T, NUM_REV_CHANNELS > m_flip, m_revSamples;
    std::array< sjf_lpf< T >, NUM_REV_CHANNELS > m_lowPass;
    const std::array< T, NUM_REV_CHANNELS > m_allpassT
    {
        0.020346, 0.024421, 0.031604, 0.027333, 0.022904, 0.029291, 0.013458, 0.019123
    };
    
    const std::array< T, NUM_REV_CHANNELS > m_totalDelayT
    {
        0.153129, 0.210389, 0.127837, 0.256891, 0.174713, 0.192303, 0.125, 0.219991
    };
    
    std::array< T, NUM_REV_CHANNELS > m_erDelayTimes, m_lrDelayTimesSamples;
    
    sjf_randOSC< T > m_modulator;
public:
    sjf_zitaRev()
    {
        m_revSamples.fill( 0 );
        for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
        {
            if ( rand01() < 0.5 ) { m_flip[ i ] = -1.0; }
            else { m_flip[ i ] = 1.0; }
        }
    }
    ~sjf_zitaRev(){}
    
    void initialise( const int &sampleRate, const int &totalNumInputChannels, const int &totalNumOutputChannels, const int &samplesPerBlock )
    {
        m_nInChannels = totalNumInputChannels;
        m_nOutChannels = totalNumOutputChannels;
//        m_outSamps.resize( m_nOutChannels );
//        m_inSamps.resize( m_nInChannels );
        DBG("samplerate " << sampleRate );
        for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
        {
            int dt = sampleRate * m_allpassT[ i ] + 0.5;
            m_allpass[ i ].initialise( round ( dt * 1.5 ) );
            m_allpass[ i ].setDelayTimeSamps( dt );
            m_allpass[ i ].setCoefficients( 0.6, 1, -0.6 );
            m_erDelayTimes[ i ] = dt;
            DBG("er " << i << " dt " << dt << " allpassT " << m_allpassT[ i ] << " m_erDelayTimes " << m_erDelayTimes[ i ] );
            
            dt = ( sampleRate * m_totalDelayT[ i ] + 0.5 ) - dt;
            m_delays[ i ].initialise( round( dt * 1.5 ) );
            m_delays[ i ].setDelayTimeSamps( dt );
            m_lrDelayTimesSamples[ i ] = dt;
            
            m_lowPass[ i ].setCutoff( 0.7 );
        }
        
        m_modulator.initialise( sampleRate );
        m_modulator.setFrequency( 0.1 );
    }
    
    void processAudio( juce::AudioBuffer<T> &buffer )
    {
        auto bufferSize = buffer.getNumSamples();
        T hadScale = 1 / sqrt( NUM_REV_CHANNELS );
        
        T drySamp, wetSamp, dt;
        for ( int indexThroughBuffer = 0; indexThroughBuffer < bufferSize; indexThroughBuffer++ )
        {
            for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
            {
                // first distribute input across rev channels
                m_revSamples[ i ] += buffer.getSample( ( i % m_nInChannels ) , indexThroughBuffer ) * m_flip[ i ];
                // then feed through allpass
                dt = m_erDelayTimes[ i ] * m_size;
                DBG("er " << i << " dt " << dt);
                m_allpass[ i ].setDelayTimeSamps( dt );
                m_revSamples[ i ] = m_allpass[ i ].filterInputRounded( m_revSamples[ i ] );
            }
            // mix
            Hadamard< T, NUM_REV_CHANNELS >::inPlace( m_revSamples.data(), hadScale );
            // copy to output
            for ( int i = 0; i < m_nOutChannels; i++ )
            {
//                m_outSamps[ i ] = m_revSamples[ i ];
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
            
            dt = m_lrDelayTimesSamples[ 0 ] * m_size;
            dt += dt + ( m_modulator.output() * dt * m_modD );
            DBG("lr " << 0 << " dt " << dt);
            m_delays[ 0 ].setDelayTimeSamps( dt );
            m_revSamples[ 0 ] = m_delays[ 0 ].getSample2()  * m_FB ; // only modulate first channel;
            
            for ( int i = 1; i < NUM_REV_CHANNELS; i++ )
            {
                dt = m_lrDelayTimesSamples[ i ] * m_size;
                DBG("lr " << i << " dt " << dt);
                m_delays[ i ].setDelayTimeSamps( dt );
                m_revSamples[ i ] = m_delays[ i ].getSampleRoundedIndex2()  * m_FB ; // only modulate first channel;
            }
            
            
        }
    }
    
    void setSize( const T &newSize )
    {
        
        m_size = ( newSize * 0.01 ); // convert to 0 --> 1
        m_size = m_size * 0.9 + 0.1;
        m_size *= m_size;
//        m_erSize = ( m_size * 0.5 ) + 0.5;
        DBG( "Size " << newSize << " " << m_size << " " << m_erSize );
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
    }
    void setFeedbackControl( const T &newFeedback )
    {
    }
private:
    
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_zitaRev )
};



#endif /* sjf_zitaRev_h */




