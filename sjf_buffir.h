//
//  sjf_buffir_h
//
//  Created by Simon Fay on 13/04/2023
//

#ifndef sjf_buffir_h
#define sjf_buffir_h

#include "sjf_audioUtilities.h"
// implementation of buffer based fir filter
//
template < class T, int KERNEL_SIZE >
class sjf_buffir
{
public:
    sjf_buffir()
    {
        for ( int i = 0; i < KERNEL_SIZE; i++ )
        {
            m_delay[ i ] = 0;
        }
    };
    ~sjf_buffir(){};
    
    void setKernel( std::array< T, KERNEL_SIZE >& kernel )
    {
        for ( int i = 0; i < KERNEL_SIZE; i++ )
        {
            m_kernel[ i ] = kernel[ i ];
        }
    }
    
    T filterInput( T samp )
    {
        m_delay[ m_wp ] = samp;
        samp *= m_kernel[ 0 ];
        int rp = m_wp;
        for ( int i = 1; i < KERNEL_SIZE; i++ )
        {
            fastMod3<int>( --rp, KERNEL_SIZE );
            samp += m_delay[ rp ] * m_kernel[ i ];
//            rp--;
        }
        fastMod3<int>( ++m_wp, KERNEL_SIZE );
        
        return samp;
    }
    
private:
    std::array< T, KERNEL_SIZE > m_kernel, m_delay;
    int m_wp = 0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_buffir )
};


#endif /* sjf_buffir_h */




