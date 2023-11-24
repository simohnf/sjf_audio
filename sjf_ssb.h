//
//  sjf_ssb.h.h
//
//  Created by Simon Fay on 24/11/2023.
//

#ifndef sjf_ssb_h
#define sjf_ssb_h
#include "sjf_wavetables.h"
//

template< class T >
class sjf_phasor{
    
    T m_frequency = 440;
    T m_SR = 44100;
    T m_increment;
    T m_position = 0.0f;
    bool m_negFreqFlag = false;
public:
    sjf_phasor()
    {
        calculateIncrement() ;
    };
    sjf_phasor(const T & sample_rate, const T & f)
    {
        initialise(sample_rate, f);
    };
    
    ~sjf_phasor() {};
    
    void initialise(const T & sample_rate, const T & f)
    {
        m_SR = sample_rate;
        setFrequency( f );
    }
    
    void setSampleRate( const T & sample_rate)
    {
        m_SR = sample_rate;
        calculateIncrement();
    }
    
    void setFrequency(const T  f)
    {
        m_frequency = f;
        calculateIncrement();
    }
    
    T getFrequency(){ return m_frequency ;}
    
    T output()
    {
        T p = m_position;
        m_position += m_increment;
        m_position = (m_position >= 1) ? m_position - 1.0f : ( (m_position < 0.0f) ? m_position + 1.0f : m_position);
        return p;
    }
    
    void setPhase(const T &p)
    {
        if (p < 0.0f) { p = 0.0f; }
        else if (p > 1.0f){ p = 1.0f; }
        m_position = p;
    }
    
    T getPhase()
    {
        return m_position;
    }
    
private:
    void calculateIncrement()
    {
        m_increment = ( m_frequency / m_SR );
    };

    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (sjf_phasor)
};




template < class T >
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

// based on the hilbert transform abstraction contained in pure data
template < class T >
class sjf_hilbert
{
public:
    sjf_hilbert()
    {
        for ( auto i = 0; i < DEPTH; i++ )
            for ( auto j =0 ; j < NAPs; j++ )
            {
                m_aps[ i ][ j ].setCoefficients( m_coeffs[ i ][ j ][ 0 ], m_coeffs[ i ][ j ][ 1 ] );
            }
    }
    ~sjf_hilbert(){}
    
    std::array< T, 2 > process( const T x )
    {
        std::array< T, 2 >  output = { 0, 0 };
        for ( auto i = 0; i < DEPTH; i++ )
        {
            output[ i ] = m_aps[ i ][ 1 ].process( m_aps[ i ][ 0 ].process( x ) );
        }
        return output;
    }
    
    T get0degVersion( const T x )
    {
        return m_aps[ 0 ][ 1 ].process( m_aps[ 0 ][ 0 ].process( x ) );
    }
    
    T get90degVersion( const T x )
    {
        return m_aps[ 1 ][ 1 ].process( m_aps[ 1 ][ 0 ].process( x ) );
    }
    
private:
    static constexpr int NAPs = 2;
    static constexpr int DEPTH = 2;
    std::array< std::array< sjf_allpass< T >, NAPs >, DEPTH > m_aps;
    std::array< std::array< std::array< T, 2 >, NAPs >, DEPTH > m_coeffs =
    { { { { { -0.260502, 0.02569 }, { 0.870686, -1.8685 } } }, { { { 0.94657, -1.94632 }, { 0.06338, -0.83774 } } } } };
//    std::array< T, 2 >
//    m_coefs1a = { -0.260502, 0.02569 },
//    m_coefs1b = { 0.870686, -1.8685 },
//    m_coefs2a = { 0.94657, -1.94632 },
//    m_coefs2b = { 0.06338, -0.83774 };
};

template < class T >
class sjf_ssb
{
public:
    sjf_ssb(){}
    ~sjf_ssb(){}
    void initialise( T sampleRate )
    {
        m_phasor.initialise( sampleRate, 100 );
    }
    
    T process( T x )
    {
        auto phase = m_phasor.output() * TABLE_SIZE;
        auto zeroDeg = m_hilbert.get0degVersion( x );
        auto ninetyDeg = m_hilbert.get90degVersion( x );
        zeroDeg *= m_cosArray.getValue( phase );
        ninetyDeg *= m_sinArray.getValue( phase );
        return zeroDeg - ninetyDeg;
    }
    
    T process( T x, T cosVal, T sinVal )
    {
        return m_hilbert.get0degVersion( x )*cosVal - m_hilbert.get90degVersion( x )*sinVal;
    }
    
    std::array< T, 2 > getBothSideBands( T x )
    {
        auto phase = m_phasor.output() * TABLE_SIZE;
        auto zeroDeg = m_hilbert.get0degVersion( x );
        auto ninetyDeg = m_hilbert.get90degVersion( x );
        zeroDeg *= m_cosArray.getValue( phase );
        ninetyDeg *= m_sinArray.getValue( phase );
        return { zeroDeg - ninetyDeg, zeroDeg - ninetyDeg };
    }
    
    std::array< T, 2 > getBothSideBands( T x, T cosVal, T sinVal )
    {
        auto zeroDeg = m_hilbert.get0degVersion( x )*cosVal;
        auto ninetyDeg = m_hilbert.get90degVersion( x )*sinVal;
        return { zeroDeg - ninetyDeg, zeroDeg - ninetyDeg };
    }
    void setFreqShift( T f )
    {
        m_fShift = f;
        m_phasor.setFrequency( f );
    }
    
private:
//    std::array< std::array< sjf_allpass< T >, 2 >, NCHANNELS > m_hilbert;
    static constexpr int TABLE_SIZE = 1024;
    sjf_hilbert< T > m_hilbert;
    sjf_phasor< T > m_phasor;
    sjf_sinArray< T, TABLE_SIZE > m_sinArray;
    sjf_cosArray< T, TABLE_SIZE > m_cosArray;
    T m_fShift;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_ssb )
};



#endif /* sjf_ssb_h */






