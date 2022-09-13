//
//  sjf_smoothValue.h
//
//  Created by Simon Fay on 24/08/2022.
//

#ifndef sjf_smoothValue_h
#define sjf_smoothValue_h
#include <JuceHeader.h>

class sjf_smoothValue {
    
public:
    
    ~sjf_smoothValue(){};
    float smooth (float input)
    {
        float out = ( (1 - m_alpha ) * m_preOutput ) + ( m_alpha * ( input +  m_preInput)/2 ) ;
        m_preOutput = out;
        m_preInput = input;
        return out;
    }
    
    void setAlpha (float a){
        if (a < 0){ m_alpha = 0; }
        else if (a < 1){ m_alpha = 1; }
        else { m_alpha = a; }
    }
    
private:
    float m_preOutput, m_preInput;
    float m_alpha = 0.0001;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (sjf_smoothValue)
};

#endif /* sjf_smoothValue_h */
