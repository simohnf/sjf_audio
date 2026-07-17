//
//  sjf_multiDelay.h
//
//  Created by Simon Fay on 19/01/2023.
//  Multi channel delay line
//

#ifndef sjf_multiDelay_h
#define sjf_multiDelay_h
#include "sjf_interpolationTypes.h"

template <class floatType, int NUM_CHANNELS>
class sjf_multiDelay
{
public:
    sjf_multiDelay( )
    {
        for ( int i = 0; i < NUM_CHANNELS; i++ )
        {
            m_delayTimeInSamps[ i ] = 0;
            m_delayTimeMS[ i ] = 0;
        }
    };
    ~sjf_multiDelay( ) { };
    
    void initialise( const int &sampleRate )
    {
        m_SR = sampleRate;
        int size = round(m_SR * 0.001 * m_maxSizeMS);
        m_delayBufferSize = size;
        m_delayLine.resize( m_delayBufferSize * NUM_CHANNELS, 0 );
        m_delayLine.shrink_to_fit();
        setDelayTime( m_delayTimeMS );
    }
    
    
    void initialise( const int &sampleRate , const floatType &sizeMS )
    {
        m_maxSizeMS = sizeMS;
        m_SR = sampleRate;
        int size = round(m_SR * 0.001 * m_maxSizeMS);
        m_delayBufferSize = size;
        m_delayLine.resize( m_delayBufferSize * NUM_CHANNELS, 0 );
        m_delayLine.shrink_to_fit();
    }
    
    void setDelayTime( const int &channel, const floatType &delayInMS )
    {
        if ( m_delayTimeMS[ channel ] == delayInMS ) { return; }
        m_delayTimeMS[ channel ] = delayInMS;
        m_delayTimeInSamps[ channel ] = m_delayTimeMS * m_SR * 0.001f;
    }
    
    void setDelayTimeSamps( const int &channel, const floatType &delayInSamps )
    {
        m_delayTimeInSamps[ channel ] = delayInSamps;
    }
    
    floatType getDelayTimeMS( const int &channel )
    {
        m_delayTimeMS[ channel ] = m_delayTimeInSamps / ( m_SR * 0.001f );
        return m_delayTimeMS[ channel ];
    }
    
    
    floatType getSample( const int &channel, const int &indexThroughCurrentBuffer )
    {
        floatType y0, y1, y2, y3, mu;
        floatType readPos = m_writePos - m_delayTimeInSamps[ channel ] + indexThroughCurrentBuffer;
        while ( readPos < 0 ) { readPos += m_delayBufferSize; }
        while ( readPos >= m_delayBufferSize ) { readPos -= m_delayBufferSize; }
        int index = readPos;
        mu = readPos - index;
        index -= 1;
        fastMod3( index, m_delayBufferSize );
        y0 = m_delayLine[ ( index * NUM_CHANNELS ) + channel ];
        
        index++;
        fastMod3( index, m_delayBufferSize );
        y1 = m_delayLine[ ( index * NUM_CHANNELS ) + channel ];
        
        index++;
        fastMod3( index, m_delayBufferSize );
        y2 = m_delayLine[ ( index * NUM_CHANNELS ) + channel ];
        
        index++;
        fastMod3( index, m_delayBufferSize );
        y3 = m_delayLine[ ( index * NUM_CHANNELS ) + channel ];
        switch ( m_interpolationType )
        {
            case 1:
                return linearInterpolate( mu, y1, y2 );
            case 2:
                return cubicInterpolate( mu, y0, y1, y2, y3 );
            case 3:
                return fourPointInterpolatePD( mu, y0, y1, y2, y3 );
            case 4:
                return fourPointFourthOrderOptimal( mu, y0, y1, y2, y3 );
            case 5:
                return cubicInterpolateGodot( mu, y0, y1, y2, y3 );
            case 6:
                return cubicInterpolateHermite( mu, y0, y1, y2, y3 );
            default:
                return linearInterpolate( mu, y1, y2 );
        }
    }
    

    
    void popSamplesOutOfDelayLine( const int &indexThroughCurrentBuffer, floatType* whereToPopTo )
    {
        int index;
        floatType y0, y1, y2, y3, mu, readPos;
        floatType rp = m_writePos + indexThroughCurrentBuffer;
        for ( int channel = 0; channel < NUM_CHANNELS; channel++ )
        {
            
            readPos = rp - m_delayTimeInSamps[ channel ];
            while ( readPos < 0 ) { readPos += m_delayBufferSize; }
            while ( readPos >= m_delayBufferSize ) { readPos -= m_delayBufferSize; }
            index = readPos;
            mu = readPos - index;
            index -= 1;
            fastMod3( index, m_delayBufferSize );
            y0 = m_delayLine[ ( index * NUM_CHANNELS ) + channel ];
            
            index++;
            fastMod3( index, m_delayBufferSize );
            y1 = m_delayLine[ ( index * NUM_CHANNELS ) + channel ];
            
            index++;
            fastMod3( index, m_delayBufferSize );
            y2 = m_delayLine[ ( index * NUM_CHANNELS ) + channel ];
            
            index++;
            fastMod3( index, m_delayBufferSize );
            y3 = m_delayLine[ ( index * NUM_CHANNELS ) + channel ];
            switch ( m_interpolationType )
            {
                case 1:
                    whereToPopTo[ channel ] =  linearInterpolate( mu, y1, y2 );
                    break;
                case 2:
                    whereToPopTo[ channel ] =  cubicInterpolate( mu, y0, y1, y2, y3 );
                    break;
                case 3:
                    whereToPopTo[ channel ] =  fourPointInterpolatePD( mu, y0, y1, y2, y3 );
                    break;
                case 4:
                    whereToPopTo[ channel ] =  fourPointFourthOrderOptimal( mu, y0, y1, y2, y3 );
                    break;
                case 5:
                    whereToPopTo[ channel ] =  cubicInterpolateGodot( mu, y0, y1, y2, y3 );
                    break;
                case 6:
                    whereToPopTo[ channel ] =  cubicInterpolateHermite( mu, y0, y1, y2, y3 );
                    break;
                default:
                    whereToPopTo[ channel ] =  linearInterpolate( mu, y1, y2 );
                    break;
            }
        }
    }
    
