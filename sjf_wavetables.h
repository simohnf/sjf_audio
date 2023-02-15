//
//  sjf_wavetables.h
//
//  Created by Simon Fay on 19/08/2022.
//
#ifndef sjf_wavetables_h
#define sjf_wavetables_h

#include "/Users/simonfay/Programming_Stuff/gcem/include/gcem.hpp"
template< typename T, int TABLE_SIZE >
struct sinArray
{
    constexpr sinArray() : table()
    {
        for ( int i = 0; i < TABLE_SIZE; i++ )
        {
            table[ i ] = gcem::sin( 2.0f * PI * (T)i/(T)TABLE_SIZE );
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

#endif /* sjf_wavetables_h */
