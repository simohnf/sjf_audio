//
//  sjf_audio.hpp
//  
//
//  Created by Simon Fay on 22/07/2022.
//

#ifndef sjf_audio_hpp
#define sjf_audio_hpp

#include <stdio.h>
#include <JuceHeader.h>
#include <vector>
#include "sjf_interpolationTypes.h"
#include "sjf_delayLine.h"
#include "sjf_overdrive.h"
#include "sjf_oscillator.h"
#include "sjf_lookAndFeel.h"
#include "sjf_phasor.h"
#include "sjf_smoothValue.h"

#define PI 3.14159265
//#define outlineColour juce::LookAndFeel_V4::ColourScheme::UIColour::outline
//==============================================================================
/**
 */





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




#endif /* sjf_audio_hpp */
