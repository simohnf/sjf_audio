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
    sjf_monoDelay( ) { };
    ~sjf_monoDelay( ) { };
    
    void initialise( int sampleRate )
    {
        m_SR = sampleRate;
        int size = round(m_SR * 0.001 * m_maxSizeMS);
        m_delayBufferSize = size;
        m_delayLine.resize( m_delayBufferSize, 0 );
        m_delayLine.shrink_to_fit();
        
        setDelayTime( m_delayTimeMS );
    }
    
    
    void initialise( int sampleRate , float sizeMS )
    {
        m_maxSizeMS = sizeMS;
        m_SR = sampleRate;
        int size = round(m_SR * 0.001 * m_maxSizeMS);
        m_delayBufferSize = size;
        m_delayLine.resize( m_delayBufferSize, 0 );
        m_delayLine.shrink_to_fit();
        setDelayTime( m_delayTimeMS );
        
    }
    
    void setDelayTime( float delayInMS )
    {
        if ( m_delayTimeMS == delayInMS ) { return; }
        m_delayTimeMS = delayInMS;
        m_delayTimeInSamps = m_delayTimeMS * m_SR * 0.001f;
    }
    
    void setDelayTimeSamps( float delayInSamps )
    {
        m_delayTimeInSamps = delayInSamps;
//        m_delayTimeMS = m_delayTimeInSamps / ( m_SR * 0.001f );
//        DBG( m_delayTimeInSamps << " " << m_delayTimeMS );
    }
    
    float getDelayTimeMS()
    {
        m_delayTimeMS = m_delayTimeInSamps / ( m_SR * 0.001f );
        return m_delayTimeMS;
    }

    
    float getSample( const int &indexThroughCurrentBuffer )
    {
        float readPos = m_writePos - m_delayTimeInSamps + indexThroughCurrentBuffer;
        while ( readPos < 0 ) { readPos += m_delayBufferSize; }
        while ( readPos >= m_delayBufferSize ) { readPos -= m_delayBufferSize; }
        switch ( m_interpolationType )
        {
            case 1:
                return linearInterpolate( m_delayLine, readPos, m_delayBufferSize );
            case 2:
                return cubicInterpolate( m_delayLine, readPos, m_delayBufferSize );
            case 3:
                return fourPointInterpolatePD( m_delayLine, readPos, m_delayBufferSize );
            case 4:
                return fourPointFourthOrderOptimal( m_delayLine, readPos, m_delayBufferSize );
            case 5:
                return cubicInterpolateGodot( m_delayLine, readPos, m_delayBufferSize );
            case 6:
                return cubicInterpolateHermite( m_delayLine, readPos, m_delayBufferSize );
            default:
                return linearInterpolate( m_delayLine, readPos, m_delayBufferSize );
        }
    }
    
    float getSampleRoundedIndex( const int &indexThroughCurrentBuffer )
    {
        int readPos = round( m_writePos - m_delayTimeInSamps + indexThroughCurrentBuffer);
        fastMod3< int > ( readPos, m_delayBufferSize );
//        while ( readPos < 0 ) { readPos += m_delayBufferSize; }
//        readPos = fastMod2( readPos, m_delayBufferSize );
        return m_delayLine[ readPos ];
    }
    
    
    
    void setSample( const int &indexThroughCurrentBuffer, const float &value )
    {
        auto wp = m_writePos + indexThroughCurrentBuffer;
        fastMod3< int >( wp, m_delayBufferSize );
//        if ( wp < 0 ) { wp += m_delayBufferSize; }
//        else { wp = fastMod2 ( wp, m_delayBufferSize ); }
        m_delayLine[ wp ]  = value;
    }
    
    int updateBufferPositions( int bufferSize )
    {
        //    Update write position ensuring it stays within size of delay buffer
        m_writePos += bufferSize;
        while ( m_writePos >= m_delayBufferSize ) { m_writePos -= m_delayBufferSize; }
        return m_writePos;
    }
    
    void setInterpolationType( int interpolationType )
    {
        m_interpolationType = interpolationType;
    }

protected:
    float m_delayTimeInSamps = 0.0f, m_SR = 44100, m_delayTimeMS = 0.0f, m_maxSizeMS;
    int m_writePos = 0, m_delayBufferSize, m_interpolationType = 1;
    std::vector<float> m_delayLine;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_monoDelay )
};

#endif /* sjf_monoDelay_h */
