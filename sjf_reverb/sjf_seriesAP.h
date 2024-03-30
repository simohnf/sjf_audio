//
//  sjf_seriesAP.h
//
//  Created by Simon Fay on 27/03/2024.
//


#ifndef sjf_rev_seriesAllpass_h
#define sjf_rev_seriesAllpass_h

#include "../sjf_audioUtilitiesC++.h"
#include "../sjf_interpolators.h"
#include "../gcem/include/gcem.hpp"
#include "sjf_oneMultAP.h"

#include "sjf_rev_consts.h"

namespace sjf::rev
{
    /**
     An array of one multiply allpass filters connected in series
     input signal is passed through each all pass filter in turn
     */
    template < typename T, int NSTAGES >
    class seriesAllpass
    {
    private:
        std::array< oneMultAP< T >, NSTAGES > m_aps;
        std::array< T, NSTAGES > m_coefs, m_delayTimes;
        
    public:
        seriesAllpass()
        {
            initialise( 44100 ); // default sample rate
            setCoefs( 0.7 ); // default coefficient
        }
        ~seriesAllpass(){}
        
        /**
         This must be called before first use in order to set basic information such as maximum delay lengths and sampel rate
         */
        void initialise( T sampleRate )
        {
            auto size = sjf_nearestPowerAbove( sampleRate * 0.1, 2 );
            for ( auto & a : m_aps )
                a.initialise( size );
        }
        
        /**
         This sets all of the allpass coefficients to the give value
         */
        void setCoefs( T coef )
        {
            for ( auto a = 0; a < NSTAGES; a++ )
                m_coefs[ a ] = coef;
        }
        
        /**
         Set all of the delayTimes
         */
        void setDelayTimes( const std::array< T, NSTAGES >& dt )
        {
            for ( auto d = 0; d < NSTAGES; d++ )
                m_delayTimes[ d ] = dt[ d ];
        }
        
        /**
         This processes a single delay, it should be called once for every sample in the block
         Input:
            sample to process
            interpolation type ( optional, defaults to linear, see @sjf_interpolators )
         output:
            Processed sample
         */
        T process( T x, int interpType = DEFAULT_INTERP )
        {
            for ( auto a = 0; a < NSTAGES; a++ )
            {
                x = m_aps[ a ].process( x, m_delayTimes[ a ], m_coefs[ a ], interpType );
            }
            return x;
        }

    };
    
    //============//============//============//============//============//============
    //============//============//============//============//============//============
    //============//============//============//============//============//============
    //============//============//============//============//============//============
    
//    
//    template< typename T, int NSTAGES >
//    class seriesAP
//    {
//    public:
//        seriesAP()
//        {
//            
//        }
//        ~seriesAP(){}
//        
//        void initialise( T sampleRate )
//        {
//            m_SR = sampleRate;
//            auto size = sjf_nearestPowerAbove( sampleRate, 2 );
//            m_buffer.resize( size );
//            m_wrapMask = size - 1;
//        }
//        
//    private:
//        std::vector< T > m_buffer;
//        int m_wrapMask = 0;
//        T coef = 0.7, m_SR = 44100;
//        std::array< T, NSTAGES > m_delayTimes;
//    };
//
//    
}

#endif /* sjf_rev_APLoop_h */
