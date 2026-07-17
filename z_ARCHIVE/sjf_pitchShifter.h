//
//  sjf_pitchShifter.h
//
//  Created by Simon Fay on 24/08/2022.
//

#ifndef sjf_pitchShifter_h
#define sjf_pitchShifter_h

#include <JuceHeader.h>
#include "sjf_delayLine.h"
#include "sjf_phasor.h"


#define PI 3.14159265

class sjf_pitchShifter : public sjf_delayLine
{
public:
    sjf_pitchShifter(){};
    ~sjf_pitchShifter(){};
    
    void intialise( int sampleRate , int totalNumInputChannels, int totalNumOutputChannels, int samplesPerBlock) override
    {
        if (sampleRate > 0 ) { m_SR = sampleRate; }
        m_delayBuffer.setSize( totalNumOutputChannels, m_SR * m_delBufferLength );
        m_delayBuffer.clear();
        m_delL.reset( m_SR, 0.1f ) ;
        m_delR.reset( m_SR, 0.1f ) ;
        
        m_pitchPhasorL.initialise(m_SR, 0);
        m_pitchPhasorR.initialise(m_SR, 0);
        
        m_phaseResetL.reset(m_SR, 0.02f);
        m_phaseResetR.reset(m_SR, 0.02f);
    }
    
    void copyFromDelayBufferWithPitchShift(juce::AudioBuffer<float>& destinationBuffer, float gain, float transpositionSemiTonesL, float transpositionSemiTonesR)
    {
        auto bufferSize = destinationBuffer.getNumSamples();
        auto delayBufferSize = m_delayBuffer.getNumSamples();
        auto numChannels = destinationBuffer.getNumChannels();
        auto delTimeL = m_delL.getCurrentValue() * m_SR / 1000.0f;
        auto delTimeR = m_delR.getCurrentValue() * m_SR / 1000.0f;
        
        auto pitchShiftWindowSize = 20.0f; // ms
        auto pitchShiftWindowSamples = m_SR* pitchShiftWindowSize/1000.0f;
        auto transpositionRatioL = pow(2.0f, transpositionSemiTonesL/12.0f);
        auto transpositionRatioR = pow(2.0f, transpositionSemiTonesR/12.0f);
        auto fL = -1*(transpositionRatioL - 1) / (pitchShiftWindowSize*0.001);
        auto fR = -1*(transpositionRatioR - 1) / (pitchShiftWindowSize*0.001);
        m_pitchPhasorL.setFrequency( fL );
        if (fL == 0.0f && m_lastTranspositionL != transpositionSemiTonesL){
            m_phaseResetL.setCurrentAndTargetValue(m_pitchPhasorL.getPhase());
            m_phaseResetL.setTargetValue(0.0f);
            m_pitchPhasorL.setPhase(m_phaseResetL.getCurrentValue());
        }
        
        m_pitchPhasorR.setFrequency( fR );
        if (fR == 0.0f && m_lastTranspositionR != transpositionSemiTonesR){
            m_phaseResetR.setCurrentAndTargetValue(m_pitchPhasorR.getPhase());
            m_phaseResetR.setTargetValue(0.0f);
            m_pitchPhasorR.setPhase(m_phaseResetR.getCurrentValue());
        }
        for (int index = 0; index < bufferSize; index++)
        {
            if(fL == 0.0f){ m_pitchPhasorL.setPhase( m_phaseResetL.getNextValue() ); }
            if(fR == 0.0f){ m_pitchPhasorR.setPhase( m_phaseResetR.getNextValue() ); }
            for (int channel = 0; channel < numChannels; channel++)
            {
                if(channel < 0 || channel > 1){ return; }
                float delTime, phase;
                if (channel == 0){
                    delTime = delTimeL;
                    phase = m_pitchPhasorL.output();
                }
                else{     delTime = delTimeR;
                    phase = m_pitchPhasorR.output();
                }
                auto val = pitchShiftOutputCalculations(channel, index, delTime, phase, pitchShiftWindowSamples, delayBufferSize, gain);
                destinationBuffer.setSample(channel, index, val );
                
                phase += 0.5;
                while ( phase >= 1.0f ){ phase -= 1.0f; }
                while ( phase < 0.0f ){ phase += 1.0f; }
                val = pitchShiftOutputCalculations(channel, index, delTime, phase, pitchShiftWindowSamples, delayBufferSize, gain);
                destinationBuffer.addSample(channel, index, val );
            }
            delTimeL = m_delL.getNextValue() * m_SR / 1000.0f;
            delTimeR = m_delR.getNextValue() * m_SR / 1000.0f;
        }
        m_lastTranspositionL = transpositionSemiTonesL;
        m_lastTranspositionR = transpositionSemiTonesR;
    };
    
    
private:
    float pitchShiftOutputCalculations(int channel, int index, float delTime, float phase, float pitchShiftWindowSamples, float delayBufferSize, float gain ){
        auto channelReadPos = m_writePos - (delTime + ( pitchShiftWindowSamples * phase )) + index;
        auto amp = sin( PI * phase );
        while ( channelReadPos < 0 ) { channelReadPos += delayBufferSize; }
        while (channelReadPos >= delayBufferSize) { channelReadPos -= delayBufferSize; }
        auto val = cubicInterpolate(m_delayBuffer, channel, channelReadPos) * gain;
        val *= amp;
        return val;
    }
private:
    sjf_phasor m_pitchPhasorL, m_pitchPhasorR;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> m_phaseResetL, m_phaseResetR;
    float m_lastTranspositionL, m_lastTranspositionR;
    

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (sjf_pitchShifter)
};

#endif /* sjf_pitchShifter_h */
