//
//  sjf_chebyshevPolys.h
//
//  Created by Simon Fay on 08/11/2022.
//

#ifndef sjf_chebyshevPolys_h
#define sjf_chebyshevPolys_h



// class to contain arrays for chebyshev polynomials
//
class sjf_chebyshev
{
public:
    sjf_chebyshev()
    {
//        int offset1 = m_halfArraySize * ( sqrt(0.5) );
//        DBG( "offset " << offset1 );
        for ( int i = 0; i < m_arraySize; i++ )
        {
            float x = ( (float)i / (float)m_halfArraySize ) - 1.0f;
            float x2 = x * x;
            float x3 = x2 * x;
            float x4 = x3 * x;
            float x5 = x4 * x;
            float x6 = x5 * x;
            float x7 = x6 * x;
            m_chebys[ 0 ][ i ] = ( 2.0f * x2 ) - 1.0f;
            m_chebys[ 1 ][ i ] = ( 4.0f * x3 ) - ( 3.0f * x );
            m_chebys[ 2 ][ i ] = ( 8.0f * x4 ) - ( 8.0f * x2 ) + 1.0f;
            m_chebys[ 3 ][ i ] = ( 16.0f * x5 ) - ( 20.0f * x3 ) + ( 5.0f * x );
            m_chebys[ 4 ][ i ] = ( 32.0f * x6 ) - ( 48.0f * x4 ) + ( 18.0f * x2 ) - 1.0f;
            m_chebys[ 5 ][ i ] = ( 64.0f * x7 ) - ( 112.0f * x5 ) + ( 56.0f  * x3 ) - ( 7.0f * x );
        }
    }
    ~sjf_chebyshev(){}
    
    float process( int order, float value )
    {
        if (order < 0 ) { order = 0; }
        else if ( order >= m_nOrders ) { order  = m_nOrders - 1; }
        if ( value < -1.0f ) { value = -1.0f; }
        else if ( value > 1.0f ){ value = 1.0f; }
        
        value += 1.0f;
        value *= m_halfArraySize;
        return m_chebys[ order ][ value ];
    }
    
    int getNumOrders() { return m_nOrders; }
    
private:
    
    const static int m_halfArraySize = 2116800;
    const static int m_nOrders = 6;
    const static int m_arraySize = 4233600;
    
                                      
    std::array< std::array< float,  m_arraySize >, m_nOrders > m_chebys;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (sjf_chebyshev)
};

#endif /* sjf_chebyshev_h */


