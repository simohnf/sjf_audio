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


namespace sjf::rev
{
    
    template < typename T, int NSTAGES >
    class seriesAllpass
    {
    public:
        seriesAllpass(){}
        ~seriesAllpass(){}
        
        void initialise( T sampleRate )
        {
            auto size = sjf_nearestPowerAbove( sampleRate * 0.1, 2 );
            for ( auto & a : m_aps )
                a.initialise( size );
        }
        
        void setCoefs( T coef )
        {
            for ( auto a = 0; a < NSTAGES; a++ )
                m_coefs[ a ] = coef;
        }
        
        void setDelayTimes( const std::array< T, NSTAGES >& dt )
        {
            for ( auto d = 0; d < NSTAGES; d++ )
                m_delayTimes[ d ] = dt[ d ];
        }
        
        T process( T x )
        {
            for ( auto a = 0; a < NSTAGES; a++ )
            {
                x = m_aps[ a ].process( x, m_delayTimes[ a ], m_coefs[ a ] );
            }
            return x;
        }
    private:
        std::array< oneMultAP< T >, NSTAGES > m_aps;
        std::array< T, NSTAGES > m_coefs, m_delayTimes;
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
