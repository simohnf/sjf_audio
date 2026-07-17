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
//============================================================
//============================================================
//============================================================
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
    ~sinArray(){}
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
    
    const T getValue ( T findex, int interpolationType ) const
    {
        int pos1; // this step value
        int pos2; // next step value
        T mu; // fractional part between step 1 & 2
        
        pos1 = static_cast< int >( findex );
        pos1 = fastMod4< int >( pos1, TABLE_SIZE );
        pos2 = fastMod4< int >( ( pos1 + 1 ), TABLE_SIZE );
        mu = findex - pos1;
        switch ( interpolationType )
        {
            case sjf_interpolators::interpolatorTypes::linear :
            {
                return sjf_interpolators::linearInterpolate( mu, m_table[ pos1 ], m_table[ pos2 ] );
                break;
            }
            case sjf_interpolators::interpolatorTypes::cubic :
            {
                auto pos0 = fastMod4< int >( pos1 - 1, static_cast< int >( TABLE_SIZE ) );
                auto pos3 = fastMod4< int >( pos2 + 1, static_cast< int >( TABLE_SIZE ) );
                return sjf_interpolators::cubicInterpolate( mu, m_table[ pos0 ], m_table[ pos1 ], m_table[ pos2 ], m_table[ pos3 ] );
                break;
            }
            case sjf_interpolators::interpolatorTypes::pureData :
            {
                auto pos0 = fastMod4< int >( pos1 - 1, static_cast< int >( TABLE_SIZE ) );
                auto pos3 = fastMod4< int >( pos2 + 1, static_cast< int >( TABLE_SIZE ) );
                return sjf_interpolators::fourPointInterpolatePD( mu, m_table[ pos0 ], m_table[ pos1 ], m_table[ pos2 ], m_table[ pos3 ] );
                break;
            }
            case sjf_interpolators::interpolatorTypes::fourthOrder :
            {
                auto pos0 = fastMod4< int >( pos1 - 1, static_cast< int >( TABLE_SIZE ) );
                auto pos3 = fastMod4< int >( pos2 + 1, static_cast< int >( TABLE_SIZE ) );
                return sjf_interpolators::fourPointFourthOrderOptimal( mu, m_table[ pos0 ], m_table[ pos1 ], m_table[ pos2 ], m_table[ pos3 ] );
                break;
            }
            case sjf_interpolators::interpolatorTypes::godot :
            {
                auto pos0 = fastMod4< int >( pos1 - 1, static_cast< int >( TABLE_SIZE ) );
                auto pos3 = fastMod4< int >( pos2 + 1, static_cast< int >( TABLE_SIZE ) );
                return sjf_interpolators::cubicInterpolateGodot( mu, m_table[ pos0 ], m_table[ pos1 ], m_table[ pos2 ], m_table[ pos3 ] );
                break;
            }
            case sjf_interpolators::interpolatorTypes::hermite :
            {
                auto pos0 = fastMod4< int >( pos1 - 1, static_cast< int >( TABLE_SIZE ) );
                auto pos3 = fastMod4< int >( pos2 + 1, static_cast< int >( TABLE_SIZE ) );
                return sjf_interpolators::cubicInterpolateHermite( mu, m_table[ pos0 ], m_table[ pos1 ], m_table[ pos2 ], m_table[ pos3 ] );
                break;
            }
            default:
            {
                return sjf_interpolators::linearInterpolate( mu, m_table[ pos1 ], m_table[ pos2 ] );
                break;
            }
        }
    }
private:
    T m_table[ TABLE_SIZE ];
};

//============================================================
//============================================================
//============================================================
template< typename T, int TABLE_SIZE >
struct sjf_sinArray
{
    constexpr sjf_sinArray() : m_table()
    {
        for ( int i = 0; i < TABLE_SIZE; i++ )
        {
            m_table[ i ] = gcem::sin( 2.0f * M_PI * static_cast< T >(i)/static_cast< T >(TABLE_SIZE) );
        }
    }
    ~sjf_sinArray(){}
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
    
