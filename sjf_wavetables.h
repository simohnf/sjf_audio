//
//  sjf_wavetables.h
//
//  Created by Simon Fay on 19/08/2022.
//
#ifndef sjf_wavetables_h
#define sjf_wavetables_h
#include <math.h>
#include "gcem/include/gcem.hpp"
#include "sjf_interpolators.h"
template< typename T, int TABLE_SIZE >
struct sinArray
{
    constexpr sinArray() : m_table()
    {
        for ( int i = 0; i < TABLE_SIZE; i++ )
        {
            m_table[ i ] = gcem::sin( 2.0f * M_PI * static_cast< T >(i)/static_cast< T >(TABLE_SIZE) );
        }
    }
    const T& operator[](std::size_t index) const { return m_table[ index ]; }
    const T getValue ( T findex ) const
    {
        T y1; // this step value
        T y2; // next step value
        T mu; // fractional part between step 1 & 2
        
        int index = findex;
        mu = findex - index;
        
        fastMod3( index, TABLE_SIZE );
        y1 = m_table[ index ];
        fastMod3( ++index, TABLE_SIZE );
        y2 = m_table[ index ];
        
        return y1 + mu*(y2-y1) ;
    }
private:
    T m_table[ TABLE_SIZE ];
};



template< typename T, int TABLE_SIZE >
struct sjf_hannArray
{
    constexpr sjf_hannArray() : m_table()
    {
        m_table[ 0 ] = 0;
        m_table[ TABLE_SIZE + 1 ] = 0;
        
        for ( int i = 0; i < TABLE_SIZE; i++ )
        {
            m_table[ i + 1 ] = 0.5 - ( 0.5 * gcem::cos( 2.0 * M_PI * static_cast< T >( i ) / static_cast< T >( TABLE_SIZE - 1 ) ) );
        }
    }
    
    const T& operator[](std::size_t index) const { return m_table[ index ]; }
    const T getValue ( T findex ) const
    {
        
        T y1; // this step value
        T y2; // next step value
        T mu; // fractional part between step 1 & 2

        int index = findex;
        mu = findex - index;

        fastMod3( index, TABLE_SIZE );
        y1 = m_table[ index ];
        fastMod3( ++index, TABLE_SIZE );
        y2 = m_table[ index ];
        
        return sjf_interpolators::linearInterpolate( mu, y1, y2 );
    }
private:
    T m_table[ TABLE_SIZE + 2 ]; // an extra 2 zeros, one at the beginning and one at the end of the envelope
};

#endif /* sjf_wavetables_h */
