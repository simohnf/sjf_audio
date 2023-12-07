//
//  sjf_circularBuffer_h
//
//  Created by Simon Fay on 07/12/2023.
//

#ifndef sjf_circularBuffer_h
#define sjf_circularBuffer_h

template< typename T >
class sjf_circularBuffer
{
    // uses bit mask as per Pirkle designing audio effect plugins p. 400;
public:
    sjf_circularBuffer()
    {
        clear();
    }
    ~sjf_circularBuffer(){}
    
    void initialise( int sizeInSamps )
    {
        DBG( "initialise delay ");
        m_size = nearestPow2(sizeInSamps);
        m_wrapMask = m_size - 1;
        m_buffer.resize( m_size );
        clear();
    }
    
    void setSample( T samp )
    {
        m_buffer[ m_writePos ] = samp;
        m_writePos++;
        m_writePos &= m_wrapMask;
    }
    
    T getSample( T delayInSamps )
    {
        T findex = m_writePos - delayInSamps;
        findex = findex < 0 ? findex + m_size : findex;
//        T findex = fastMod4< T >( m_writePos - delayInSamps, m_size ) ;
        T x0, x1, x2, x3, mu;
        auto ind1 = static_cast< long >( findex );
        mu = findex - ind1;
        x0 = m_buffer[ ( (ind1-1) & m_wrapMask ) ];
        x1 = m_buffer[ ind1 ];
        x2 = m_buffer[ ( (ind1+1) & m_wrapMask ) ];
        x3 = m_buffer[ ( (ind1+2) & m_wrapMask ) ];
        
        return sjf_interpolators::fourPointInterpolatePD ( mu, x0, x1, x2, x3 );
        
    }
    
    void clear()
    {
        std::fill( m_buffer.begin(), m_buffer.end(), 0 );
    }
private:
    
    unsigned long nearestPow2( unsigned long val )
    {
        return std::pow( 2, std::ceil( std::log(44100)/ std::log(2) ) );
    }
    
    std::vector< T > m_buffer;
    unsigned long m_writePos = 0;
    unsigned long m_size = nearestPow2( 44100 );
    unsigned long m_wrapMask = m_size - 1;
    
};

#endif
