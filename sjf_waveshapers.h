//
//  sjf_waveshapers.h
//
//  Created by Simon Fay on 25/07/2025.
//

#ifndef sjf_waveshapers_h
#define sjf_waveshapers_h

template< class T >
inline T sjf_cubic( T x )
{
    x -= ( x * x * x );
    return x;
}


template< class T >
inline T sjf_softClip( T x )
{
    static constexpr T twoThirds =  2.0 / 3.0;
    x = (x < -1.0) ? -twoThirds : ( (x > 1.0) ? twoThirds : ( x - ( x * x * x ) / 3.0 ) );
    return x;
}

template< class T >
class sjf_asymAllpass
{
public:
    T process ( T x, T aPos, T aNeg )
    {
        return ( x + m_y1 >= 0 ? calculate( x, aPos ) : calculate( x, aNeg ) );
    }
    
    T process ( T x )
    {
        return ( x + m_y1 >= 0 ? calculate( x, m_aPos ) : calculate( x, m_aNeg ) );
    }
    
    void setA( double aPos, double aNeg )
    {
        m_aPos = aPos;
        m_aNeg = aNeg;
    }
    
    void clear()
    {
        m_y1 = m_x1 = 0;
    }
private:
    T calculate( T x, T a )
    {
        m_y1 = a*x + m_x1 - a*m_y1;
        m_x1 = x;
        return m_y1;
    }
    T m_y1 = 0, m_x1 =0, m_aPos = 0, m_aNeg = 0;
};

#endif 
