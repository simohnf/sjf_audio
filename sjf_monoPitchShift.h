//
//  sjf_monoPitchShift.h
//
//  Created by Simon Fay on 24/08/2022.
//

#ifndef sjf_monoPitchShift_h
#define sjf_monoPitchShift_h

#include <JuceHeader.h>
#include "sjf_monoDelay.h"
#include "sjf_phasor.h"
#include "sjf_lpf.h"


#define PI 3.14159265

class sjf_monoPitchShift : public sjf_monoDelay
{
    sjf_phasor m_pitchPhasor;
    sjf_lpf lpf, hpf;
    float m_windowSize = 100, m_transpositionCalculationFactor;
    
public:
    sjf_monoPitchShift( )
    {
        lpf.setCutoff ( 0.9999f );
        hpf.setCutoff ( 0.0001f );
        setTransopositionFactor( m_windowSize );
        m_pitchPhasor.initialise( m_SR, 1.0f );
    }
    ~sjf_monoPitchShift( ) { }
    

    
    
    // transposition should be calculated as multiple (i.e. 2 would be an octave up, 0.5 an octave down)
    float pitchShiftOutput( int indexThroughCurrentBuffer, float transposition )
    {
        // f = (t-1)* R/s
        transposition -= 1.0f;
        transposition *= m_transpositionCalculationFactor;
//        DBG( "transposition " << transposition );
        m_pitchPhasor.setFrequency( transposition );
        // first voice
        auto phase = m_pitchPhasor.output();
        auto pitchShiftTimeDiff = phase * m_windowSize;
        setDelayTime( pitchShiftTimeDiff );
        auto amp = sin( PI * phase );
        auto val = amp * getSample( indexThroughCurrentBuffer );
        // second voice
        phase += 0.5f;
        if ( phase >= 1.0f ) { phase -= 1.0f; }
        pitchShiftTimeDiff = phase * m_windowSize;
        setDelayTime( pitchShiftTimeDiff );
        amp = sin( PI * phase );
        val += amp * getSample( indexThroughCurrentBuffer );
        return ( lpf.filterInput( val ) - hpf.filterInput( val ) );
    }
    
    float getWindowSize( )
    {
        return m_windowSize;
    }
    
    void setWindowSize( float windowSizeMilliseconds )
    {
        m_windowSize = windowSizeMilliseconds;
        setTransopositionFactor( m_windowSize );
    }
private:
    
    void setTransopositionFactor( float windowSize )
    {
        m_transpositionCalculationFactor = -1.0f  / ( windowSize * 0.001f ); // f = (t-1)* R/s
    }
    

    
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_monoPitchShift )
};

#endif /* sjf_monoPitchShift_h */

