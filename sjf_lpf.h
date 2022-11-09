
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
    
    float filterInput( float x )
    {
        // out = in + preIn
        m_y0 += m_b * (x - m_y0);
        m_y1 += m_b * (m_y0 - m_y1);
        return m_y1;
    }
    
    void setCutoff( float newCutoff )
    {
        if (newCutoff >= 1) { m_b = 0.99999f; }
        else if( newCutoff < 0 ) { m_b = 0.0f; }
        else { m_b = newCutoff; }
    }
    
    float getCutoff()
    {
        return m_b;
    }
private:
    float m_y0 = 0.0f, m_y1 = 0.0f, m_b = 0.5;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_lpf )
};
#endif /* sjf_lpf_h */
