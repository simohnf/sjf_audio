//
//  sjf_delayLine.h
//
//  Created by Simon Fay on 23/01/2023.
//

#ifndef sjf_delayLine_h
#define sjf_delayLine_h
#include "sjf_audioUtilities.h"
#include "sjf_interpolationTypes.h"
#include <JuceHeader.h>

#include <vector>


template< class T >
class sjf_delayLine
{
protected:
    std::vector< T > m_delayLine;
    T m_delayTimeInSamps = 0.0f;
    int m_writePos = 0, m_interpolationType = 1, m_delayLineSize = 0;
    
public:
    sjf_delayLine() { };
    ~sjf_delayLine() {};
    
    void initialise( const int &MaxDelayInSamps )
    {
        if ( m_delayLineSize != MaxDelayInSamps )
        {
            m_delayLineSize = MaxDelayInSamps;
            m_delayLine.resize( m_delayLineSize );
            m_delayLine.shrink_to_fit();
            std::fill(m_delayLine.begin(), m_delayLine.end(), 0);
        }
    }
    
    void setDelayTimeSamps( const T &delayInSamps )
    {
        m_delayTimeInSamps = delayInSamps;
    }
    
    T size()
    {
        return m_delayLine.size();
    }
    
    const T& getDelayTimeSamps(  )
    {
        return m_delayTimeInSamps;
    }
    
