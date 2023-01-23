//
//  sjf_delayLine.h
//
//  Created by Simon Fay on 23/01/2023.
//

#ifndef sjf_delayLine_h
#define sjf_delayLine_h
#include "sjf_audioUtilities.h"
#include "sjf_interpolationTypes.h"


#include <vector>

template< class floatType >
class sjf_delayLine
{
protected:
    std::vector< floatType > m_delayLine;
    floatType m_delayTimeInSamps = 0.0f;
    int m_writePos = 0, m_interpolationType = 1, m_delayLineSize;
public:
    sjf_delayLine() { };
    ~sjf_delayLine() {};
    
    void initialise( const int &MaxDelayInSamps )
    {
        m_delayLineSize = MaxDelayInSamps;
        m_delayLine.resize( m_delayLineSize );
        m_delayLine.shrink_to_fit();
    }
    
    void setDelayTimeSamps( const floatType &delayInSamps )
    {
        m_delayTimeInSamps = delayInSamps;
    }
    
    
    inline floatType getSample( const int &indexThroughCurrentBuffer )
    {
        floatType readPos = m_writePos + indexThroughCurrentBuffer - m_delayTimeInSamps;
        fastMod3< floatType >( readPos, m_delayLineSize );
        switch ( m_interpolationType )
        {
            case 1:
                return linearInterpolate( m_delayLine, readPos, m_delayLineSize );
            case 2:
                return cubicInterpolate( m_delayLine, readPos, m_delayLineSize );
            case 3:
                return fourPointInterpolatePD( m_delayLine, readPos, m_delayLineSize );
            case 4:
                return fourPointFourthOrderOptimal( m_delayLine, readPos, m_delayLineSize );
            case 5:
                return cubicInterpolateGodot( m_delayLine, readPos, m_delayLineSize );
            case 6:
                return cubicInterpolateHermite( m_delayLine, readPos, m_delayLineSize );
            default:
                return linearInterpolate( m_delayLine, readPos, m_delayLineSize );
        }
    }
    
    inline floatType getSampleRoundedIndex( const int &indexThroughCurrentBuffer )
    {
        auto readPos = round( m_writePos + indexThroughCurrentBuffer - m_delayTimeInSamps );
        fastMod3< floatType >( readPos, m_delayLineSize );
        return m_delayLine[ readPos ];
    }
    
    inline void setSample( const int &indexThroughCurrentBuffer, const floatType &value )
    {
        auto wp = m_writePos + indexThroughCurrentBuffer;
        fastMod3< int >( wp, m_delayLineSize );
        m_delayLine[ wp ]  = value;
    }
    
    int updateBufferPositions( const int &bufferSize )
    {
        //    Update write position ensuring it stays within size of delay buffer
        m_writePos += bufferSize;
        while ( m_writePos >= m_delayLineSize ) { m_writePos -= m_delayLineSize; }
        return m_writePos;
    }
    
    void setInterpolationType( int interpolationType ) { m_interpolationType = interpolationType; }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_delayLine )
};

//------------------------------------------------------------------------
//------------------------------------------------------------------------
//------------------------------------------------------------------------
//------------------------------------------------------------------------

template <class floatType, int NUM_CHANNELS>
class sjf_multiDelay
{
protected:
//    floatType m_SR = 44100, m_maxSizeMS;
//    std::array< floatType, NUM_CHANNELS > m_delayTimeInSamps, m_delayTimeMS;
    int m_writePos = 0;
    std::array< sjf_delayLine< floatType >, NUM_CHANNELS > m_delayLines;
    
    
    
public:
    sjf_multiDelay( ) { };
    ~sjf_multiDelay( ) { };
    
    
    void initialise( const int &sampleRate , const floatType &sizeMS )
    {
        auto size = round(sampleRate * 0.001 * sizeMS) + 1;
        for ( int i = 0; i < NUM_CHANNELS; i++ ) { m_delayLines[ i ].initialise( size ); }
    }
    
    
    void setDelayTimeSamps( const int &channel, const floatType &delayInSamps )
    {
        m_delayLines[ channel ].setDelayTimeSamps( delayInSamps );
    }

    
    inline floatType getSample( const int &channel, const int &indexThroughCurrentBuffer )
    {
        return m_delayLines[ channel ].getSample( indexThroughCurrentBuffer );
    }
    
    inline void popSamplesOutOfDelayLine( const int &indexThroughCurrentBuffer, floatType* whereToPopTo )
    {

        for ( int channel = 0; channel < NUM_CHANNELS; channel++ )
        {
            whereToPopTo[ channel ] = m_delayLines[ channel ].getSample( indexThroughCurrentBuffer );
        }
    }
    
    inline void processAudioInPlaceRoundedIndex( const int &indexThroughCurrentBuffer, floatType* data )
    {
        pushSamplesIntoDelayLine( indexThroughCurrentBuffer, data );
        popSamplesOutOfDelayLineRoundedIndex( indexThroughCurrentBuffer, data );
    }
    
    inline void popSamplesOutOfDelayLineRoundedIndex( const int &indexThroughCurrentBuffer, floatType* whereToPopTo )
    {
        for ( int channel = 0; channel < NUM_CHANNELS; channel++ )
        {

            whereToPopTo[ channel ] = m_delayLines[ channel ].getSampleRoundedIndex( indexThroughCurrentBuffer );
        }
    }
    inline floatType getSampleRoundedIndex( const int &channel, const int &indexThroughCurrentBuffer )
    {
        return m_delayLines[ channel ].getSampleRoundedIndex( indexThroughCurrentBuffer );
    }
    
    
    inline void pushSamplesIntoDelayLine( const int &indexThroughCurrentBuffer, const floatType* valuesToPush )
    {
        for ( int channel = 0; channel < NUM_CHANNELS; channel++ )
        {
            m_delayLines[ channel ].setSample( indexThroughCurrentBuffer,  valuesToPush[ channel ] );
        }
    }
    
    inline void setSample( const int &channel, const int &indexThroughCurrentBuffer, const floatType &value )
    {
        m_delayLines[ channel ].setSample( indexThroughCurrentBuffer,  value );
    }
    
    inline void updateBufferPositions( const int &bufferSize )
    {
        //    Update write position ensuring it stays within size of delay buffer
        for ( int channel = 0; channel < NUM_CHANNELS; channel++ )
        {
            m_delayLines[ channel ].updateBufferPositions( bufferSize );
        }
    }
    
    void setInterpolationType( const int &interpolationType )
    {
        for ( int channel = 0; channel < NUM_CHANNELS; channel++ )
        {
            m_delayLines[ channel ].setInterpolationType( interpolationType );
        }
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_multiDelay )
};

#endif /* sjf_delayLine_h */
