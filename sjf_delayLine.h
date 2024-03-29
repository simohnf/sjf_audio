//
//  sjf_delayLine.h
//
//  Created by Simon Fay on 23/01/2023.
//

#ifndef sjf_delayLine_h
#define sjf_delayLine_h
#include "sjf_audioUtilities.h"
//#include "sjf_interpolationTypes.h"
#include "sjf_interpolators.h"
#include <JuceHeader.h>
#include <limits>
#include <vector>

//------------------------------------------------
//------------------------------------------------
//------------------------------------------------
template< class T, int NTAPS = 1 >
class sjf_delayLine
{
protected:
    std::vector< T > m_delayLine;
    T m_delayTimeInSamps = 0.0f;
    int m_writePos = 0, m_interpolationType = 1, m_delayLineSize = 0;
    bool m_clearFlag = true;
    
//    static constexpr T m_min = std::is_floating_point_v
private:
    std::array< sjf_interpolators::sjf_allpassInterpolator< T >, NTAPS + 1 >  m_apInterp;
    std::array< T, NTAPS > m_tapDelays;
    static constexpr T MINIMUMVAL = std::numeric_limits<T>::min();
public:
    sjf_delayLine() { };
    ~sjf_delayLine() {};
    
    void initialise( const int MaxDelayInSamps )
    {
        if ( m_delayLineSize != MaxDelayInSamps )
        {
            m_delayLineSize = MaxDelayInSamps;
            m_delayLine.resize( m_delayLineSize );
            m_delayLine.shrink_to_fit();
            std::fill(m_delayLine.begin(), m_delayLine.end(), 0);
        }
    }
    
    void setDelayTimeSamps( const T delayInSamps )
    {
#ifndef NDEBUG
        assert ( delayInSamps <= m_delayLineSize );
#endif
        m_delayTimeInSamps = delayInSamps;
        m_apInterp[ 0 ].setMu( m_delayTimeInSamps - static_cast<int>(m_delayTimeInSamps) );
//        for ( auto& ap : m_apInterp )
//            ap.setMu( m_delayTimeInSamps - static_cast<int>(m_delayTimeInSamps) );
    }
    
    const T size()
    { return m_delayLine.size(); }
    
    const T getDelayTimeSamps(  )
    { return m_delayTimeInSamps; }
    
    const T getSample( const int indexThroughCurrentBuffer )
    {
        T findex = m_writePos + indexThroughCurrentBuffer - m_delayTimeInSamps;
        findex = findex < 0.0 ? findex + m_delayLineSize : findex;
//        fastMod3< T >( findex, m_delayLineSize );
        
        int index = findex;
        T mu = findex - index;  // fractional part between step 1 & 2
        if ( mu <= MINIMUMVAL )
            return m_delayLine[ index ];
        
        return getSampleValue( index, mu, 0 );
    }
    
    const T getSample2( )
    {
        T findex = m_writePos - m_delayTimeInSamps;
        findex = findex < 0.0 ? findex + m_delayLineSize : findex;
//        fastMod3< T >( findex, m_delayLineSize );
        
        int index = findex;
        T mu = findex - index;  // fractional part between step 1 & 2
        if ( mu <= MINIMUMVAL )
            return m_delayLine[ index ];
        
        return getSampleValue( index, mu, 0 );
    }
    
    const T getSampleRoundedIndex( const int &indexThroughCurrentBuffer )
    {
        int readPos =  m_writePos + indexThroughCurrentBuffer - m_delayTimeInSamps ;
        readPos = readPos < 0 ? readPos + m_delayLineSize : readPos;
//        fastMod3< int >( readPos, m_delayLineSize );
        return m_delayLine[ readPos ];
    }
    
    const T getSampleRoundedIndex2( )
    {
        int readPos =  m_writePos - m_delayTimeInSamps ;
        readPos = readPos < 0 ? readPos + m_delayLineSize : readPos;
//        fastMod3< int >( readPos, m_delayLineSize );
        return m_delayLine[ readPos ];
    }
    
    void setSample( const int &indexThroughCurrentBuffer, const T &value )
    {
        auto wp = m_writePos + indexThroughCurrentBuffer;
        fastMod3< int >( wp, m_delayLineSize );
        m_delayLine[ wp ]  = value;
        if ( m_clearFlag ){ m_clearFlag = false; }
    }
    
    void setSample2( const T &value )
    {
        if ( m_delayLineSize <= 0 ){ return; }
        fastMod3< int >( m_writePos, m_delayLineSize);
        m_delayLine[ m_writePos ]  = value;
        m_writePos++;
        if ( m_clearFlag ){ m_clearFlag = false; }
    }
    
    
    const int getWritePosition ()
    { return m_writePos; }
    
    const T getSampleAtIndex( const int& index )
    { return m_delayLine[ index ]; }

    int updateBufferPosition( const int &bufferSize )
    {
        //    Update write position ensuring it stays within size of delay buffer
        m_writePos += bufferSize;
        while ( m_writePos >= m_delayLineSize ) { m_writePos -= m_delayLineSize; }
        return m_writePos;
    }
    
    void setInterpolationType( const int &interpolationType )
    {
        m_interpolationType = interpolationType;
    }
    
    void clearDelayline()
    {
        if( !m_clearFlag )
        {
            std::fill(m_delayLine.begin(), m_delayLine.end(), 0);
            for (auto& ap : m_apInterp )
                ap.reset();
            m_clearFlag = true;
        }
    }
    
