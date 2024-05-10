//
//  sjf_delay.h
//
//  Created by Simon Fay on 27/03/2024.
//

#ifndef sjf_rev_delay_h
#define sjf_rev_delay_h

//#include "../sjf_audioUtilitiesC++.h"
//#include "../sjf_interpolators.h"
//#include "../gcem/include/gcem.hpp"
//
//#include "sjf_rev_consts.h"

#include "sjf_rev.h"

namespace sjf::delayLine
{
    /**
     basic circular buffer based delay line
     */
    template < typename  T >
    class delay
    {
    public:
        delay()
        {
            initialise( 4096 ); // ensure this are initialised to something
        }
        ~delay(){}
        
        
        delay( delay&&) noexcept = default;
        delay& operator=( delay&&) noexcept = default;
        
        /**
         This must be called before first use in order to set basic information such as maximum delay lengths and sample rate
         Size should be a power of 2
         */
        void initialise( int sizeInSamps_pow2 )
        {
            if (!sjf_isPowerOf( sizeInSamps_pow2, 2 ) )
                sizeInSamps_pow2 = sjf_nearestPowerAbove( sizeInSamps_pow2, 2 );
            m_buffer.resize( sizeInSamps_pow2, 0 );
            m_wrapMask = sizeInSamps_pow2 - 1;
        }
        
        /**
         This retrieves a sample from a previous point in the buffer
         Input is:
            the number of samples in the past to read from
            the interpolation type see @sjf_interpolators
         */
//        inline T getSample( T delay, int interpType = DEFAULT_INTERP )
//        {
//            auto rp = getPosition( ( m_writePos - delay ) );
//            switch( interpType )
//            {
//                case 0:
//                    return m_buffer[ static_cast< int >(rp) ];
//                case sjf_interpolators::interpolatorTypes::linear:
//                    return linInterp( rp );
//                case sjf_interpolators::interpolatorTypes::cubic:
//                    return polyInterp( rp, sjf_interpolators::interpolatorTypes::cubic );
//                case sjf_interpolators::interpolatorTypes::pureData:
//                    return polyInterp( rp, sjf_interpolators::interpolatorTypes::pureData );
//                case sjf_interpolators::interpolatorTypes::fourthOrder:
//                    return polyInterp( rp, sjf_interpolators::interpolatorTypes::fourthOrder );
//                case sjf_interpolators::interpolatorTypes::godot:
//                    return polyInterp( rp, sjf_interpolators::interpolatorTypes::godot );
//                case sjf_interpolators::interpolatorTypes::hermite:
//                    return polyInterp( rp, sjf_interpolators::interpolatorTypes::hermite );
//            }
//            return m_buffer[ rp ];
//        }
        
        
        /**
         This retrieves a sample from a previous point in the buffer
         Input is:
            the number of samples in the past to read from
            the interpolation type see @sjf_interpolators
         */
        inline T getSample( T delay )
        {
            auto rp = getPosition( ( m_writePos - delay ) );
            return m_interpolate( rp );
        }
        
        /**
         This sets the cvalue of the sample at the current write position and automatically updates the write pointer
         */
        void setSample( T x )
        {
            m_buffer[ m_writePos ] = x;
            m_writePos += 1;
            m_writePos &= m_wrapMask;
        }
        
        /** Set the interpolation Type to be used */
        void setInterpolationType( sjf_interpolators::interpolatorTypes interpType )
        {
            switch ( interpType ) {
                case 0:
                    m_interpolate = &delay::noInterp;
                    return;
                case sjf_interpolators::interpolatorTypes::linear :
                    m_interpolate = &delay::linInterp;
                    return;
                case sjf_interpolators::interpolatorTypes::cubic :
                    m_interpolate = &delay::cubicInterp;
                    return;
                case sjf_interpolators::interpolatorTypes::pureData :
                    m_interpolate = &delay::pdInterp;
                    return;
                case sjf_interpolators::interpolatorTypes::fourthOrder :
                    m_interpolate = &delay::fourPointInterp;
                    return;
                case sjf_interpolators::interpolatorTypes::godot :
                    m_interpolate = &delay::godotInterp;
                    return;
                case sjf_interpolators::interpolatorTypes::hermite :
                    m_interpolate = &delay::hermiteInterp;
                    return;
                default:
                    m_interpolate = &delay::linInterp;
                    return;
            }
        }
        
