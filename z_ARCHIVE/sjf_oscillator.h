//
//  sjf_oscillator.h
//
//  Created by Simon Fay on 24/08/2022.
//

#ifndef sjf_oscillator_h
#define sjf_oscillator_h

#include <vector>
#include <JuceHeader.h>

#define PI 3.14159265

class sjf_oscillator {
    
public:
    sjf_oscillator()
    {
        m_waveTable.resize(m_waveTableSize);
        setSine();
    };
    ~sjf_oscillator(){};
    
    void setFrequency(float f)
    {
        initialise(m_SR, f);
    };
    
    void initialise(int sampleRate, float f)
    {
        m_SR = sampleRate;
        m_frequency = f;
        m_readIncrement = m_frequency * m_waveTableSize / m_SR ;
    };
    
    void setSine()
    {
        for (int index = 0; index< m_waveTableSize; index++)
        {
            m_waveTable[index] = sin( index * 2 * PI / m_waveTableSize ) ;
        }
    };
    
    std::vector<float> outputBlock(int numSamples)
    {
        m_outBuff.resize( numSamples ) ;
        for ( int index = 0; index < numSamples; index++ )
        {
            m_outBuff[index] = cubicInterpolate(m_waveTable, m_readPos);
            m_readPos += m_readIncrement;
            if (m_readPos >= m_waveTableSize)
            {
                m_readPos -= m_waveTableSize;
            }
        }
        return m_outBuff;
    };
    
    std::vector<float> outputBlock(int numSamples, float gain)
    {
        m_outBuff.resize( numSamples ) ;
        for ( int index = 0; index < numSamples; index++ )
        {
            m_outBuff[index] = cubicInterpolate(m_waveTable, m_readPos) * gain;
            m_readPos += m_readIncrement;
            if (m_readPos >= m_waveTableSize)
            {
                m_readPos -= m_waveTableSize;
            }
        }
        return m_outBuff;
    };
    
    float outputSample(int numSamples)
    {
        float out = cubicInterpolate(m_waveTable, m_readPos);
        m_readPos += numSamples * m_readIncrement;
        if (m_readPos >= m_waveTableSize)
        {
            m_readPos -= m_waveTableSize;
        }
        return out;
    };
    
private:
    float m_waveTableSize = 512;
    float m_SR = 44100;
    float m_readPos = 0;
    float m_frequency = 440;
    float m_readIncrement = ( m_frequency * m_SR ) / m_waveTableSize;
    std::vector<float> m_outBuff, m_waveTable;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (sjf_oscillator)
};

#endif /* sjf_oscillator_h */
