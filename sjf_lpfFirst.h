//
//  sjf_lpfFirst.h
//
//  Created by Simon Fay on 12/11/2022.
//

#ifndef sjf_lpfFirst_h
#define sjf_lpfFirst_h

inline
float calculateOnePoleFilterCoefficient( float f, float sampleRate)
{
    return ( sin( f * 2 * 3.141593 / sampleRate ) );
}

class sjf_lpfFirst
{
public:
    sjf_lpfFirst(){};
    ~sjf_lpfFirst(){};
    
    float filterInput( float x )
    {
        // out = in + preIn
        m_y0 += m_b * (x - m_y0);
        return m_y0;
    }
    
    float getY()
    {
        return m_y0;
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
    float m_y0 = 0.0f, m_b = 0.5;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_lpfFirst )
};
#endif /* sjf_lpfFirst_h */