    private:
        inline T noInterp( T findex ){ return m_buffer[ findex ]; }
        
        inline T linInterp( T findex )
        {
            T x1, x2, mu;
            auto ind1 = static_cast< long >( findex );
            mu = findex - ind1;
            x1 = m_buffer[ ind1 ];
            x2 = m_buffer[ ( (ind1+1) & m_wrapMask ) ];
            
            return sjf_interpolators::linearInterpolate( mu, x1, x2 );
        }
        
        inline T cubicInterp( T findex )
        {
            T x0, x1, x2, x3, mu;
            auto ind1 = static_cast< long >( findex );
            mu = findex - ind1;
            x0 = m_buffer[ ( (ind1-1) & m_wrapMask ) ];
            x1 = m_buffer[ ind1 ];
            x2 = m_buffer[ ( (ind1+1) & m_wrapMask ) ];
            x3 = m_buffer[ ( (ind1+2) & m_wrapMask ) ];
            return sjf_interpolators::cubicInterpolate( mu, x0, x1, x2, x3 );
        }
        
        inline T pdInterp( T findex )
        {
            T x0, x1, x2, x3, mu;
            auto ind1 = static_cast< long >( findex );
            mu = findex - ind1;
            x0 = m_buffer[ ( (ind1-1) & m_wrapMask ) ];
            x1 = m_buffer[ ind1 ];
            x2 = m_buffer[ ( (ind1+1) & m_wrapMask ) ];
            x3 = m_buffer[ ( (ind1+2) & m_wrapMask ) ];
            return sjf_interpolators::fourPointInterpolatePD( mu, x0, x1, x2, x3 );
        }

        
        inline T fourPointInterp( T findex )
        {
            T x0, x1, x2, x3, mu;
            auto ind1 = static_cast< long >( findex );
            mu = findex - ind1;
            x0 = m_buffer[ ( (ind1-1) & m_wrapMask ) ];
            x1 = m_buffer[ ind1 ];
            x2 = m_buffer[ ( (ind1+1) & m_wrapMask ) ];
            x3 = m_buffer[ ( (ind1+2) & m_wrapMask ) ];
            return sjf_interpolators::fourPointFourthOrderOptimal( mu, x0, x1, x2, x3 );
        }
        
        inline T godotInterp( T findex )
        {
            T x0, x1, x2, x3, mu;
            auto ind1 = static_cast< long >( findex );
            mu = findex - ind1;
            x0 = m_buffer[ ( (ind1-1) & m_wrapMask ) ];
            x1 = m_buffer[ ind1 ];
            x2 = m_buffer[ ( (ind1+1) & m_wrapMask ) ];
            x3 = m_buffer[ ( (ind1+2) & m_wrapMask ) ];
            return sjf_interpolators::cubicInterpolateGodot( mu, x0, x1, x2, x3 );
        }
        
        inline T hermiteInterp( T findex )
        {
            T x0, x1, x2, x3, mu;
            auto ind1 = static_cast< long >( findex );
            mu = findex - ind1;
            x0 = m_buffer[ ( (ind1-1) & m_wrapMask ) ];
            x1 = m_buffer[ ind1 ];
            x2 = m_buffer[ ( (ind1+1) & m_wrapMask ) ];
            x3 = m_buffer[ ( (ind1+2) & m_wrapMask ) ];
            return sjf_interpolators::cubicInterpolateHermite( mu, x0, x1, x2, x3 );
        }

        T getPosition( T pos )
        {
            int p = pos;
            T mu = pos - p;
            p &= m_wrapMask;
            return ( static_cast< T >( p ) + mu );
        }
        
    private:
        std::vector< T > m_buffer;
        int m_writePos = 0, m_wrapMask;
        sjf::utilities::classMemberFunctionPointer<delay, T, T> m_interpolate{ this, &delay::linInterp };
    };
}

#endif /* sjf_rev_delay_h */
