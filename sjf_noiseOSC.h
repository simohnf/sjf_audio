//
//  sjf_noiseOSC.h
//
//  Created by Simon Fay on 17/10/2022.
//

#ifndef sjf_noiseOSC_h
#define sjf_noiseOSC_h
#define PI 3.14159265

#include "sjf_audioUtilities.h"

class sjf_noiseOSC
{
public:
    sjf_noiseOSC()
    {
        m_currentTarget = randomTarget();
    };
    ~sjf_noiseOSC(){};
    
    float output( float phase )
    {
        if ( phase < m_lastPhase * 0.5 )
        {
            m_lastTarget = m_currentTarget;
            m_currentTarget = randomTarget();
            m_diff = m_currentTarget - m_lastTarget;
        }
        m_lastPhase = phase;
        return m_lastTarget + ( phase * m_diff );
    }

private:
    float randomTarget()
    {
        return ( rand01() * 2.0f ) - 1.0f;
    }
    
    float m_lastPhase = 1.0f,  m_currentTarget = 0.0f, m_lastTarget = 0.0f, m_diff;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_noiseOSC )
};
#endif /* sjf_noiseOSC_h */


