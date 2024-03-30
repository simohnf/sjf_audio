//
//  sjf_multitap.h
//
//  Created by Simon Fay on 27/03/2024.
//


#ifndef sjf_rev_multitap_h
#define sjf_rev_multitap_h

#include "../sjf_audioUtilitiesC++.h"
#include "../sjf_interpolators.h"
#include "../gcem/include/gcem.hpp"
#include "sjf_delay.h"





namespace sjf::rev
{
    /**
     Basic multitap delayLine for use primarily as part of an early reflection generator
     No filtering or feedback is applied
     */
    template < typename T, int MAXNTAPS >
    class multiTap
    {
    private:
        std::array< T, MAXNTAPS > m_delayTimes, m_gains;
        int m_nTaps = MAXNTAPS;
        delay< T > m_delay;
    public:
        multiTap(){}
        ~multiTap(){}
        
        
        /**
         This must be called before first use in order to set basic information such as maximum delay lengths and sample rate
         */
        void initialise( T sampleRate )
        {
            auto delSize = sjf_nearestPowerAbove( sampleRate / 2, 2 );
            m_delay.initialise( delSize );
        }
        
        /**
         Set all of the delayTimes
         */
        void setDelayTimes( const std::array< T, MAXNTAPS >& dt )
        {
            for ( auto d = 0; d < MAXNTAPS; d++ )
                m_delayTimes[ d ] = dt[ d ];
        }
        
        /**
         Set all of the delayTimes
         */
        void setGains( const std::array< T, MAXNTAPS >& gains )
        {
            for ( auto  g = 0; g < MAXNTAPS; g++ )
                m_gains[ g ] = gains[ g ];
        }
        
        /**
         Using this you can change the number of active taps
         */
        void setNTaps( int nTaps )
        {
            m_nTaps = nTaps < 1 ? 1 : ( nTaps > MAXNTAPS ? MAXNTAPS : nTaps );
        }
        
        
        /**
         This processone sample
         Input:
            sample to be inserted into the delayLine
         Output:
            combination of all of the taps
         */
        T process( T x, int interpType = 0 )
        {
            T output = 0.0;
            for ( auto t = 0; t < m_nTaps; t++ )
                output += getSample( t, interpType );
//                output += m_delay.getSample( m_delayTimes[ t ], interpType ) * m_gains[ t ];
            m_delay.setSample( x );
            return output;
        }
        
        /**
         Using this you can get the output of a single tap at a time
         Input:
            The tap number
            optional - the interpolation type (defaults to no interpolation)
         output:
            Output of delay tap
         */
        inline T getSample( int tapNum, int interpType = 0 )
        {
            return m_delay.getSample( m_delayTimes[ tapNum ], interpType ) * m_gains[ tapNum ];
        }
        
        /**
         Push a sample value into the delay line
         Input:
            value to store in delay line
         */
        inline void setSample( T x )
        {
            m_delay.setSample( x );
        }

    };
}


#endif /* sjf_rev_multitap_h */
