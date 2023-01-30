//
//  sjf_randOSC.h
//
//  Created by Simon Fay on 26/01/2023.
//

#ifndef sjf_randOSC_h
#define sjf_randOSC_h

#include "/Users/simonfay/Programming_Stuff/sjf_audio/sjf_audioUtilities.h"

template< class floatType >
class sjf_randOSC
{
    floatType m_lastPhase = 1.0f,  m_currentTarget = 0.0f, m_lastTarget = 0.0f, m_diff, m_SR = 44100, m_increment;
    
public:
    sjf_randOSC()
    {
        m_currentTarget = randomTarget();
        setFrequency( 1.0f );
    }
    
    ~sjf_randOSC( ) { }
    
    void initialise( const int &sampleRate )
    {
        m_SR = sampleRate;
    }
    
    void setFrequency( const floatType &f )
    {
        if ( abs(f) > m_SR * 0.5)
        {
            return;
        }
        m_increment = ( f / m_SR );
        if ( f < 0 ){ m_increment *= -1.0f; }
    }
    
    floatType output( )
    {
        return calculateOutput( m_lastPhase + m_increment );
    }
    
private:
    floatType randomTarget()
    {
        return ( rand01() * 2.0f ) - 1.0f;
    }
    
    floatType calculateOutput( floatType phase )
    {
//        DBG("phase " << phase << " m_lastPhase " << m_lastPhase);
        if ( phase >= 1.0f )
        {
            phase -= 1.0f;
        }
        if ( phase < m_lastPhase * 0.5 )
        {
            m_lastTarget = m_currentTarget;
            m_currentTarget = randomTarget();
            DBG( " m_currentTarget " << m_currentTarget );
            m_diff = m_currentTarget - m_lastTarget;
        }
        m_lastPhase = phase;
//        DBG( m_lastTarget + ( phase * m_diff ) );
        return m_lastTarget + ( phase * m_diff );
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_randOSC )
};
#endif /* sjf_randOSC_h */



