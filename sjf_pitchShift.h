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
template < class T >
class sjf_pitchShift
{
    sjf_delayLine< T > m_pitchShifter;
    sjf_phasor< T > m_pitchPhasor;
    T m_windowSize = 4410, m_transpositionCalculationFactor, m_initialDelay = 0.0f;
    
    
public:
    sjf_pitchShift() { }
    ~sjf_pitchShift() { }
    
    
    void initialise( const int &sampleRate, const T &windowSizeMS )
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
    
    void initialise( const T &sampleRate, const T &windowSizeMS, const T &maxDelayMS )
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
    T pitchShiftOutput( const int &indexThroughCurrentBuffer, T transposition )
    {
        // f = (t-1)* R/s
        transposition -= 1.0f;
        transposition *= m_transpositionCalculationFactor;
//        DBG( "transposition " << transposition );
        m_pitchPhasor.setFrequency( transposition );
        // first voice
        auto phase = m_pitchPhasor.output();
        m_pitchShifter.setDelayTimeSamps( ( phase * m_windowSize ) + m_initialDelay );
        auto val = juce::dsp::FastMathApproximations::sin( M_PI * phase ) * m_pitchShifter.getSample2( );
        // second voice
        phase += 0.5f;
        if ( phase >= 1.0f ) { phase -= 1.0f; }
        m_pitchShifter.setDelayTimeSamps( ( phase * m_windowSize ) + m_initialDelay );
        val += ( juce::dsp::FastMathApproximations::sin( M_PI * phase ) * m_pitchShifter.getSample2( ) );
        return ( val );
    }
    
    void setSample( const int &indexThroughCurrentBuffer, const T &inVal )
    {
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
    
    void setDelayTimeSamps( const T &delayInSamps )
    {
        m_initialDelay = delayInSamps;
    }
    
    void clearDelayline()
    {
        m_pitchShifter.clearDelayline();
    }
private:
    
    void setTransopositionFactor( const T &sampleRate, const T &windowSize )
    {
        DBG( "SR / WindowSize " << (sampleRate / windowSize) );
        m_transpositionCalculationFactor = -1.0f * sampleRate / windowSize ; // f = (t-1)* R/s
    }
    
    
//    void setTransopositionFactor( T windowSize )
//    {
//        m_transpositionCalculationFactor = -1.0f  / ( windowSize * 0.001f ); // f = (t-1)* R/s
//    }
//
};
#endif /* sjf_pitchShif_h */

