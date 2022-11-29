//
//  sjf_biquad_h
//
//  Created by Simon Fay on 28/11/2022.
//

#ifndef sjf_biquad_h
#define sjf_biquad_h

// implementation of biquad filter
//
template <class T>
class sjf_biquad {
public:
    sjf_biquad(){};
    ~sjf_biquad(){};

    T filterInput( T input )
    {
        T output = ( input * m_b0 ) + m_d1;
        m_d1 = ( input * m_b1 ) + m_d2 + ( output * m_a1 );
        m_d2 = ( input * m_b2 ) + ( output * m_a2 );
        return output;
    }
    
    void setCoefficients( const std::vector< T > &coefficients )
    {
        if ( coefficients.size() != 5 )
        {
            return;
        }
        m_b0 = coefficients[ 0 ];
        m_b1 = coefficients[ 1 ];
        m_b2 = coefficients[ 2 ];
        m_a1 = -1 * coefficients[ 3 ];
        m_a2 = -1 * coefficients[ 4 ];
    }
    
    T getD1() { return m_d1; }
    
    T getD2() { return m_d2; }
private:
    
    T m_d1 = 0, m_d2 = 0;
    T m_a1, m_a2, m_b0, m_b1, m_b2;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_biquad )
};


#endif /* sjf_biquad_h */



