
//
//  sjf_bitCrusher_h
//
//  Created by Simon Fay on 28/11/2022.
//

#ifndef sjf_bitCrusher_h
#define sjf_bitCrusher_h

#include "gcem/include/gcem.hpp"
// wrapper for containing both biquad and coefficient calculator in one object
//

template < class T >
class sjf_bitCrusher {
public:
    sjf_bitCrusher() { };
    ~sjf_bitCrusher(){ };
    
    T process( T val )
    {
        
        if ( m_rateDivide > 1 )
        {
            val = ( m_count == 0 ) ? std::round( val * m_nQuantizeVals ) * m_nQuantizeValsInv : m_lastOut;
        }
        else
        {
            val = std::round( val * m_nQuantizeVals ) * m_nQuantizeValsInv;
        }
        m_count++;
        m_count = m_count < m_rateDivide ? m_count : m_count % m_rateDivide;
        m_lastOut = val;
        return val;
        
    }
    
    void setNBits( int bits )
    {
#ifndef NDEBUG
        assert ( bits >= 1 && bits <= 64 );
#endif
        m_nQuantizeVals = gcem::pow( 2.0, bits );
        m_nQuantizeValsInv = ( 1.0 / m_nQuantizeVals );
    }
    
    void setSRDivide( int SRDivide )
    {
#ifndef NDEBUG
        assert ( SRDivide >= 1 );
#endif
        m_rateDivide = SRDivide;
    }
    
private:
    T m_nQuantizeVals = gcem::pow( 2.0, 32.0 );
    T m_nQuantizeValsInv = ( 1.0 / m_nQuantizeVals );
    int  m_rateDivide = 1;
    size_t m_count = 0;
    T m_lastOut = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_bitCrusher )
};


#endif /* sjf_bitCrusher_h */





