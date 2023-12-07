
//
//  sjf_rotary.h
//
//  Created by Simon Fay on 27/11/2023.
//

#ifndef sjf_rotary_h
#define sjf_rotary_h
#include "sjf_wavetables.h"
#include "sjf_delayLine.h"
#include "sjf_wavetables.h"
#include "sjf_phasor.h"
#include "sjf_lpf.h"
#include "sjf_audioUtilitiesC++.h"
#include "sjf_interpolators.h"
#include "gcem/include/gcem.hpp"
//

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

template < class T >
class sjf_rotary
{
private:
    static constexpr int NCHANNELS = 2;
    static constexpr int TABLESIZE = 1024;
    static constexpr int NBANDS = 2;
    T m_delayTime = 441, m_modDepth = 1, m_modDepthSamples = m_delayTime * m_modDepth;
    std::array< std::array< sjf_circularBuffer< T >, NBANDS >, NCHANNELS > m_delays;
    std::array< sjf_phasor< T >, NBANDS > m_phasors;
    std::array< sjf_lpf< T >, NCHANNELS > m_lpfs;
    std::array< T, NBANDS > m_rotationMultiples = { 0.5, 2 };
    T m_baseF = 0.5, m_rotRatio = 1, m_Xtalk = 0.7;
    sjf_sinArray< T, TABLESIZE > m_sinArr;
    
    
public:
    sjf_rotary(){}
    ~sjf_rotary(){}
    
    void initialise( T sampleRate, T maxDelayInSeconds = 1 )
    {
        m_delayTime = sampleRate * 0.01; // 10ms delay
        for ( auto & dChans : m_delays )
        {
            for ( auto & d : dChans )
                d.initialise( sampleRate * maxDelayInSeconds );
        }
        for ( auto i = 0; i < NBANDS; i++ )
            m_phasors[ i ].initialise( sampleRate, m_baseF * m_rotationMultiples[ i ] );
    }
    
    void processInPlace( std::array< T, NCHANNELS >& samples )
    {
        T s = 0, sinWc = 0;
        for ( auto c = 0; c < NCHANNELS; c++ )
        {
            for ( auto b = 0; b < NBANDS; b++ )
            {
                s = ( b == 0 ) ? m_lpfs[ c ].filterInputSecondOrder( samples[ c ] ) : samples[ c ] - s;
                m_delays[ c ][ b ].setSample( s );
            }
            samples[ c ] = 0;
        }
     
        for ( auto b = 0; b < NBANDS; b++ )
        {
            sinWc = m_sinArr.getValue( m_phasors[ b ].output() * static_cast< T >( TABLESIZE ) );
            for ( auto c = 0; c < NCHANNELS; c++ )
            {
                sinWc = ( c == 1 ) ? sinWc*-1 : sinWc;
                samples[ c ] += m_delays[ c ][ b ].getSample( ( 1 + m_delayTime + ( sinWc * m_modDepthSamples ) ) ) * ( 1 + sinWc );
            }
        }
        
        // crossover
        s = samples[ 0 ] + ( m_Xtalk * samples[ 1 ] );
        samples[ 1 ] = samples[ 1 ] + ( m_Xtalk * samples[ 0 ] );
        samples [ 0 ] = s;
    }
    
    void setDelayTime( T delayInSamps, T modulationDepthPercentage = 100 )
    {
#ifndef NDEBUG
        assert ( modulationDepthPercentage >= 0 && modulationDepthPercentage <= 100 );
#endif
        m_delayTime =  delayInSamps > 0 ? delayInSamps : ( delayInSamps == 0 ) ? 1 : abs( delayInSamps );
        m_modDepth = modulationDepthPercentage* 0.01;
        m_modDepthSamples = m_delayTime * m_modDepth;
    }
    

    void setBaseRotationFrequency( T f )
    {
        m_baseF = f;
        for ( auto i = 0; i < NBANDS; i++ )
            m_phasors[ i ].setFrequency( m_baseF * m_rotationMultiples[ i ] );
    }
    
    T getBaseFrequency()
    {
        return m_baseF;
    }
    
    void setRotationOffset( T rotationOffsetPercentage )
    {
#ifndef NDEBUG
        assert ( rotationOffsetPercentage >= 0 && rotationOffsetPercentage <= 100 );
#endif
        if ( m_rotRatio != rotationOffsetPercentage * 0.01 )
        {
            m_rotRatio = rotationOffsetPercentage * 0.01;
            m_rotationMultiples[ 0 ] = std::pow( 2.0, -1.0 * m_rotRatio );
            m_rotationMultiples[ 1 ] = std::pow( 2.0, m_rotRatio );
            setBaseRotationFrequency( m_baseF );
        }
    }
    
    T getRotationOffset()
    {
        return m_rotRatio;
    }
    
    void setXoverF( T f, T sampleRate )
    {
        T c = calculateLPFCoefficient( f, sampleRate );
        for ( auto & filt : m_lpfs )
            filt.setCoefficient( c );
    }
    
    void setStereoDepth( T stereoDepthPercentage )
    {
#ifndef NDEBUG
        assert ( stereoDepthPercentage >= 0 && stereoDepthPercentage <= 100 );
#endif
        m_Xtalk = 0.7 * ( 1 - ( stereoDepthPercentage * 0.01 ) );
    }
    
    void clear()
    {
        for ( auto c = 0; c < NCHANNELS; c++ )
            for ( auto b = 0; b < NBANDS; b++ )
                m_delays[ c ][ b ].clear();
    }
    

    
};



#endif /* sjf_rotary_h */







