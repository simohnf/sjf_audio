//
//  sjf_triangle.h
//
//  Created by Simon Fay on 17/10/2022.
//

#ifndef sjf_triangle_h
#define sjf_triangle_h

// convert phase ramp 0->1 to triangle wave with variable duty cycle
// not band limited!!!
// suitable for lfo
class sjf_triangle
{
public:
    sjf_triangle(){};
    ~sjf_triangle(){};
    
    float output ( float x )
    {
        x += 0.25;
        x = x - (int)x;
        
        if ( x > m_duty )
        {
            x = ( 1.0f - x ) * m_inverseOneMinusDuty;
        }
        else
        {
            x *= m_duty;
        }
        x *= 2.0f;
        x -= 1.0f;
        
        return x;
    }
    
    void setDuty( float d )
    {
        m_duty = d;
        m_inverseOneMinusDuty = 1.0f / ( 1.0f - m_duty );
    }
    
private:
    float m_duty = 0.5f, m_inverseOneMinusDuty = 2.0f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_triangle )
};
#endif /* sjf_triangle_h */

