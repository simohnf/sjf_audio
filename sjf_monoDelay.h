//
//  sjf_monoDelay.h
//
//  Created by Simon Fay on 11/10/2022.
//

#ifndef sjf_monoDelay_h
#define sjf_monoDelay_h
#include "sjf_interpolationTypes.h"

class sjf_monoDelay
{
public:
    sjf_monoDelay(){};
    ~sjf_monoDelay(){};
    
    void initialise( int sampleRate )
    {
        m_SR = sampleRate;
        int size = round(m_SR * 0.001 * m_maxSizeMS);
        m_delayBufferSize = size;
        m_delayLine.resize( m_delayBufferSize, 0 );
        setDelayTime( m_delayTimeMS );
    }
    
    
    void initialise( int sampleRate , float sizeMS )
    {
        m_maxSizeMS = sizeMS;
        m_SR = sampleRate;
        int size = round(m_SR * 0.001 * m_maxSizeMS);
        m_delayBufferSize = size;
        m_delayLine.resize( m_delayBufferSize, 0 );
        setDelayTime( m_delayTimeMS );
    }
    
    void setDelayTime( float delayInMS )
    {
        m_delayTimeMS = delayInMS;
        m_delayTimeInSamps = m_delayTimeMS * m_SR * 0.001;
    }
    
    float getDelayTimeMS()
    {
        return m_delayTimeInSamps / ( m_SR * 0.001 );
    }

    
    float getSample( int indexThroughCurrentBuffer )
    {
        float readPos = m_delayBufferSize + m_writePos - m_delayTimeInSamps + indexThroughCurrentBuffer;
        while (readPos >= m_delayBufferSize) { readPos -= m_delayBufferSize; }
//        return m_delayLine[ readPos ];
        return cubicInterpolateGodot( m_delayLine, readPos );
//        return fourPointFourthOrderOptimal( m_delayLine, readPos );
//        return fourPointInterpolatePD( m_delayLine, readPos );
    }
    
    float getSampleRoundedIndex( int indexThroughCurrentBuffer )
    {
        float readPos = round(m_delayBufferSize + m_writePos - m_delayTimeInSamps + indexThroughCurrentBuffer);
        while (readPos >= m_delayBufferSize) { readPos -= m_delayBufferSize; }
        return m_delayLine[ readPos ];
    }
    
    void setSample( int indexThroughCurrentBuffer, float value )
    {
        auto wp = m_writePos + indexThroughCurrentBuffer + m_delayBufferSize;
        wp %= m_delayBufferSize;
        m_delayLine[ wp ]  = value;
    }
    
    int updateBufferPositions(int bufferSize)
    {
        //    Update write position ensuring it stays within size of delay buffer
        m_writePos += bufferSize;
        while ( m_writePos >= m_delayBufferSize )
        {
            m_writePos -= m_delayBufferSize;
        }
        return m_writePos;
    };

private:
    float m_delayTimeInSamps, m_SR = 44100, m_delayTimeMS, m_maxSizeMS;
    int m_writePos = 0, m_delayBufferSize;
    std::vector<float> m_delayLine;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_monoDelay )
};

#endif /* sjf_monoDelay_h */
