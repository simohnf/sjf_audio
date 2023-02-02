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
    using NUM_REV_CHANNELS = 8;
    std::array< sjf_comb< T > NUM_REV_CHANNELS > m_allpass;
    sjf_multiDelay< T, NUM_REV_CHANNELS > m_delays;

    int m_nInChannels, m_nOutChannels;
    
    std::vector< T > m_outSamps;
    std::array< T, NUM_REV_CHANNELS > m_flip;
    std::array< T, NUM_REV_CHANNELS > m_revSamples;
    std::array< T, NUM_REV_CHANNELS > m_allpassT
    {
        20346e-6f, 24421e-6f, 31604e-6f, 27333e-6f, 22904e-6f, 29291e-6f, 13458e-6f, 19123e-6f
    };
    
    std::array< T, NUM_REV_CHANNELS > m_totalDelayT
    {
        153129e-6f, 210389e-6f, 127837e-6f, 256891e-6f, 174713e-6f, 192303e-6f, 125000e-6f, 219991e-6f
    };
    
public:
    sjf_zitaRev()
    {
        m_revSamples.fill( 0 );
        for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
        {
            if ( rand01() < 0.5 ) { m_flip[ i ] = -1; }
            else { m_flip[ i ] = 1; }
        }
    }
    ~sjf_zitaRev(){}
    
    void initialise( const int &sampleRate, const int &totalNumInputChannels, const int &totalNumOutputChannels, const int &samplesPerBlock )
    {
        m_nInChannels = totalNumInputChannels;
        m_nOutChannels = totalNumOutputChannels;
        m_outSamps.resize( m_nOutChannels );
        for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
        {
            T dt = sampleRate * m_allpassT[ i ];
            m_allpass[ i ].initialise( dt * 2 );
            m_allpass[ i ].setDelayTimeSamps( dt );
            m_allpass[ i ].setCoefficients( 0.6, 1, -0.6 );
            
            dt = ( sampleRate * m_totalDelayT[ i ] ) - dt;
            m_delays[ i ].initialise( dt * 2 );
            m_delays[ i ].setDelayTimeSamps( dt );
        }
    }
    
    processAudio( juce::AudioBuffer<T> &buffer )
    {
        auto bufferSize = buffer.getNumSamples();
        T hadScale = 1 / sqrt( NUM_REV_CHANNELS );
        for ( int indexThroughBuffer = 0; indexThroughBuffer < bufferSize; indexThroughBuffer++ )
        {
            
            for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
            {
                // first distribute input across rev channels
                m_revSamples[ i ] += buffer.getSample( ( i % m_nInChannels ) , indexThroughBuffer ) * m_flip[ i ];
                // then feed into allpass
                m_revSamples[ i ] = m_allpass[ i ].filterInputRounded( m_revSamples[ i ] );
            }
            Hadamard< T, NUM_REV_CHANNELS >::inPlace( m_revSamples.data(), hadScale );
            for ( int i = 0; i < m_nOutChannelsl i++ )
            {
                m_outSamps[ i ] = m_revSamples[ i ];
            }
            for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
            {
                
            }
        }
    }
private:
    void distribute( )
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_zitaRev )
};



#endif /* sjf_zitaRev_h */




