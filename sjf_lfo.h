//
//  sjf_lfo.h
//
//  Created by Simon Fay on 17/10/2022.
//

#ifndef sjf_lfo_h
#define sjf_lfo_h

#include "/Users/simonfay/Programming_Stuff/sjf_audio/sjf_phaseRateMultiplier.h"
#include "/Users/simonfay/Programming_Stuff/sjf_audio/sjf_audioUtilities.h"

class sjf_lfo
{
public:
    sjf_lfo(){};
    ~sjf_lfo(){};
    
    float output()
    {
        m_phase = m_count / m_SR;
        m_count++;
        while ( m_count >= m_SR )
        {
            m_count -= m_SR;
        }
        
        m_phase = m_rateMultiplier.rateChange( m_phase );
    }
    
    void setLFOtype( int type )
    {
        m_lfoType = type;
    }
    
    enum lfoType
    {
        sine, triangle, noise1, noise2
    };
    
private:
    float m_SR = 44100.0f, m_phase = 0;
    int m_count = 0, m_lfoType = 0;
    sjf_phaseRateMultiplier m_rateMultiplier;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_lfo )
};
#endif /* sjf_lfo_h */

