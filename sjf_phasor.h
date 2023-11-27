//
//  sjf_phasor.h
//
//  Created by Simon Fay on 26/01/2023.
//

#ifndef sjf_phasor_h
#define sjf_phasor_h
#include <JuceHeader.h>

template< typename T >
class sjf_phasor{
    
    T m_frequency = 440;
    T m_SR = 44100;
    T m_increment;
    T m_position = 0.0f;
    bool m_negFreqFlag = false;
public:
    sjf_phasor()
    {
        calculateIncrement() ;
    };
    sjf_phasor(const T sample_rate, const T f)
    {
        initialise(sample_rate, f);
    };
    
    ~sjf_phasor() {};
    
    void initialise(const T sample_rate, const T f)
    {
        m_SR = sample_rate;
        setFrequency( f );
    }
    
    void setSampleRate( const T sample_rate)
    {
        m_SR = sample_rate;
        calculateIncrement();
    }
    
    void setFrequency(const T  f)
    {
        m_frequency = f;
        calculateIncrement();
    }
    
    T getFrequency(){ return m_frequency;}
    
    T output()
    {
        T p = m_position;
        m_position += m_increment;
        m_position = (m_position >= 1) ? m_position - 1.0f : ( (m_position < 0.0f) ? m_position + 1.0f : m_position);
        return p;
    }
    
    void setPhase(const T p)
    {
        if (p < 0.0f) { p = 0.0f; }
        else if (p > 1.0f){ p = 1.0f; }
        m_position = p;
    }
    
    T getPhase()
    {
        return m_position;
    }
private:
    void calculateIncrement()
    {
        m_increment = ( m_frequency / m_SR );
    };

    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (sjf_phasor)
};

#endif /* sjf_phasor_h */

