//
//  sjf_noiseOSC.h
//
//  Created by Simon Fay on 17/10/2022.
//

#ifndef sjf_noiseOSC_h
#define sjf_noiseOSC_h
#define PI 3.14159265

#include "/Users/simonfay/Programming_Stuff/sjf_audio/sjf_audioUtilities.h"

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
        if ( phase < m_lastPhase )
        {
            m_lastTarget = m_currentTarget;
            m_currentTarget = randomTarget();
        }
        m_lastPhase = phase;
        m_out += phase * ( m_currentTarget - m_lastTarget );
        return m_out;
    }

private:
    float randomTarget()
    {
        return ( rand01() * 2.0f ) - 1.0f;
    }
    
    float m_lastPhase = 1.0f, m_out = 0.0f, m_currentTarget = 0.0f, m_lastTarget = 0.0f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_noiseOSC )
};
#endif /* sjf_noiseOSC_h */