    void processAudioInPlaceRoundedIndex( const int &indexThroughCurrentBuffer, floatType* data )
    {
        pushSamplesIntoDelayLine( indexThroughCurrentBuffer, data );
        popSamplesOutOfDelayLineRoundedIndex( indexThroughCurrentBuffer, data );
    }
    
    void popSamplesOutOfDelayLineRoundedIndex( const int &indexThroughCurrentBuffer, floatType* whereToPopTo )
    {
        int readPos;
        int rp = m_writePos + indexThroughCurrentBuffer;
        for ( int channel = 0; channel < NUM_CHANNELS; channel++ )
        {
            readPos = rp - round( m_delayTimeInSamps[ channel ] );
            fastMod3< int > ( readPos, m_delayBufferSize );
            whereToPopTo[ channel ] = m_delayLine[ (readPos * NUM_CHANNELS) + channel ];
        }
    }
    floatType getSampleRoundedIndex( const int &channel, const int &indexThroughCurrentBuffer )
    {
        int readPos = round( m_writePos - m_delayTimeInSamps[ channel ] + indexThroughCurrentBuffer);
        fastMod3< int > ( readPos, m_delayBufferSize );
        return m_delayLine[ ( readPos * NUM_CHANNELS ) + channel ];
    }
    
    
    void pushSamplesIntoDelayLine( const int &indexThroughCurrentBuffer, const floatType* valuesToPush )
    {
        auto wp = m_writePos + indexThroughCurrentBuffer;
        fastMod3< int >( wp, m_delayBufferSize );
        for ( int channel = 0; channel < NUM_CHANNELS; channel++ )
        {
            m_delayLine[ ( wp * NUM_CHANNELS ) + channel ] = valuesToPush[ channel ];
        }
    }
    
    void setSample( const int &channel, const int &indexThroughCurrentBuffer, const floatType &value )
    {
        auto wp = m_writePos + indexThroughCurrentBuffer;
        fastMod3< int >( wp, m_delayBufferSize );
        m_delayLine[ ( wp * NUM_CHANNELS ) + channel ]  = value;
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
    floatType m_SR = 44100, m_maxSizeMS;
    std::array< floatType, NUM_CHANNELS > m_delayTimeInSamps, m_delayTimeMS;
    int m_writePos = 0, m_delayBufferSize, m_interpolationType = 1;
    std::vector<floatType> m_delayLine;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_multiDelay )
};

#endif /* sjf_multiDelay_h */


