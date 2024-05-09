//
//  sjf_multitap.h
//
//  Created by Simon Fay on 27/03/2024.
//


#ifndef sjf_rev_multitap_h
#define sjf_rev_multitap_h

#include "../sjf_rev.h"

namespace sjf::rev
{
    /**
     Basic multitap delayLine for use primarily as part of an early reflection generator
     No filtering or feedback is applied
     */
    template < typename T >
    class multiTap
    {

    public:
        multiTap( int maxNTaps) noexcept : MAXNTAPS( maxNTaps )
        {
            m_delayTimesSamps.resize( MAXNTAPS );
            m_gains.resize( MAXNTAPS );
            for ( auto i = 0; i < MAXNTAPS; i++ )
                m_delayTimesSamps[ i ] = m_gains[ i ] = 0;
        }
        ~multiTap(){}
        
        
        /**
         This must be called before first use in order to set basic information such as maximum delay length and sample rate
         */
        void initialise( T sampleRate )
        {
//            auto delSize = sjf_nearestPowerAbove( sampleRate * 0.5, 2 );
            auto delSize = sampleRate / 2;
            m_delay.initialise( delSize );
        }
        
        /**
         This must be called before first use in order to set the maximum delay length
         */
        void initialise( int maxDelayInSamps )
        {
            if ( !sjf_isPowerOf( maxDelayInSamps, 2 ) )
                maxDelayInSamps = sjf_nearestPowerAbove( maxDelayInSamps, 2 );
            m_delay.initialise( maxDelayInSamps );
        }
        
        
        /**
         Set all of the delayTimes in samples
         */
        void setDelayTimesSamps( const std::vector< T >& dt )
        {
            assert( dt.size() == MAXNTAPS );
            for ( auto d = 0; d < MAXNTAPS; d++ )
                m_delayTimesSamps[ d ] = dt[ d ];
        }
        
//        /**
//         Set the delayTime in samples for a single tap
//         */
//        void setDelayTimeSamps( int tapNum, T dt )
//        {
//            m_delayTimesSamps[ tapNum ] = dt;
//        }
        
        /**
         Set the delayTime in samples for a single tap
         */
        void setDelayTimeSamps( T dt, size_t tapNum )
        {
            m_delayTimesSamps[ tapNum ] = dt;
        }
        
        /**
         Set all of the gains
         */
        void setGains( const std::vector< T >& gains )
        {
            assert( gains.size() == MAXNTAPS );
            for ( auto  g = 0; g < MAXNTAPS; g++ )
                m_gains[ g ] = gains[ g ];
        }
        
//        /**
//         Set the gain for an individual tap
//         */
//        void setGain( int tapNum, T gain )
//        {
//            m_gains[ tapNum ] = gain;
//        }
        
        /**
         Set the gain for an individual tap
         */
        void setGain( T gain, size_t tapNum )
        {
            m_gains[ tapNum ] = gain;
        }
        
        /**
         Using this you can change the number of active taps
         */
        void setNTaps( int nTaps )
        {
            m_nTaps = nTaps < 1 ? 1 : ( nTaps > MAXNTAPS ? MAXNTAPS : nTaps );
        }
        
        /**
         Return the maximum number of taps possible
         */
        int getMaxNTaps()
        {
            return MAXNTAPS;
        }
        
        /**
         This processone sample
         Input:
            sample to be inserted into the delayLine
         Output:
            combination of all of the taps
         */
        T process( T x )
        {
            T output = 0.0;
            for ( auto t = 0; t < m_nTaps; t++ )
                output += getSample( t );
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
        inline T getSample( int tapNum )
        {
            return m_delay.getSample( m_delayTimesSamps[ tapNum ] ) * m_gains[ tapNum ];
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
        
        
        void setInterpolationType( sjf_interpolators::interpolatorTypes type ) { m_delay.setInterpolationType( type );   }
    private:
        const int MAXNTAPS;
        std::vector< T > m_delayTimesSamps, m_gains;
        int m_nTaps = MAXNTAPS;
        delay< T > m_delay;
    };
}


#endif /* sjf_rev_multitap_h */
