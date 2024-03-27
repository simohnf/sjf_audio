//
//  sjf_fdn.h
//
//  Created by Simon Fay on 27/03/2024.
//

#ifndef sjf_rev_fdn_h
#define sjf_rev_fdn_h

#include "../sjf_audioUtilitiesC++.h"
#include "../sjf_interpolators.h"
#include "../gcem/include/gcem.hpp"

#include "sjf_delay.h"

namespace sjf::rev
{
    template< typename T, int NCHANNELS >
    class fdn
    {
    public:
        fdn()
        {
            std::array< T, NCHANNELS > rDT;
            for ( auto & d : rDT )
                d = m_SR * ( 0.1 + ( rand01() * 0.4 ) ); // just to ensure dts are set to something!
            setDelayTimes( rDT );
        }
        ~fdn(){}
        
        void initialise( int maxSizePerChannelSamps, T sampleRate )
        {
            m_SR = sampleRate;
            for ( auto & d : m_delays )
                d.initialise( sjf_nearestPowerAbove( maxSizePerChannelSamps, 2 ) );
        }
        
        void setDelayTimes( const std::array< T, NCHANNELS >& dt )
        {
            for ( auto c = 0; c < NCHANNELS; c ++ )
                m_delayTimesSamps[ c ] = dt[ c ];
            setDecayInMS( m_decayInMS );
        }
        
        void setDecayInMS( T decayInMS )
        {
            m_decayInMS = decayInMS;
            auto dt = 0.0;
            for ( auto c = 0; c < NCHANNELS; c++ )
            {
                dt = m_delayTimesSamps[ c ] * m_SR * 0.001;
                m_fbGains[ c ] = calculateFeedbackGain< T >( dt, m_decayInMS );
            }
        }
        
        void processInPlace( std::array< T, NCHANNELS  >& samples, int mixType = mixers::hadamard, int interpType = 1 )
        {
            std::array< T, NCHANNELS  > delayed;
            for ( auto c = 0; c < NCHANNELS; c++ )
                delayed[ c ] = m_delays[ c ].getSample( m_delayTimesSamps[ c ], interpType );
            
            switch( mixType )
            {
                case mixers::none :
                    break;
                case mixers::hadamard :
                    Hadamard< T, NCHANNELS >::inPlace( delayed );
                case mixers::householder :
                    Householder< T, NCHANNELS >::mixInPlace( delayed );
            }
            // need to finish processing
            // add filtering??
            // feed delayed and input back into delay line
            // output delayed
            // possibly include allpass in loop
        }
        
        
        enum mixers
        {
            none, hadamard, householder
        };
    private:
        std::array< delay< T >, NCHANNELS > m_delays;
        std::array< T, NCHANNELS > m_delayTimesSamps, m_fbGains;
        T m_decayInMS = 1000, m_SR = 44100;
    };
}

#endif /* sjf_rev_fdn_h */
