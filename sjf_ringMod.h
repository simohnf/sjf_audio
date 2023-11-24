//
//  sjf_ringMod.h
//
//  Created by Simon Fay on 26/01/2023.
//

#ifndef sjf_ringMod_h
#define sjf_ringMod_h

#include "sjf_audioUtilities.h"
#include "sjf_wavetables.h"

template< typename T >
class sjf_ringMod
{
public:
    sjf_ringMod(){}
    ~sjf_ringMod(){}
    
    void initialise( T sampleRate )
    {
        sampleRate = sampleRate <= 0 ? 44100 : sampleRate;
        m_SR = sampleRate;
        setModFreq( 440 );
    }
    
    T process( T input, int modType = modTypes::ringMod )
    {
        m_modVal = m_osc.getValue( m_readPos, m_interpolationType );
        m_modVal = (modType == modTypes::ampMod) ? 1 + ( m_modVal * m_ampModDepth ) : m_modVal;
        m_readPos = fastMod4< T >(m_readPos + m_readIncrementSamps, TABLE_SIZE );
        return ( input * m_modVal );
    }
    
    T getModVal( )
    {
        return m_modVal;
    }
    
    void setModFreq( T f )
    {
#ifndef NDEBUG
        assert( f > 0 );
#endif
        m_readIncrementSamps = f * static_cast< T >( TABLE_SIZE ) / m_SR;
    }
    
    void setInterpolationType( int interpType )
    {
        m_interpolationType = interpType;
    }
    
    void setAmpModDepth( T ampModDepthPercentage )
    {
#ifndef NDEBUG
        assert( ampModDepthPercentage >= 0 && ampModDepthPercentage <= 100 );
#endif
        m_ampModDepth = ampModDepthPercentage* 0.01;
    }
    
    enum modTypes { ringMod, ampMod };
    
private:
    static constexpr int TABLE_SIZE = 1024;
    sinArray< T, TABLE_SIZE > m_osc;
    
    int m_interpolationType = sjf_interpolators::pureData;
    T m_readPos = 0;
    T m_SR = 44100.0;
    T m_readIncrementSamps = static_cast< T >( TABLE_SIZE ) / m_SR;
    T m_ampModDepth = 1;
    T m_modVal = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_ringMod )
};
#endif /* sjf_ringMod_h */