    T getSample( const int &indexThroughCurrentBuffer )
    {
        T readPos = m_writePos + indexThroughCurrentBuffer - m_delayTimeInSamps;
        fastMod3< T >( readPos, m_delayLineSize );
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
    
    T getSample2( )
    {
        T readPos = m_writePos - m_delayTimeInSamps;
        fastMod3< T >( readPos, m_delayLineSize );
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
    
    T getSampleRoundedIndex( const int &indexThroughCurrentBuffer )
    {
        int readPos =  m_writePos + indexThroughCurrentBuffer - m_delayTimeInSamps ;
        fastMod3< int >( readPos, m_delayLineSize );
        return m_delayLine[ readPos ];
    }
    
    T getSampleRoundedIndex2( )
    {
        int readPos =  m_writePos - m_delayTimeInSamps ;
        fastMod3< int >( readPos, m_delayLineSize );
        return m_delayLine[ readPos ];
    }
    
    void setSample( const int &indexThroughCurrentBuffer, const T &value )
    {
        auto wp = m_writePos + indexThroughCurrentBuffer;
        fastMod3< int >( wp, m_delayLineSize );
        m_delayLine[ wp ]  = value;
    }
    
    void setSample2( const T &value )
    {
        m_delayLine[ m_writePos ]  = value;
        if ( ++m_writePos >= m_delayLineSize ) { m_writePos = 0; }
    }
    
    int getWritePosition()
    {
        return m_writePos;
    }
    
    T getSampleAtIndex( const int& index )
    {
        return m_delayLine[ index ];
    }
//    int updateBufferPosition( const int &bufferSize )
//    {
//        //    Update write position ensuring it stays within size of delay buffer
//        m_writePos += bufferSize;
//        while ( m_writePos >= m_delayLineSize ) { m_writePos -= m_delayLineSize; }
//        return m_writePos;
//    }
    
    void setInterpolationType( const int &interpolationType )
    {
        m_interpolationType = interpolationType;
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_delayLine )
};

//------------------------------------------------------------------------
//------------------------------------------------------------------------
//------------------------------------------------------------------------
//------------------------------------------------------------------------

template <class T, int NUM_CHANNELS>
class sjf_multiDelay
{
protected:
    
    int m_writePos = 0;
    std::array< sjf_delayLine< T >, NUM_CHANNELS > m_delayLines;
    
    
    
public:
    sjf_multiDelay( ) { };
    ~sjf_multiDelay( ) { };
    
    
    void initialise( const int &sampleRate , const T &sizeMS )
    {
        auto size = round(sampleRate * 0.001 * sizeMS) + 1;
        for ( int channel = 0; channel < NUM_CHANNELS; channel++ )
        {
            m_delayLines[ channel ].initialise( size );
        }
    }
    
    void initialise( const T &sizeInSamps )
    {
        for ( int channel = 0; channel < NUM_CHANNELS; channel++ )
        {
            m_delayLines[ channel ].initialise( sizeInSamps );
        }
    }
    
    void initialiseChannel( const int &channel, const T &sizeInSamps )
    {
        m_delayLines[ channel ].initialise( sizeInSamps );
    }
    
    void size( const int &channel )
    {
        return m_delayLines[ channel ].size();
    }
    
    void setDelayTimeSamps( const int &channel, const T &delayInSamps )
    {
        m_delayLines[ channel ].setDelayTimeSamps( delayInSamps );
    }

    
    T getSample( const int &channel, const int &indexThroughCurrentBuffer )
    {
        return m_delayLines[ channel ].getSample2( );
//        return m_delayLines[ channel ].getSample( indexThroughCurrentBuffer );
    }
    
    void popSamplesOutOfDelayLine( const int &indexThroughCurrentBuffer, std::array< float, NUM_CHANNELS > &whereToPopTo )
    {

        for ( int channel = 0; channel < NUM_CHANNELS; channel++ )
        {
//            whereToPopTo[ channel ] = m_delayLines[ channel ].getSample( indexThroughCurrentBuffer );
            whereToPopTo[ channel ] = m_delayLines[ channel ].getSample2(  );
        }
    }
    
    void processAudioInPlaceRoundedIndex( const int &indexThroughCurrentBuffer, std::array< float, NUM_CHANNELS > &data )
    {
        for ( int channel = 0; channel < NUM_CHANNELS; channel++ )
        {
//            m_delayLines[ channel ].setSample( indexThroughCurrentBuffer,  data[ channel ] );
            m_delayLines[ channel ].setSample2(  data[ channel ] );
            data[ channel ] = m_delayLines[ channel ].getSampleRoundedIndex2(  );
        }
    }
    
    void popSamplesOutOfDelayLineRoundedIndex( const int &indexThroughCurrentBuffer, std::array< float, NUM_CHANNELS > &whereToPopTo )
    {
        for ( int channel = 0; channel < NUM_CHANNELS; channel++ )
        {
            whereToPopTo[ channel ] = m_delayLines[ channel ].getSampleRoundedIndex2( );
        }
    }
    
    
    T getSampleRoundedIndex( const int &channel, const int &indexThroughCurrentBuffer )
    {
        return m_delayLines[ channel ].getSampleRoundedIndex2( );
    }
    
    
    void pushSamplesIntoDelayLine( const int &indexThroughCurrentBuffer, const std::array< float, NUM_CHANNELS > &valuesToPush )
    {
        for ( int channel = 0; channel < NUM_CHANNELS; channel++ )
        {
//            m_delayLines[ channel ].setSample( indexThroughCurrentBuffer,  valuesToPush[ channel ] );
            m_delayLines[ channel ].setSample2( valuesToPush[ channel ] );
        }
    }
    
    void setSample( const int &channel, const int &indexThroughCurrentBuffer, const T &value )
    {
//        m_delayLines[ channel ].setSample( indexThroughCurrentBuffer,  value );
        m_delayLines[ channel ].setSample2( value );
    }
    
//    void updateBufferPositions( const int &bufferSize )
//    {
//        //    Update write position ensuring it stays within size of delay buffer
//        for ( int channel = 0; channel < NUM_CHANNELS; channel++ )
//        {
//            m_delayLines[ channel ].updateBufferPosition( bufferSize );
//        }
//    }
    
    void setInterpolationType( const int &interpolationType )
    {
        for ( int channel = 0; channel < NUM_CHANNELS; channel++ )
        {
            m_delayLines[ channel ].setInterpolationType( interpolationType );
        }
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_multiDelay )
};

//------------------------------------------------------------------------
//------------------------------------------------------------------------
//------------------------------------------------------------------------
//------------------------------------------------------------------------
template< typename T >
class sjf_reverseDelay
{
    sjf_delayLine< T > m_delayLine;
    T m_delayInSamps, m_invDelInSamps;
    int m_revCount = 0, m_writePos;
    
public:
    sjf_reverseDelay( ) { }
    ~sjf_reverseDelay( ) { }
    
    void initialise( const T& maxDelayInSamps )
    {
        m_delayLine.initialise( maxDelayInSamps );
    }
    
    void setDelayTimeSamps( const T& delayInSamps )
    {
        m_delayInSamps = delayInSamps;
        m_invDelInSamps = 1 / m_delayInSamps;
        m_delayLine.setDelayTimeSamps( delayInSamps );
    }
    
    void setSample2( const T& val ){ m_delayLine.setSample2( val ); }
    
    T getSample2( )
    {
        return m_delayLine.getSampleRoundedIndex2( );
    }
    
    T getSampleReverse()
    {
        if ( m_revCount == 0 )
            m_writePos = m_delayLine.getWritePosition() - 1; // always read from behind write pointer
        auto index = m_writePos - m_revCount;
        fastMod3< int >( index, m_delayLine.size() );
        T amp = phaseEnvelope( (T)m_revCount * m_invDelInSamps );
        m_revCount++;
        if ( m_revCount >= m_delayInSamps )
            m_revCount = 0;
        return m_delayLine.getSampleAtIndex( index ) * amp;
    }
    
private:
    T phaseEnvelope( const T& phase )
    {
        T up, down;
        
        up = down = phase * 100;
        clipInPlace< T >( up, 0, 1 );
        
        down -= 99;
        down *= -1;
        clipInPlace< T > ( down, -1, 0 );
        
        return up + down;
    }
};
#endif /* sjf_delayLine_h */