    T tapDelayLine( const T delayInSamps, int tapNumber )
    {
        T findex = m_writePos - delayInSamps;
        findex = findex < 0.0 ? findex + m_delayLineSize : findex;
//        fastMod3< T >( findex, m_delayLineSize );
        int index = findex;
        T mu = findex - index;  // fractional part between step 1 & 2
        if ( mu <= MINIMUMVAL )
            return m_delayLine[ index ];
        setTapTime( tapNumber, delayInSamps );
        return getSampleValue( index, mu, tapNumber + 1 );
    }
    
    T tapDelayLine( int tapNumber )
    {
        T findex = m_writePos - m_tapDelays[ tapNumber ];
        findex = findex < 0.0 ? findex + m_delayLineSize : findex;
//        fastMod3< T >( findex, m_delayLineSize );
        int index = findex;
        T mu = findex - index;  // fractional part between step 1 & 2
        if ( mu <= MINIMUMVAL )
            return m_delayLine[ index ];
        
        return getSampleValue( index, mu, tapNumber + 1 );
    }
    
    void setTapTime( int tapNumber, const T delayInSamps )
    {
        m_apInterp[ tapNumber + 1 ].setMu( delayInSamps - static_cast< int >( delayInSamps ) );
        m_tapDelays[ tapNumber ] = delayInSamps;
    }
    
private:
    T getSampleValue( int index, T mu, int tapNumber  )
    {
        if ( m_interpolationType == sjf_interpolators::interpolatorTypes::allpass )
        {
            return m_apInterp[ tapNumber ].process( m_delayLine[ index ] );
        }
        T y1; // this step value
        T y2; // next step value
        if ( m_interpolationType == sjf_interpolators::interpolatorTypes::linear )
        {
            y1 = m_delayLine[ index ];
            index++;
            index = index > m_delayLineSize ? index - m_delayLineSize : index;
//            fastMod3( ++index, m_delayLineSize );
            y2 = m_delayLine[ index ];
            return sjf_interpolators::linearInterpolate< T >( mu, y1, y2 );
        }
        T y0; // previous step value
        T y3; // next next step value
        
        y0 = ( index == 0 ) ? m_delayLine[ m_delayLineSize - 1 ] : m_delayLine[ index - 1 ];
//        y0 = m_delayLine[ index ];
//        fastMod3( ++index, m_delayLineSize );
        y1 = m_delayLine[ index ];
//        fastMod3( ++index, m_delayLineSize );
        index++;
        index = index > m_delayLineSize ? index - m_delayLineSize : index;
        y2 = m_delayLine[ index ];
//        fastMod3( ++index, m_delayLineSize );
        index++;
        index = index > m_delayLineSize ? index - m_delayLineSize : index;
        y3 = m_delayLine[ index ];
        
        switch ( m_interpolationType )
        {
            case sjf_interpolators::interpolatorTypes::linear:
                return sjf_interpolators::linearInterpolate< T >( mu, y1, y2 );
            case sjf_interpolators::interpolatorTypes::cubic:
                return sjf_interpolators::cubicInterpolate< T >( mu, y0, y1, y2, y3 );
            case sjf_interpolators::interpolatorTypes::pureData:
                return sjf_interpolators::fourPointInterpolatePD< T >( mu, y0, y1, y2, y3 );
            case sjf_interpolators::interpolatorTypes::fourthOrder:
                return sjf_interpolators::fourPointFourthOrderOptimal< T >( mu, y0, y1, y2, y3 );
            case sjf_interpolators::interpolatorTypes::godot:
                return sjf_interpolators::cubicInterpolateGodot< T >( mu, y0, y1, y2, y3 );
            case sjf_interpolators::interpolatorTypes::hermite:
                return sjf_interpolators::cubicInterpolateHermite2< T >( mu, y0, y1, y2, y3 );
            default:
                return sjf_interpolators::linearInterpolate< T >( mu, y1, y2 );
        }
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
    T m_delayInSamps, m_invDelInSamps, m_rampLenSamps = 100.0f, m_nRampSegments = 100.0f;
    int m_revCount = 0, m_writePos;
    
public:
    sjf_reverseDelay( ) { }
    ~sjf_reverseDelay( ) { }
    
    void initialise( const T& maxDelayInSamps )
    {
        m_delayLine.initialise( maxDelayInSamps );
    }
    
    void initialise( const T& maxDelayInSamps, const T& rampLenInSamps )
    {
        m_delayLine.initialise( maxDelayInSamps );
        setRampLength( rampLenInSamps );
    }
    
    void setDelayTimeSamps( const T& delayInSamps )
    {
        m_delayInSamps = delayInSamps;
        m_invDelInSamps = 1 / m_delayInSamps;
        m_delayLine.setDelayTimeSamps( delayInSamps );
        calculateNRampSegments();
    }
    
    void setRampLength( const T& rampLenInSamps )
    {
        m_rampLenSamps = rampLenInSamps;
        calculateNRampSegments();
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
        T amp = phaseEnvelope< T >( static_cast<T>(m_revCount) * m_invDelInSamps, m_nRampSegments );
        m_revCount++;
        if ( m_revCount >= m_delayInSamps )
            m_revCount = 0;
        return m_delayLine.getSampleAtIndex( index ) * amp;
    }
    
private:
    
    void calculateNRampSegments()
    {
        m_nRampSegments = m_delayInSamps / m_rampLenSamps;
        if ( m_nRampSegments < 2 )
        { m_nRampSegments = 2; }
//        DBG( "delay " << m_delayInSamps << " ramp " << m_rampLenSamps << " segments " << m_nRampSegments );
    }
    

};
#endif /* sjf_delayLine_h */
