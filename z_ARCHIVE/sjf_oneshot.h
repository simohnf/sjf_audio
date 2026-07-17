//
//  sjf_oneshot.h
//
//  Created by Simon Fay on 13/09/2022.
//

#ifndef sjf_oneshot_h
#define sjf_oneshot_h

#include <JuceHeader.h>
#include "sjf_sampler.h"

class sjf_oneshot : public sjf_sampler
{
    class sjf_oneshotVoice
    {
    public:
        sjf_oneshotVoice(){};
        ~sjf_oneshotVoice(){};
        
        void initialise(int sampleRate)
        {
            m_SR  = sampleRate;
            m_fadeLenSamps = round(m_SR * 0.001); // fade length is set to 1ms for simplicity... rounded for simplicity
        }
        //==============================================================================
        void play( juce::AudioBuffer<float> &destinationBuffer, juce::AudioBuffer<float> &sourceBuffer )
        {
            auto bufferSize = destinationBuffer.getNumSamples();
            auto numChannels = destinationBuffer.getNumChannels();
            
            for (int index = 0; index < bufferSize; index++)
            {
                if(!m_isPlayingFlag){ return; }
                for (int channel = 0; channel < numChannels; channel++)
                {
                    if ( m_read_pos >= sourceBuffer.getNumSamples()){ m_read_pos = 0; return; }
                    auto val = sourceBuffer.getSample(channel % sourceBuffer.getNumChannels(), m_read_pos);
                    val *= m_gain;
                    if (m_isTurningOffFlag)
                    {
                        auto fadeAmp = 1.0f - ( m_fadePos / m_fadeLenSamps );
                        m_fadePos ++;
                        if (m_fadePos >= m_fadeLenSamps)
                        {
                            m_isPlayingFlag = false;
                        }
                        val *= fadeAmp;
                    }
                    destinationBuffer.addSample( channel, index, val );
                }
                m_read_pos ++;
                if ( m_read_pos >= sourceBuffer.getNumSamples() )
                {
                    m_isPlayingFlag = false;
                }
            }
        }
        //==============================================================================
        void turnVoiceOff()
        {
            if(m_isPlayingFlag){ m_isTurningOffFlag = true;}
        }
        //==============================================================================
        void turnVoiceOn()
        {
            m_isTurningOffFlag = false;
            m_isPlayingFlag = true;
            m_read_pos = 0;
            m_fadePos = 0;
        }
        //==============================================================================
        void setGain( float gain )
        {
            m_gain = gain;
        }
        //==============================================================================
        float getGain()
        {
            return m_gain;
        }
        //==============================================================================
        void setPan( float pan )
        {
            m_pan = pan;
        }
        //==============================================================================
        float getPan()
        {
            return m_pan;
        }
        //==============================================================================
    private:
        float m_read_pos = 0, m_SR = 44100, m_fadePos = 0, m_fadeLenSamps = 44, m_gain = 1, m_pan = 0.5;
        bool m_isPlayingFlag = false, m_isTurningOffFlag = false;
    };
    
public:
    sjf_oneshot()
    {
        for(int i = 0; i < m_nVoices; i++)
        {
            sjf_oneshotVoice *voice = new sjf_oneshotVoice;
            oneshotVoices.add(voice);
        }
    };
    ~sjf_oneshot(){};
    //==============================================================================
    void initialise(int sampleRate)
    {
        m_SR  = sampleRate;
        srand((unsigned)time(NULL));
        for(int i = 0; i < m_nVoices; i++)
        {
            oneshotVoices[i]->initialise(m_SR);
        }
    }
    //==============================================================================
    void triggerNewOneshot()
    {
        oneshotVoices[m_currentVoice]->turnVoiceOff();
        m_currentVoice += 1;
        m_currentVoice %= m_nVoices;
        oneshotVoices[m_currentVoice]->turnVoiceOn();
    }
    //==============================================================================
    void playOneShot( juce::AudioBuffer<float> &destinationBuffer )
    {
        if ( !m_sampleLoadedFlag ){ return; }
        for(int i = 0; i < m_nVoices; i++)
        {
            oneshotVoices[i]->play( destinationBuffer, m_AudioSample );
        }
    }
    //==============================================================================
    void setGain( float gain )
    {
        m_gain = gain;
        for(int i = 0; i < m_nVoices; i++)
        {
            oneshotVoices[i]->setGain( m_gain );
        }
    }
    //==============================================================================
    float getGain()
    {
        return m_gain;
    }
    //==============================================================================
    void setPan( float pan )
    {
        m_pan = pan;
        for(int i = 0; i < m_nVoices; i++)
        {
            oneshotVoices[i]->setPan( m_pan );
        }
    }
    //==============================================================================
    float getPan()
    {
        return m_pan;
    }
    //==============================================================================
private:
    const static int m_nVoices = 2;
    juce::OwnedArray < sjf_oneshotVoice > oneshotVoices;
    int m_currentVoice = 0;
    float m_gain = 1, m_pan = 0.5;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_oneshot )
};



#endif /* sjf_oneshot_h */




