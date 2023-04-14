//
//  sjf_buffir2_h
//
//  Created by Simon Fay on 13/04/2023
//

#ifndef sjf_buffir2_h
#define sjf_buffir2_h

#include "sjf_audioUtilities.h"
#include <Accelerate/Accelerate.h>
// implementation of buffer based fir filter
//
// MAC OS ONLY!!!!

template < int KERNEL_SIZE >
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
    
    void setKernel( std::array< float , KERNEL_SIZE >& kernel )
    {
        for ( int i = 0; i < KERNEL_SIZE; i++ )
        {
            m_kernel[ i ] = kernel[ i ];
        }
    }
    
    float filterInput( float samp )
    {
        float y;
        m_delay[ m_wp ] = m_delay[ m_wp + KERNEL_SIZE ] = samp;
        vDSP_dotpr ( m_delay.data() + m_wp, 1, m_kernel.data(), 1, &y, KERNEL_SIZE);
        fastMod3< int > ( --m_wp, KERNEL_SIZE ); // delay line is written in reverse to line up with kernel
        return y;
    }
    
private:
    std::array< float , KERNEL_SIZE > m_kernel;
    std::array< float, KERNEL_SIZE * 2 > m_delay; // delay buffer is double length so that it can alway be read by dot product algorithm
    int m_wp = 0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_buffir )
};


#endif /* sjf_buffir2_h */





