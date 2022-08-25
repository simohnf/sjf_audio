//
//  sjf_pitchShifter.h
//  sjf_granSynth
//
//  Created by Simon Fay on 24/08/2022.
//

#ifndef sjf_pitchShifter_h
#define sjf_pitchShifter_h

#include "sjf_delayLine.h"

class sjf_pitchShifter : public sjf_delayLine
{
public:
    sjf_pitchShifter(){};
    ~sjf_pitchShifter(){};
    
    void intialise( int sampleRate , int totalNumInputChannels, int totalNumOutputChannels, int samplesPerBlock) override
    {
        if (sampleRate > 0 ) { SR = sampleRate; }
        delayBuffer.setSize( totalNumOutputChannels, SR * del_buff_len );
        delayBuffer.clear();
        delL.reset( SR, 0.1f ) ;
        delR.reset( SR, 0.1f ) ;
        
        pitchPhasorL.initialise(SR, 0);
        pitchPhasorR.initialise(SR, 0);
        
        phaseResetL.reset(SR, 0.02f);
        phaseResetR.reset(SR, 0.02f);
    }
    
    void copyFromDelayBufferWithPitchShift(juce::AudioBuffer<float>& destinationBuffer, float gain, float transpositionSemiTonesL, float transpositionSemiTonesR)
    {
        auto bufferSize = destinationBuffer.getNumSamples();
        auto delayBufferSize = delayBuffer.getNumSamples();
        auto numChannels = destinationBuffer.getNumChannels();
        auto delTimeL = delL.getCurrentValue() * SR / 1000.0f;
        auto delTimeR = delR.getCurrentValue() * SR / 1000.0f;
        
        auto pitchShiftWindowSize = 20.0f; // ms
        auto pitchShiftWindowSamples = SR* pitchShiftWindowSize/1000.0f;
        auto transpositionRatioL = pow(2.0f, transpositionSemiTonesL/12.0f);
        auto transpositionRatioR = pow(2.0f, transpositionSemiTonesR/12.0f);
        auto fL = -1*(transpositionRatioL - 1) / (pitchShiftWindowSize*0.001);
        auto fR = -1*(transpositionRatioR - 1) / (pitchShiftWindowSize*0.001);
        pitchPhasorL.setFrequency( fL );
        if (fL == 0.0f && lastTranspositionL != transpositionSemiTonesL){
            phaseResetL.setCurrentAndTargetValue(pitchPhasorL.getPhase());
            phaseResetL.setTargetValue(0.0f);
            pitchPhasorL.setPhase(phaseResetL.getCurrentValue());
        }
        
        pitchPhasorR.setFrequency( fR );
        if (fR == 0.0f && lastTranspositionR != transpositionSemiTonesR){
            phaseResetR.setCurrentAndTargetValue(pitchPhasorR.getPhase());
            phaseResetR.setTargetValue(0.0f);
            pitchPhasorR.setPhase(phaseResetR.getCurrentValue());
        }
        for (int index = 0; index < bufferSize; index++)
        {
            if(fL == 0.0f){ pitchPhasorL.setPhase( phaseResetL.getNextValue() ); }
            if(fR == 0.0f){ pitchPhasorR.setPhase( phaseResetR.getNextValue() ); }
            for (int channel = 0; channel < numChannels; channel++)
            {
                if(channel < 0 || channel > 1){ return; }
                float delTime, phase;
                if (channel == 0){
                    delTime = delTimeL;
                    phase = pitchPhasorL.output();
                }
                else{     delTime = delTimeR;
                    phase = pitchPhasorR.output();
                }
                auto val = pitchShiftOutputCalculations(channel, index, delTime, phase, pitchShiftWindowSamples, delayBufferSize, gain);
                destinationBuffer.setSample(channel, index, val );
                
                phase += 0.5;
                while ( phase >= 1.0f ){ phase -= 1.0f; }
                while ( phase < 0.0f ){ phase += 1.0f; }
                val = pitchShiftOutputCalculations(channel, index, delTime, phase, pitchShiftWindowSamples, delayBufferSize, gain);
                destinationBuffer.addSample(channel, index, val );
            }
            delTimeL = delL.getNextValue() * SR / 1000.0f;
            delTimeR = delR.getNextValue() * SR / 1000.0f;
        }
        lastTranspositionL = transpositionSemiTonesL;
        lastTranspositionR = transpositionSemiTonesR;
    };
    
    
private:
    sjf_phasor pitchPhasorL, pitchPhasorR;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> phaseResetL, phaseResetR;
    float lastTranspositionL, lastTranspositionR;
    
    float pitchShiftOutputCalculations(int channel, int index, float delTime, float phase, float pitchShiftWindowSamples, float delayBufferSize, float gain ){
        auto channelReadPos = write_pos - (delTime + ( pitchShiftWindowSamples * phase )) + index;
        auto amp = sin( PI * phase );
        while ( channelReadPos < 0 ) { channelReadPos += delayBufferSize; }
        while (channelReadPos >= delayBufferSize) { channelReadPos -= delayBufferSize; }
        auto val = cubicInterpolate(delayBuffer, channel, channelReadPos) * gain;
        val *= amp;
        return val;
    }
};

#endif /* sjf_pitchShifter_h */
