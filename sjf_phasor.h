//
//  sjf_phasor.h
//
//  Created by Simon Fay on 26/01/2023.
//

#ifndef sjf_phasor_h
#define sjf_phasor_h
#include <JuceHeader.h>

template< class floatType >
class sjf_phasor{
    
    floatType m_frequency = 440;
    floatType m_SR = 44100;
    floatType m_increment;
    floatType m_position = 0.0f;
    bool m_negFreqFlag = false;
public:
    sjf_phasor()
    {
        calculateIncrement() ;
    };
    sjf_phasor(const floatType & sample_rate, const floatType & f)
    {
        initialise(sample_rate, f);
    };
    
    ~sjf_phasor() {};
    
    void initialise(const floatType & sample_rate, const floatType & f)
    {
        m_SR = sample_rate;
        setFrequency( f );
    }
    
    void setSampleRate( const floatType & sample_rate)
    {
        m_SR = sample_rate;
        calculateIncrement();
    }
    
    void setFrequency(const floatType & f)
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
    
    floatType getFrequency(){ return m_frequency ;}
    
    floatType output()
    {
        floatType p = m_position;
        m_position += m_increment;
        if (m_position >= 1.0f){ m_position -= 1.0f; }
        if (!m_negFreqFlag)
        {
            return p;
        }
        else
        {
            return 1 - p;
        }
    }
    
    void setPhase(const floatType &p)
    {
        if (p < 0.0f) { p = 0.0f; }
        else if (p > 1.0f){ p = 1.0f; }
        m_position = p;
    }
    
    floatType getPhase()
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

