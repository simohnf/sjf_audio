//
//  sjf_audioUtilities.h
//
//  Created by Simon Fay on 25/08/2022.
//

#ifndef sjf_audioUtilities_h
#define sjf_audioUtilities_h

#include <JuceHeader.h>


#define PI 3.14159265
//==============================================================================
inline
float phaseEnv( float phase, float period, float envLen){
    auto nSegments = period / envLen;
    auto segmentPhase = phase * nSegments;
    auto rampUp = segmentPhase;
    if (rampUp > 1) {rampUp = 1;}
    else if (rampUp < 0) {rampUp = 0;}
    
    float rampDown = segmentPhase - (nSegments - 1);
    if (rampDown > 1) {rampDown = 1;}
    else if (rampDown < 0) {rampDown = 0;}
    rampDown *= -1;
    //    return rampUp+rampDown; // this would give linear fade
    return sin( PI* (rampUp+rampDown)/2 ); // this gives a smooth sinewave based fade
}

//==============================================================================

inline
float rand01()
{
    return float( rand() ) / float( RAND_MAX );
}


//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================

inline
void addBuffers(juce::AudioBuffer<float>& bufferToAddTo, juce::AudioBuffer<float>& bufferToAddFrom, float gainOfBuffer1, float gainOfBuffer2){
    float s1, s2;
    for (int channel = 0; channel < bufferToAddTo.getNumChannels() ; ++channel)
    {
        for (int index = 0; index < bufferToAddTo.getNumSamples(); index++){
            s1 = bufferToAddTo.getSample(channel, index) * gainOfBuffer1;
            s2 = bufferToAddFrom.getSample(channel, index) * gainOfBuffer2;
            
            bufferToAddTo.setSample(channel, index, (s1 + s2) );
        }
    }
}

#endif /* sjf_audioUtilities_h */
