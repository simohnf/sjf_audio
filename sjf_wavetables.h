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
    constexpr sinArray() : table()
    {
        for ( int i = 0; i < TABLE_SIZE; i++ )
        {
            table[ i ] = gcem::sin( 2.0f * M_PI * static_cast< T >(i)/static_cast< T >(TABLE_SIZE) );
        }
    }
    const T& operator[](std::size_t index) const { return table[ index ]; }
    const T getValue ( T findex ) const
    {
        T y1; // this step value
        T y2; // next step value
        T mu; // fractional part between step 1 & 2
        
        int index = findex;
        mu = findex - index;
        
        fastMod3( index, TABLE_SIZE );
        y1 = table[ index ];
        fastMod3( ++index, TABLE_SIZE );
        y2 = table[ index ];
        
        return y1 + mu*(y2-y1) ;
    }
private:
    T table[ TABLE_SIZE ];
};



template< typename T, int TABLE_SIZE >
struct sjf_hannArray
{
    constexpr sjf_hannArray() : table()
    {
        for ( int i = 0; i < TABLE_SIZE; i++ )
        {
            auto val = 0.75 + static_cast< T >( i );
            table[ i ] = 0.5 * gcem::sin( ( 2.0f * M_PI * val )/ ( static_cast< T >(TABLE_SIZE) ) );
        }
    }
    
    const T& operator[](std::size_t index) const { return table[ index ]; }
    const T getValue ( T findex ) const
    {
        
        T y1; // this step value
        T y2; // next step value
        T mu; // fractional part between step 1 & 2

        int index = findex;
        mu = findex - index;

        fastMod3( index, TABLE_SIZE );
        y1 = table[ index ];
        fastMod3( ++index, TABLE_SIZE );
        y2 = table[ index ];
        
        return sjf_interpolators::linearInterpolate( mu, y1, y2 );
    }
private:
    T table[ TABLE_SIZE ];
};

#endif /* sjf_wavetables_h */
