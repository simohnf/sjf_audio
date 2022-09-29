//
//  sjf_drumMachine.h
//
//  Created by Simon Fay on 28/08/2022.
//

#ifndef sjf_drumMachine_h
#define sjf_drumMachine_h

#include <vector>
#include <JuceHeader.h>
#include "sjf_audioUtilities.h"
#include "sjf_oneshot.h"
#include "sjf_conductor.h"



class sjf_drumMachine
{
public:
    sjf_drumMachine( ) : m_stepVector( m_nSteps, std::vector<bool>(m_nVoices, false) )
    {
        for (int v = 0; v < m_nVoices; v++)
        {
            sjf_oneshot *sampleVoice = new sjf_oneshot;
            samples.add(sampleVoice);
            conductor.setNumSteps(m_nPatSteps);
        }
    }
    ~sjf_drumMachine(){}
    
    void initialise(int sampleRate, int totalNumOutputChannels, int samplesPerBlock)
    {
        for (int v = 0; v < m_nVoices; v++)
        {
            samples[v]->initialise(sampleRate);
        }
        tempBuffer.setSize(totalNumOutputChannels, samplesPerBlock);
    }
    
    int getCurrentStep() { return conductor.getCurrentStep(); }
    
    
    void loadSample (int voiceNumber ){ samples[voiceNumber]->loadSample(); }
    void setStepPattern(int step, const std::vector<bool> &stepPat)
    {
        m_stepVector[step] = stepPat;
    }
    
    void runMachine(juce::AudioBuffer< float > &buffer, int totalNumOutputChannels, float hostPosition)
    {
        auto buffSize = buffer.getNumSamples();
        auto step = conductor.getCurrentStep( hostPosition );
        for (int v = 0; v < m_nVoices; v++)
        {
            tempBuffer.clear();
            if (m_stepVector[step][v] && m_lastStep != step)
            {
                samples[v]->triggerNewOneshot();
            }
            samples[v]->playOneShot(tempBuffer);
            for (int c = 0; c < totalNumOutputChannels; c++)
            {
                buffer.addFrom(c, 0, tempBuffer, c, 0, buffSize);
            }
        }
        m_lastStep = step;
    }
    
    void turnOn( bool state) { conductor.turnOn(state); }
    int getNumVoices(){ return m_nVoices; }
    int getMaxNumSteps(){ return m_nSteps; }
    int getNumPatternSteps(){ return m_nPatSteps; }
    void setNumPatSteps(int nSteps ){
        m_nPatSteps = nSteps;
        conductor.setNumSteps(m_nPatSteps);
    }
    void setDivision( int newDivision ){ m_divisionType = newDivision; conductor.setDivision( m_divisionType ); }
    void setTuplet( int newTuplet ){ m_tupletType = newTuplet; conductor.setTuplet( m_tupletType ); }
    bool isOn(){ return conductor.isOn(); }
    bool getStepVoiceState( int step, int voice ){ return m_stepVector[step][voice]; }
    juce::String getSampleName( int voiceNumber ){ return samples[voiceNumber]->m_samplePath.getFileName(); }
    int getDivisionType(){ return m_divisionType; }
    int getTupletType(){ return m_tupletType; }
    float getGain( int voice ){ return samples[ voice ]->getGain(); }
    void setGain( int voice, float gain ){ samples[ voice ]->setGain( gain ); }
    float getPan( int voice ){ return samples[ voice ]->getPan(); }
    void setPan( int voice, float pan ){ samples[ voice ]->setPan( pan ); }
private:
    sjf_conductor conductor;
    int m_nVoices = 10, m_nSteps = 32, m_lastStep = -1, m_nPatSteps = 32, m_divisionType = 2, m_tupletType = 1;
    std::vector< std::vector< bool > >  m_stepVector;
    
    juce::OwnedArray< sjf_oneshot > samples;
    juce::AudioBuffer< float > tempBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_drumMachine )
};
#endif /* sjf_drumMachine_h */
