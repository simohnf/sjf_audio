//
//  sjf_phasor.h
//
//  Created by Simon Fay on 24/08/2022.
//

#ifndef sjf_phasor_h
#define sjf_phasor_h
#include <JuceHeader.h>

class sjf_phasor{
public:
    sjf_phasor() { calculateIncrement() ; };
    sjf_phasor(float sample_rate, float f) { initialise(sample_rate, f); };
    
    ~sjf_phasor() {};
    
    void initialise(float sample_rate, float f)
    {
        m_SR = sample_rate;
        setFrequency( f );
    }
    
    void setSampleRate( float sample_rate)
    {
        m_SR = sample_rate;
        calculateIncrement();
    }
    
    void setFrequency(float f)
    {
        m_frequency = f;
        
        if (m_frequency >= 0)
        {
            m_negFreqFlag = false;
            m_increment = m_frequency / m_SR ;
        }
        else
        {
            m_negFreqFlag = true;
            m_increment = -1.0f * m_frequency / m_SR ;
        }
    }
    
    float getFrequency(){ return m_frequency ;}
    
    float output()
    {
        if (!m_negFreqFlag)
        {
            float p = m_position;
            m_position += m_increment;
            if (m_position >= 1.0f){ m_position -= 1.0f; }
            return p;
        }
        else
        {
            float p = m_position;
            m_position += m_increment;
            while (m_position >= 1.0f){ m_position -= 1.0f; }
            return 1 - p;
        }
    }
    
    void setPhase(float p)
    {
        if (p < 0.0f) {p = 0.0f;}
        else if (p > 1.0f){ p = 1.0f;}
        m_position = p;
    }
    
    float getPhase(){
        return m_position;
    }
private:
    void calculateIncrement(){ m_increment = ( m_frequency / m_SR ); };
    
    float m_frequency = 440;
    float m_SR = 44100;
    float m_increment;
    float m_position = 0.0f;
    bool m_negFreqFlag = false;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (sjf_phasor)
};

#endif /* sjf_phasor_h */
