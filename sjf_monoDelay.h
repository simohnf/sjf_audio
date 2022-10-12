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
    
    void initialise( int sampleRate , int sizeMS )
    {
        m_SR = sampleRate;
        int size = round(m_SR * 0.001 * sizeMS);
        m_delayBufferSize = size;
        m_delayLine.resize( m_delayBufferSize
                           , 0 );
    }
    
    void setDelayTime( float delayInMS )
    {
        m_delayTimeInSamps = delayInMS * m_SR * 0.001;
    }
    
    float getDelayTimeMS()
    {
        return m_delayTimeInSamps / ( m_SR * 0.001 );
    }
    void writeToBuffer( std::vector<float> &sourceBuffer )
    {
        auto bufferSize = sourceBuffer.size();
        for (int index = 0; index < bufferSize; index++)
        {
            auto wp = (m_writePos + index) % m_delayBufferSize;
            m_delayLine[wp] = sourceBuffer[ index ];
        }
    }
    
    void writeToBuffer( std::vector<float> &sourceBuffer, float gain )
    {
        auto bufferSize = sourceBuffer.size();
        for (int index = 0; index < bufferSize; index++)
        {
            auto wp = (m_writePos + index) % m_delayBufferSize;
            m_delayLine[wp] = sourceBuffer[ index ] * gain;
        }
    }
    
    void addToBuffer( std::vector<float> &sourceBuffer, float gain )
    {
        auto bufferSize = sourceBuffer.size();
        for (int index = 0; index < bufferSize; index++)
        {
            auto wp = (m_writePos + index) % m_delayBufferSize;
            m_delayLine[ wp ]  += sourceBuffer[ index ] * gain;
        }
    }
    
    
    void addFromBuffer( std::vector<float> &destinationBuffer, float gain)
    {
        auto bufferSize = destinationBuffer.size();
        for (int index = 0; index < bufferSize; index++)
        {
            auto readPos = (int)(m_delayBufferSize + m_writePos - m_delayTimeInSamps + index) % m_delayBufferSize;
            destinationBuffer[ index ] += m_delayLine [ readPos ] * gain;
        }
    };
    
    
    void copyFromBuffer( std::vector<float> &destinationBuffer )
    {
        auto bufferSize = destinationBuffer.size();
        for (int index = 0; index < bufferSize; index++)
        {
            int readPos = m_writePos - m_delayTimeInSamps + index;
            while ( readPos < 0 ) { readPos += m_delayBufferSize; }
            while (readPos >= m_delayBufferSize) { readPos -= m_delayBufferSize; }
            auto val = m_delayLine [ readPos ];
            destinationBuffer[ index ] =  val;
        }
    };
    
    float getSample( int indexThroughCurrentBuffer )
    {
        float readPos = m_delayBufferSize + m_writePos - m_delayTimeInSamps + indexThroughCurrentBuffer;
        while (readPos >= m_delayBufferSize) { readPos -= m_delayBufferSize; }
//        return m_delayLine[ readPos ];
        return cubicInterpolateGodot( m_delayLine, readPos );
//        return fourPointFourthOrderOptimal( m_delayLine, readPos );
//        return fourPointInterpolatePD( m_delayLine, readPos );
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
    float m_delayTimeInSamps, m_SR = 44100;
    int m_writePos = 0, m_delayBufferSize;
    std::vector<float> m_delayLine;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_monoDelay )
};

#endif /* sjf_monoDelay_h */