    const T getValue ( T findex, int interpolationType ) const
    {
        int pos1; // this step value
        int pos2; // next step value
        T mu; // fractional part between step 1 & 2
        
        pos1 = static_cast< int >( findex );
        pos1 = fastMod4< int >( pos1, TABLE_SIZE );
        pos2 = fastMod4< int >( ( pos1 + 1 ), TABLE_SIZE );
        mu = findex - pos1;
        switch ( interpolationType )
        {
            case sjf_interpolators::interpolatorTypes::linear :
            {
                return sjf_interpolators::linearInterpolate( mu, m_table[ pos1 ], m_table[ pos2 ] );
                break;
            }
            case sjf_interpolators::interpolatorTypes::cubic :
            {
                auto pos0 = fastMod4< int >( pos1 - 1, static_cast< int >( TABLE_SIZE ) );
                auto pos3 = fastMod4< int >( pos2 + 1, static_cast< int >( TABLE_SIZE ) );
                return sjf_interpolators::cubicInterpolate( mu, m_table[ pos0 ], m_table[ pos1 ], m_table[ pos2 ], m_table[ pos3 ] );
                break;
            }
            case sjf_interpolators::interpolatorTypes::pureData :
            {
                auto pos0 = fastMod4< int >( pos1 - 1, static_cast< int >( TABLE_SIZE ) );
                auto pos3 = fastMod4< int >( pos2 + 1, static_cast< int >( TABLE_SIZE ) );
                return sjf_interpolators::fourPointInterpolatePD( mu, m_table[ pos0 ], m_table[ pos1 ], m_table[ pos2 ], m_table[ pos3 ] );
                break;
            }
            case sjf_interpolators::interpolatorTypes::fourthOrder :
            {
                auto pos0 = fastMod4< int >( pos1 - 1, static_cast< int >( TABLE_SIZE ) );
                auto pos3 = fastMod4< int >( pos2 + 1, static_cast< int >( TABLE_SIZE ) );
                return sjf_interpolators::fourPointFourthOrderOptimal( mu, m_table[ pos0 ], m_table[ pos1 ], m_table[ pos2 ], m_table[ pos3 ] );
                break;
            }
            case sjf_interpolators::interpolatorTypes::godot :
            {
                auto pos0 = fastMod4< int >( pos1 - 1, static_cast< int >( TABLE_SIZE ) );
                auto pos3 = fastMod4< int >( pos2 + 1, static_cast< int >( TABLE_SIZE ) );
                return sjf_interpolators::cubicInterpolateGodot( mu, m_table[ pos0 ], m_table[ pos1 ], m_table[ pos2 ], m_table[ pos3 ] );
                break;
            }
            case sjf_interpolators::interpolatorTypes::hermite :
            {
                auto pos0 = fastMod4< int >( pos1 - 1, static_cast< int >( TABLE_SIZE ) );
                auto pos3 = fastMod4< int >( pos2 + 1, static_cast< int >( TABLE_SIZE ) );
                return sjf_interpolators::cubicInterpolateHermite( mu, m_table[ pos0 ], m_table[ pos1 ], m_table[ pos2 ], m_table[ pos3 ] );
                break;
            }
            default:
            {
                return sjf_interpolators::linearInterpolate( mu, m_table[ pos1 ], m_table[ pos2 ] );
                break;
            }
        }
    }
private:
    T m_table[ TABLE_SIZE ];
};

//============================================================
//============================================================
//============================================================


template< typename T, int TABLE_SIZE >
struct sjf_cosArray
{
    constexpr sjf_cosArray() : m_table()
    {
        for ( int i = 0; i < TABLE_SIZE; i++ )
        {
            m_table[ i ] = gcem::cos( 2.0f * M_PI * static_cast< T >(i)/static_cast< T >(TABLE_SIZE) );
        }
    }
    ~sjf_cosArray(){}
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
    
    const T getValue ( T findex, int interpolationType ) const
    {
        int pos1; // this step value
        int pos2; // next step value
        T mu; // fractional part between step 1 & 2
        
        pos1 = static_cast< int >( findex );
        pos1 = fastMod4< int >( pos1, TABLE_SIZE );
        pos2 = fastMod4< int >( ( pos1 + 1 ), TABLE_SIZE );
        mu = findex - pos1;
        switch ( interpolationType )
        {
            case sjf_interpolators::interpolatorTypes::linear :
            {
                return sjf_interpolators::linearInterpolate( mu, m_table[ pos1 ], m_table[ pos2 ] );
                break;
            }
            case sjf_interpolators::interpolatorTypes::cubic :
            {
                auto pos0 = fastMod4< int >( pos1 - 1, static_cast< int >( TABLE_SIZE ) );
                auto pos3 = fastMod4< int >( pos2 + 1, static_cast< int >( TABLE_SIZE ) );
                return sjf_interpolators::cubicInterpolate( mu, m_table[ pos0 ], m_table[ pos1 ], m_table[ pos2 ], m_table[ pos3 ] );
                break;
            }
            case sjf_interpolators::interpolatorTypes::pureData :
            {
                auto pos0 = fastMod4< int >( pos1 - 1, static_cast< int >( TABLE_SIZE ) );
                auto pos3 = fastMod4< int >( pos2 + 1, static_cast< int >( TABLE_SIZE ) );
                return sjf_interpolators::fourPointInterpolatePD( mu, m_table[ pos0 ], m_table[ pos1 ], m_table[ pos2 ], m_table[ pos3 ] );
                break;
            }
            case sjf_interpolators::interpolatorTypes::fourthOrder :
            {
                auto pos0 = fastMod4< int >( pos1 - 1, static_cast< int >( TABLE_SIZE ) );
                auto pos3 = fastMod4< int >( pos2 + 1, static_cast< int >( TABLE_SIZE ) );
                return sjf_interpolators::fourPointFourthOrderOptimal( mu, m_table[ pos0 ], m_table[ pos1 ], m_table[ pos2 ], m_table[ pos3 ] );
                break;
            }
            case sjf_interpolators::interpolatorTypes::godot :
            {
                auto pos0 = fastMod4< int >( pos1 - 1, static_cast< int >( TABLE_SIZE ) );
                auto pos3 = fastMod4< int >( pos2 + 1, static_cast< int >( TABLE_SIZE ) );
                return sjf_interpolators::cubicInterpolateGodot( mu, m_table[ pos0 ], m_table[ pos1 ], m_table[ pos2 ], m_table[ pos3 ] );
                break;
            }
            case sjf_interpolators::interpolatorTypes::hermite :
            {
                auto pos0 = fastMod4< int >( pos1 - 1, static_cast< int >( TABLE_SIZE ) );
                auto pos3 = fastMod4< int >( pos2 + 1, static_cast< int >( TABLE_SIZE ) );
                return sjf_interpolators::cubicInterpolateHermite( mu, m_table[ pos0 ], m_table[ pos1 ], m_table[ pos2 ], m_table[ pos3 ] );
                break;
            }
            default:
            {
                return sjf_interpolators::linearInterpolate( mu, m_table[ pos1 ], m_table[ pos2 ] );
                break;
            }
        }
    }
private:
    T m_table[ TABLE_SIZE ];
};

//============================================================
//============================================================
//============================================================

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
    ~sjf_hannArray(){}
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
