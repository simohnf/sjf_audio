//
//  sjf_delay.h
//
//  Created by Simon Fay on 27/03/2024.
//

#ifndef sjf_rev_delay_h
#define sjf_rev_delay_h

#include "../sjf_audioUtilitiesC++.h"
#include "../sjf_interpolators.h"
#include "../gcem/include/gcem.hpp"



namespace sjf::rev
{
    template < typename  T >
    class delay
    {
    private:
        std::vector< T > m_buffer;
        int m_writePos = 0, m_wrapMask;
        
    public:
        delay(){}
        ~delay(){}
        
        void initialise( int sizeInSamps_pow2 )
        {
            m_buffer.resize( sizeInSamps_pow2, 0 );
            m_wrapMask = sizeInSamps_pow2 - 1;
        }
        
        T getSample( T delay, int interpType = 1 )
        {
            auto rp = getPosition( ( m_writePos - delay ) );
            switch( interpType )
            {
                case 0:
                    return m_buffer[ rp ];
                case sjf_interpolators::interpolatorTypes::linear:
                    return linInterp( rp );
                case sjf_interpolators::interpolatorTypes::cubic:
                    return polyInterp( rp, sjf_interpolators::interpolatorTypes::cubic );
                case sjf_interpolators::interpolatorTypes::pureData:
                    return polyInterp( rp, sjf_interpolators::interpolatorTypes::pureData );
                case sjf_interpolators::interpolatorTypes::fourthOrder:
                    return polyInterp( rp, sjf_interpolators::interpolatorTypes::fourthOrder );
                case sjf_interpolators::interpolatorTypes::godot:
                    return polyInterp( rp, sjf_interpolators::interpolatorTypes::godot );
                case sjf_interpolators::interpolatorTypes::hermite:
                    return polyInterp( rp, sjf_interpolators::interpolatorTypes::hermite );
            }
            return m_buffer[ rp ];
        }
        
        void setSample( T x )
        {
            m_buffer[ m_writePos ] = x;
            m_writePos += 1;
            m_writePos &= m_wrapMask;
        }
        
    private:
        inline T linInterp( T findex )
        {
            T x1, x2, mu;
            auto ind1 = static_cast< long >( findex );
            mu = findex - ind1;
            x1 = m_buffer[ ind1 ];
            x2 = m_buffer[ ( (ind1+1) & m_wrapMask ) ];
            
            return sjf_interpolators::linearInterpolate( mu, x1, x2 );
        }
        
        inline T polyInterp( T findex, int interpType )
        {
            T x0, x1, x2, x3, mu;
            auto ind1 = static_cast< long >( findex );
            mu = findex - ind1;
            x0 = m_buffer[ ( (ind1-1) & m_wrapMask ) ];
            x1 = m_buffer[ ind1 ];
            x2 = m_buffer[ ( (ind1+1) & m_wrapMask ) ];
            x3 = m_buffer[ ( (ind1+2) & m_wrapMask ) ];
            switch ( interpType ) {
                case sjf_interpolators::interpolatorTypes::cubic :
                    return sjf_interpolators::cubicInterpolate( mu, x0, x1, x2, x3 );
                    break;
                case sjf_interpolators::interpolatorTypes::pureData :
                    return sjf_interpolators::fourPointInterpolatePD( mu, x0, x1, x2, x3 );
                    break;
                case sjf_interpolators::interpolatorTypes::fourthOrder :
                    return sjf_interpolators::fourPointFourthOrderOptimal( mu, x0, x1, x2, x3 );
                    break;
                case sjf_interpolators::interpolatorTypes::godot :
                    return sjf_interpolators::cubicInterpolateGodot( mu, x0, x1, x2, x3 );
                    break;
                case sjf_interpolators::interpolatorTypes::hermite :
                    return sjf_interpolators::cubicInterpolateHermite( mu, x0, x1, x2, x3 );
                    break;
                default:
                    return sjf_interpolators::linearInterpolate( mu, x1, x2 );
                    break;
            }
        }
        
        T getPosition( T pos )
        {
            int p = pos;
            T mu = pos - p;
            p &= m_wrapMask;
            return ( static_cast< T >( p ) + mu );
        }
    };
}

#endif /* sjf_rev_delay_h */
