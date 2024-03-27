//
//  sjf_apLoop.h
//
//  Created by Simon Fay on 27/03/2024.
//


#ifndef sjf_rev_apLoop_h
#define sjf_rev_apLoop_h

#include "../sjf_audioUtilitiesC++.h"
#include "../sjf_interpolators.h"
#include "../gcem/include/gcem.hpp"
#include "sjf_oneMultAP.h"
#include "sjf_delay.h"
#include "sjf_damper.h"

namespace sjf::rev
{
    template < typename T, int NSTAGES, int AP_PERSTAGE >
    class allpassLoop
    {
    public:
        allpassLoop()
        {
            //            fillPrimes();
            // just ensure that delaytimes are set to begin with
            for ( auto s = 0; s < NSTAGES; s++ )
                for ( auto d = 0; d < AP_PERSTAGE+1; d++ )
                    setDelayTimeSamples( std::round(rand01() * 4410 + 4410), s, d );
            setDecay( 5000 );
            setDiffusion( 0.5 );
        }
        ~allpassLoop(){}
        
        void initialise( T sampleRate )
        {
            m_SR = sampleRate;
            auto delSize = sjf_nearestPowerAbove( m_SR / 2, 2 );
            for ( auto s = 0; s < NSTAGES; s++ )
            {
                for ( auto a = 0; a < AP_PERSTAGE; a++ )
                    m_aps[ s ][ a ].initialise( delSize );
                m_delays[ s ].initialise( delSize );
            }
        }
        
        void setDelayTimesSamples( std::array< std::array < T, AP_PERSTAGE + 1 >, NSTAGES > delayTimes)
        {
            for ( auto s = 0; s < NSTAGES; s++ )
                for ( auto d = 0; d < AP_PERSTAGE+1; d++ )
                    m_delayTimes[ s ][ d ] = delayTimes[ d ][ s ];
            setDecay( m_decayInMS );
        }
        
        void setDiffusion( T diff )
        {
            diff = diff <= 0.9 ? (diff >= -0.9 ? diff : -0.9 ) : 0.9;
            for ( auto s = 0; s < NSTAGES; s++ )
                for ( auto d = 0; d < AP_PERSTAGE; d++ )
                    m_diffusions[ s ][ d ] = diff;
        }
        
        void setDelayTimeSamples( T dt, int stage, int delayNumber )
        {
            m_delayTimes[ stage ][ delayNumber ] = dt;
            setDecay( m_decayInMS );
        }
        
        void setDecay( T decayInMS )
        {
            m_decayInMS = decayInMS;
            for ( auto s = 0; s < NSTAGES; s++ )
            {
                auto del = 0.0;
                for ( auto d = 0; d < AP_PERSTAGE + 1; d++ )
                    del += m_delayTimes[ s ][ d ];
                del  /= ( m_SR * 0.001 );
                m_gain[ s ] = std::pow( 10.0, -3.0 * del / m_decayInMS );
            }
        }
        
        void setDamping( T dampCoef )
        {
            m_damping = dampCoef < 1 ? (dampCoef > 0 ? dampCoef : 00001) : 0.9999;
        }
        
        
        T process( T input )
        {
            auto samp = m_lastSamp;
            auto output = 0.0;
            for ( auto s = 0; s < NSTAGES; s ++ )
            {
                samp +=  input;
                for ( auto a = 0; a < AP_PERSTAGE; a++ )
                {
                    samp = m_aps[ s ][ a ].process( samp, m_delayTimes[ s ][ a ], m_diffusions[ s ][ a ] );
                }
                samp = m_dampers[ s ].process( samp, m_damping );
                output += samp;
                m_delays[ s ].setSample( samp * m_gain[ s ] );
                samp = m_delays[ s ].getSample( m_delayTimes[ s ][ AP_PERSTAGE ] );
            }
            m_lastSamp = samp;
            return output;
        }
        
    private:
        //
        //        void fillPrimes()
        //        {
        //            static constexpr auto MAXNPRIMES = 10000;
        //            m_primes.reserve( MAXNPRIMES );
        //            for ( auto i = 2; i < MAXNPRIMES; i++ )
        //                if( sjf_isPrime( i ) )
        //                    m_primes.emplace_back( i );
        //            m_primes.shrink_to_fit();
        //        }
        
        std::array< T, NSTAGES > m_gain;
        std::array< std::array< T, AP_PERSTAGE + 1 >, NSTAGES > m_delayTimes;
        std::array< std::array< T, AP_PERSTAGE >, NSTAGES > m_diffusions;
        
        
        std::array< std::array< oneMultAP < T >, AP_PERSTAGE >, NSTAGES > m_aps;
        std::array< delay < T >, NSTAGES > m_delays;
        std::array< damper < T >, NSTAGES > m_dampers;
        
        std::vector< int > m_primes;
        
        T m_lastSamp = 0;
        T m_SR = 44100, m_decayInMS = 100;
        T m_damping = 0.999;
    };
}


#endif /* sjf_rev_apLoop_h */
