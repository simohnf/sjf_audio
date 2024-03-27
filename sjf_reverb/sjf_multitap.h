#ifndef sjf_rev_multitap_h
#define sjf_rev_multitap_h

#include "../sjf_audioUtilitiesC++.h"
#include "../sjf_interpolators.h"
#include "../gcem/include/gcem.hpp"





namespace sjf::rev
{
    template < typename T, int MAXNTAPS >
    class multiTap
    {
    public:
        multiTap(){}
        ~multiTap(){}
        
        void initialise( T sampleRate )
        {
            auto delSize = sjf_nearestPowerAbove( sampleRate / 2, 2 );
            m_delay.initialise( delSize );
        }
        
        void setDelayTimes( const std::array< T, MAXNTAPS >& dt )
        {
            for ( auto d = 0; d < MAXNTAPS; d++ )
                m_delayTimes[ d ] = dt[ d ];
        }
        
        void setGains( const std::array< T, MAXNTAPS >& gains )
        {
            for ( auto  g = 0; g < MAXNTAPS; g++ )
                m_gains[ g ] = gains[ g ];
        }
        
        void setNTaps( int nTaps )
        {
            m_nTaps = nTaps;
        }
        
        T process( T x )
        {
            T output = 0.0;
            for ( auto t = 0; t < m_nTaps; t++ )
                output += m_delay.getSample( m_delayTimes[ t ], 0 ) * m_gains[ t ];
            m_delay.setSample( x );
            return output;
        }
    private:
        std::array< T, MAXNTAPS > m_delayTimes, m_gains;
        int m_nTaps = MAXNTAPS;
        delay< T > m_delay;
    };
}


#endif /* sjf_rev_multitap_h */
