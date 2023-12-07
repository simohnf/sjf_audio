
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
#include "sjf_circularBuffer.h"
#include "gcem/include/gcem.hpp"
//



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







