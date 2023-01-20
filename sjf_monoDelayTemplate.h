//
//  sjf_monoDelayTemplate.h
//
//  Created by Simon Fay on 18/01/2023.
//

//#ifndef sjf_monoDelayTemplate_h
#define sjf_monoDelayTemplate_h
#include "sjf_interpolationTypes.h"

template<class floatType, int MAXIMUM_SIZE>
class sjf_monoDelayTemplate
{
public:
    sjf_monoDelayTemplate() { };
    ~sjf_monoDelayTemplate() {};
    
    void initialise( const int &sampleRate )
    {
        m_SR = sampleRate;
        setDelayTime( m_delayTimeMS );
    }
    
    void setDelayTime( const floatType &delayInMS )
    {
        if ( m_delayTimeMS == delayInMS ) { return; }
        m_delayTimeMS = delayInMS;
        m_delayTimeInSamps = m_delayTimeMS * m_SR * 0.001f;
    }
    
    void setDelayTimeSamps( const floatType &delayInSamps )
    {
        m_delayTimeInSamps = delayInSamps;
        m_delayTimeMS = m_delayTimeInSamps / ( m_SR * 0.001f );
//        DBG( m_delayTimeInSamps << " " << m_delayTimeMS );
    }
    
    floatType getDelayTimeMS() { return m_delayTimeMS; }
    
    
    floatType getSample( const int &indexThroughCurrentBuffer )
    {
        floatType readPos = m_writePos - m_delayTimeInSamps + indexThroughCurrentBuffer;
        fastMod3< int >( readPos, MAXIMUM_SIZE );
        switch ( m_interpolationType )
        {
            case 1:
                return linearInterpolate( m_delayLine, readPos, MAXIMUM_SIZE );
            case 2:
                return cubicInterpolate( m_delayLine, readPos, MAXIMUM_SIZE );
            case 3:
                return fourPointInterpolatePD( m_delayLine, readPos, MAXIMUM_SIZE );
            case 4:
                return fourPointFourthOrderOptimal( m_delayLine, readPos, MAXIMUM_SIZE );
            case 5:
                return cubicInterpolateGodot( m_delayLine, readPos, MAXIMUM_SIZE );
            case 6:
                return cubicInterpolateHermite( m_delayLine, readPos, MAXIMUM_SIZE );
            default:
                return linearInterpolate( m_delayLine, readPos, MAXIMUM_SIZE );
        }
    }
    
    floatType getSampleRoundedIndex( const int &indexThroughCurrentBuffer )
    {
        auto readPos = round( m_writePos - m_delayTimeInSamps + indexThroughCurrentBuffer);
        fastMod3< int >( readPos, MAXIMUM_SIZE );
//        while ( readPos < 0 ) { readPos += MAXIMUM_SIZE; }
//        readPos = fastMod2( readPos, MAXIMUM_SIZE );
        return m_delayLine[ readPos ];
    }
    
    void setSample( const int &indexThroughCurrentBuffer, const floatType &value )
    {
        auto wp = m_writePos + indexThroughCurrentBuffer;
        fastMod3< int >( wp, MAXIMUM_SIZE );
//        if ( wp < 0 ) { wp += MAXIMUM_SIZE; }
//        else { wp = fastMod2 ( wp, MAXIMUM_SIZE ); }
        m_delayLine[ wp ]  = value;
    }
    
    int updateBufferPositions( int bufferSize )
    {
        //    Update write position ensuring it stays within size of delay buffer
        m_writePos += bufferSize;
        while ( m_writePos >= MAXIMUM_SIZE ) { m_writePos -= MAXIMUM_SIZE; }
        return m_writePos;
    }
    
    void setInterpolationType( int interpolationType ) { m_interpolationType = interpolationType; }
    
protected:
    floatType m_delayTimeInSamps = 0.0f, m_SR = 44100, m_delayTimeMS = 0.0f;
    int m_writePos = 0, m_interpolationType = 1;
    floatType m_delayLine[ MAXIMUM_SIZE ] = { static_cast< floatType > (0) };;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_monoDelayTemplate )
};
