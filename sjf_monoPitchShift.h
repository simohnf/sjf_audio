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


#define PI 3.14159265

class sjf_monoPitchShift : public sjf_monoDelay
{
public:
    sjf_monoPitchShift( ) { }
    ~sjf_monoPitchShift( ) { }
    
    float pitchShiftOutput( int indexThroughCurrentBuffer )
    {
        
    }
    
    
    float pitchShiftOutputCalculations( int index, float delTime, float phase, float pitchShiftWindowSamples, float delayBufferSize, float gain )
    {

    }
private:
    sjf_phasor m_pitchPhasor;
    
    float m_lastTransposition;
    
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_monoPitchShift_h )
};

#endif /* sjf_monoPitchShift_h */

