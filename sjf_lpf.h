
//
//  sjf_lpf.h
//
//  Created by Simon Fay on 11/10/2022.
//

#ifndef sjf_lpf_h
#define sjf_lpf_h

class sjf_lpf
{
public:
    sjf_lpf(){};
    ~sjf_lpf(){};
    
    float filterInput( float value )
    {
        m_buf0 += m_cutoff * (value - m_buf0);
        m_buf1 += m_cutoff * (m_buf0 - m_buf1);
        return m_buf1;
    }
    
    void setCutoff( float newCutoff )
    {
        if (newCutoff >= 1) { m_cutoff = 0.99999f; }
        else if( newCutoff < 0 ) { m_cutoff = 0.0f; }
        else { m_cutoff = newCutoff; }
    }
    
    float getCutoff()
    {
        return m_cutoff;
    }
private:
    float m_buf0 = 0.0f, m_buf1 = 0.0f, m_cutoff = 0.5;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_lpf )
};
#endif /* sjf_lpf_h */
