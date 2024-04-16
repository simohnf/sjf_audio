//
//  sjf_seriesAP.h
//
//  Created by Simon Fay on 27/03/2024.
//


#ifndef sjf_rev_seriesAllpass_h
#define sjf_rev_seriesAllpass_h

//#include "../sjf_audioUtilitiesC++.h"
//#include "../sjf_interpolators.h"
//#include "../gcem/include/gcem.hpp"
//#include "sjf_oneMultAP.h"
//
//#include "sjf_rev_consts.h"

#include "../sjf_rev.h"

namespace sjf::rev
{
    /**
     An array of one multiply allpass filters connected in series
     input signal is passed through each all pass filter in turn
     */
    template < typename T >
    class seriesAllpass
    {
    private:
        const int NSTAGES;
        std::vector< oneMultAP< T > > m_aps;
        std::vector< T > m_coefs, m_delayTimesSamps, m_damping;
        
    public:
        seriesAllpass( int nstages ) : NSTAGES( nstages )
        {
            m_aps.resize( NSTAGES );
            m_coefs.resize( NSTAGES );
            m_delayTimesSamps.resize( NSTAGES );
            m_damping.resize( NSTAGES );
            
            initialise( 44100 ); // default sample rate
            setCoefs( 0.7 ); // default coefficient
            for ( auto s = 0; s < NSTAGES; s++ )
            {
                m_delayTimesSamps[ s ] = ( rand01()*2048 ) + 1024; // random delay times just so it's initialised
                m_coefs[ s ] = 0;
                m_damping[ s ] = 0;
            }
        }
        ~seriesAllpass(){}
        
        /**
         This must be called before first use in order to set basic information such as maximum delay length and sample rate
         */
        void initialise( T sampleRate )
        {
            auto size = sjf_nearestPowerAbove( sampleRate * 0.1, 2 );
            for ( auto & a : m_aps )
                a.initialise( size );
        }
        /**
         This must be called before first use in order to set basic information such as maximum delay length
         */
        void initialise( int maxDelayInSamples )
        {
            if ( !sjf_isPowerOf( maxDelayInSamples, 2 ) )
                maxDelayInSamples  = sjf_nearestPowerAbove( maxDelayInSamples, 2 );
            for ( auto & a : m_aps )
                a.initialise( maxDelayInSamples );
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
         This sets all of the allpass coefficients
         */
        void setCoefs( const std::vector< T >& coefs )
        {
            for ( auto a = 0; a < NSTAGES; a++ )
                m_coefs[ a ] = coefs[ a ];
        }
        
        /**
         This sets the coefficient for an inndividual allpass to the give value
         */
        void setCoef( T coef, int apNum )
        {
            m_coefs[ apNum ] = coef;
        }
        
        
        
        /**
         Set all of the delayTimes
         */
        void setDelayTimes( const std::vector< T >& dt )
        {
            for ( auto d = 0; d < NSTAGES; d++ )
                m_delayTimesSamps[ d ] = dt[ d ];
        }
        
        /**
         Set all of the delayTime of an individual allpass
         */
        void setDelayTime( T dt, int apNum )
        {
            m_delayTimesSamps[ apNum ] = dt;
        }
        
        /**
         Set all of the damping coefficients
         */
        void setDamping( const std::vector< T >& damp )
        {
            for ( auto d = 0; d < NSTAGES; d++ )
                m_damping[ d ] = damp[ d ];
        }
        
        /**
         Set  the damping coefficient for an individual allpass
         */
        void setDamping( T damp, int apNum )
        {
            m_damping[ apNum ] = damp;
        }
        
        /**
         Set  all of the damping coefficients to the same value
         */
        void setDamping( T damp )
        {
            for ( auto d = 0; d < NSTAGES; d++ )
                m_damping[ d ] = damp;
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
                x = m_aps[ a ].process( x, m_delayTimesSamps[ a ], m_coefs[ a ], interpType, m_damping[ a ] );
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
