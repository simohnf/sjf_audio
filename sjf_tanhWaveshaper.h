//
//  sjf_tanhWaveshaper.h
//
//  Created by Simon Fay on 11/11/2022.
//

#ifndef sjf_tanhWaveshaper_h
#define sjf_tanhWaveshaper_h



// class to contain arrays for tanh waveshaper
//
class sjf_tanhWaveshaper
{
public:
    sjf_tanhWaveshaper()
    {
        //        int offset1 = m_halfArraySize * ( sqrt(0.5) );
        //        DBG( "offset " << offset1 );
        for ( int i = 0; i < m_arraySize; i++ )
        {
            float x = ( (float)i / (float)m_halfArraySize ) - 1.0f;
            m_tanh [ i ] = tanh( 3.1415927 * x );
        }
    }
    ~sjf_tanhWaveshaper(){}
    
    float process( float value )
    {
        if ( value < -1.0f ) { value = -1.0f; }
        else if ( value > 1.0f ){ value = 1.0f; }
        
        value += 1.0f;
        value *= m_halfArraySize;
        return m_tanh[ value ];
    }
    
private:
    
    const static int m_halfArraySize = 2116800;
    const static int m_arraySize = 4233600;
    
    
    std::array< float, m_arraySize > m_tanh;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (sjf_chebyshev)
};

#endif /* sjf_tanhWaveshaper_h */



