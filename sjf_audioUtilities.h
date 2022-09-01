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
float pan2( float pan, int channel){
    if (channel < 0) { channel = 0 ; }
    if (channel > 1) { channel = 1 ; }
    if (pan < 0) { pan = 0; }
    if (pan >= 1) { pan = 1; }
    pan *= 0.5;
    if (channel == 0)
    {
        pan += -1.0f;
        pan += 0.5;
    }
    
    return sin( PI* pan ); // this gives a smooth sinewave based fade
}

//==============================================================================
inline
float grainEnv( float phase, int type)
{
    if (type < 0) { type = 0 ; }
    if (type > 4) { type = 4 ; }
    if (phase < 0 || phase >= 1) {phase = 0; }
    switch(type)
    {
        case 0: // hann window
        {
            phase *= 2.0f; // convert to full cycle of sine wave
            phase -= 0.5f; // offset to lowest point in wave
            auto output = sin( PI * phase );
            output += 1; // add constatnt to bring minimum to 0
            output *= 0.5; // halve values to normalise between 0 --> 1
            return output; 
        }
        case 1: // triangle window
            if (phase <= 0.5) { return phase*2.0f; }
            else { return 1 - ( (phase - 0.5)*2.0f) ; }
        case 2: // sinc function
            phase -= 0.5f;
            phase *= 9.0f;
            if ( phase == 0 ) { return 1; }
            return sin( PI * phase ) / phase;
        case 3: // exponential decay
            phase = 1 - phase;
            return pow (phase, 2);
        case 4: // reversed exponential decay
            return pow (phase, 2);
    }
    if (phase <= 0.5) { return phase*2.0f; }
    else { return 1 - ( (phase - 0.5)*2.0f) ; }
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

//inline
//void addBuffers(juce::AudioBuffer<float>& bufferToAddTo, juce::AudioBuffer<float>& bufferToAddFrom, float gainOfBuffer1, float gainOfBuffer2){
//    float s1, s2;
//    for (int channel = 0; channel < bufferToAddTo.getNumChannels() ; ++channel)
//    {
//        for (int index = 0; index < bufferToAddTo.getNumSamples(); index++){
//            s1 = bufferToAddTo.getSample(channel, index) * gainOfBuffer1;
//            s2 = bufferToAddFrom.getSample(channel, index) * gainOfBuffer2;
//            
//            bufferToAddTo.setSample(channel, index, (s1 + s2) );
//        }
//    }
//}

#endif /* sjf_audioUtilities_h */
