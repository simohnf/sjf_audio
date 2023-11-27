//
//  sjf_ssb.h.h
//
//  Created by Simon Fay on 24/11/2023.
//

#ifndef sjf_ssb_h
#define sjf_ssb_h
#include "sjf_wavetables.h"
#include "sjf_hilbert.h"
#include "sjf_phasor.h"

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






