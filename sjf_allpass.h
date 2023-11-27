//
//  sjf_allpass.h
//
//  Created by Simon Fay on 27/11/2023.
//

// 2nd order allpass

#ifndef sjf_allpass_h
#define sjf_allpass_h

template < typename T >
class sjf_allpass
{
public:
    sjf_allpass(){}
    ~sjf_allpass(){}
    
    T process( T x )
    {
        auto output = m_a0*( x - m_y2 ) + m_a1 * ( m_x1 - m_y1 ) + m_x2;
        m_x2 = m_x1;
        m_x1 = x;
        m_y2 = m_y1;
        m_y1 = output;
        return output;
    }
    
    void setCoefficients( T a0, T a1 )
    {
        m_a0 = a0;
        m_a1 = a1;
    }
    
    void clear()
    {
        m_y1 = m_y2 = m_x1 = m_x2 = 0;
    }
    
private:
    T m_y1 = 0, m_y2 = 0, m_x1 = 0, m_x2 = 0;
    T m_a0 = 0.5, m_a1 = 0.5;
};


#endif /* sjf_allpass_h */

