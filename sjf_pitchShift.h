//
//  sjf_pitchShif.h
//
//  Created by Simon Fay on 26/01/2023.
//

#ifndef sjf_pitchShif_h
#define sjf_pitchShif_h

#include "sjf_delayLine.h"
#include "sjf_phasor.h"
#include <JuceHeader.h>
#include <cmath>

//------------------------------------------------------------------------
//------------------------------------------------------------------------
//------------------------------------------------------------------------
//------------------------------------------------------------------------
template < class floatType >
class sjf_pitchShift
{
    sjf_delayLine< floatType > m_pitchShifter;
    sjf_phasor< floatType > m_pitchPhasor;
    floatType m_windowSize = 4410, m_transpositionCalculationFactor, m_initialDelay = 0.0f;
    
public:
    sjf_pitchShift() { }
    ~sjf_pitchShift() { }
    
    
    void initialise( const int &sampleRate, const floatType &windowSizeMS )
    {
//        m_windowSize = windowSizeMS;
        DBG( "windowSizeMS " << windowSizeMS );
        DBG( "sampleRate " << sampleRate );
        m_windowSize = sampleRate * 0.001f * windowSizeMS;
        DBG(" maxDelayInSamps " << m_windowSize );
        setTransopositionFactor( sampleRate, m_windowSize );
        m_pitchShifter.initialise( (int)m_windowSize );
        m_pitchPhasor.initialise( sampleRate, 1.0f );
    }
    
    void initialise( const floatType &sampleRate, const floatType &windowSizeMS, const floatType &maxDelayMS )
    {
        if ( maxDelayMS <= windowSizeMS )
        {
            initialise( sampleRate, windowSizeMS );
        }
        else
        {
            m_windowSize = sampleRate * 0.001f * windowSizeMS;
            auto maxDelayInSamps = sampleRate * 0.001f * maxDelayMS;
            setTransopositionFactor( sampleRate, m_windowSize );
            m_pitchShifter.initialise( maxDelayInSamps );
            m_pitchPhasor.initialise( sampleRate, 1.0f );
        }
    }
    // transposition should be calculated as multiple (i.e. 2 would be an octave up, 0.5 an octave down)
    floatType pitchShiftOutput( const int &indexThroughCurrentBuffer, floatType transposition )
    {
        // f = (t-1)* R/s
        transposition -= 1.0f;
        transposition *= m_transpositionCalculationFactor;
//        DBG( "transposition " << transposition );
        m_pitchPhasor.setFrequency( transposition );
        // first voice
        auto phase = m_pitchPhasor.output();
        auto pitchShiftTimeDiff = phase * m_windowSize;
        pitchShiftTimeDiff += m_initialDelay;
        m_pitchShifter.setDelayTimeSamps( pitchShiftTimeDiff + m_initialDelay );
        auto amp = juce::dsp::FastMathApproximations::sin( M_PI * phase );
        auto val = amp * m_pitchShifter.getSample2( );
        // second voice
        phase += 0.5f;
        if ( phase >= 1.0f ) { phase -= 1.0f; }
        pitchShiftTimeDiff = phase * m_windowSize;
        pitchShiftTimeDiff += m_initialDelay;
        m_pitchShifter.setDelayTimeSamps( pitchShiftTimeDiff );
        amp = juce::dsp::FastMathApproximations::sin( M_PI * phase );
        val += ( amp * m_pitchShifter.getSample2( ) );
        return ( val );
    }
    
    void setSample( const int &indexThroughCurrentBuffer, const floatType &inVal )
    {
//        m_pitchShifter.setSample( indexThroughCurrentBuffer, inVal );
        m_pitchShifter.setSample2( inVal );
    }
    
    void setInterpolationType( const int &interpolationType )
    {
        m_pitchShifter.setInterpolationType( interpolationType );
    }
    
    void updateBufferPosition( const int &bufferSize )
    {
        m_pitchShifter.updateBufferPosition( bufferSize );
    }
    
    void setDelayTimeSamps( const floatType &delayInSamps )
    {
        m_initialDelay = delayInSamps;
    }
    
private:
    
    void setTransopositionFactor( const floatType &sampleRate, const floatType &windowSize )
    {
        DBG( "SR / WindowSize " << (sampleRate / windowSize) );
        m_transpositionCalculationFactor = -1.0f * sampleRate / windowSize ; // f = (t-1)* R/s
    }
    
    
//    void setTransopositionFactor( float windowSize )
//    {
//        m_transpositionCalculationFactor = -1.0f  / ( windowSize * 0.001f ); // f = (t-1)* R/s
//    }
//
};
#endif /* sjf_pitchShif_h */

