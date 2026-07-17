//
//  sjf_hilbert.h
//
//  Created by Simon Fay on 27/11/2023.
//

// 2nd order allpass

#ifndef sjf_hilbert_h
#define sjf_hilbert_h


#include "sjf_allpass2ndOrder.h"

// based on the hilbert transform abstraction contained in pure data

template < typename T >
class sjf_hilbert
{
    
public:
    sjf_hilbert()
    {
        for ( auto i = 0; i < DEPTH; i++ )
            for ( auto j =0 ; j < NAPs; j++ )
            {
                m_aps[ i ][ j ].setCoefficients( m_coeffs[ i ][ j ][ 0 ], m_coeffs[ i ][ j ][ 1 ] );
            }
    }
    ~sjf_hilbert(){}
    
    std::array< T, 2 > process( const T x )
    {
        std::array< T, 2 >  output = { 0, 0 };
        for ( auto i = 0; i < DEPTH; i++ )
        {
            output[ i ] = m_aps[ i ][ 1 ].process( m_aps[ i ][ 0 ].process( x ) );
        }
        return output;
    }
    
    T get0degVersion( const T x )
    {
        return m_aps[ 0 ][ 1 ].process( m_aps[ 0 ][ 0 ].process( x ) );
    }
    
    T get90degVersion( const T x )
    {
        return m_aps[ 1 ][ 1 ].process( m_aps[ 1 ][ 0 ].process( x ) );
    }
    
private:
    static constexpr int NAPs = 2;
    static constexpr int DEPTH = 2;
    std::array< std::array< sjf_allpass2ndOrder< T >, NAPs >, DEPTH > m_aps;
    std::array< std::array< std::array< T, 2 >, NAPs >, DEPTH > m_coeffs =
    { { { { { -0.260502, 0.02569 }, { 0.870686, -1.8685 } } }, { { { 0.94657, -1.94632 }, { 0.06338, -0.83774 } } } } };
};

#endif /* sjf_hilbert_h */

