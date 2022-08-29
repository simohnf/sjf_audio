//
//  sjf_granular.h
//
//  Created by Simon Fay on 28/08/2022.
//

#ifndef sjf_granular_h
#define sjf_granular_h

#include <vector>
#include <JuceHeader.h>
#include "sjf_audioUtilities.h"
#include "sjf_interpolationTypes.h"
#include "sjf_sampler.h"


class sjf_grainVoice
{
public:
    sjf_grainVoice(){};
    ~sjf_grainVoice(){};
    //==============================================================================
    void setPlaying(bool shouldBePlaying, float grainStartSamps, float grainLengthSamps, float playBackSpeed, float gain, float pan, int envType )
    {
        m_isPlayingFlag = shouldBePlaying;
        m_readPos = grainStartSamps;
        m_grainLength = grainLengthSamps;
        m_playBackSpeed = playBackSpeed;
        m_gain = gain;
        m_pan = pan;
        m_envType = envType;
        m_samplesPlayedCount = 0.0f;
        return;
    }
    //==============================================================================
    void playGrain(juce::AudioBuffer<float> &buffer, juce::AudioBuffer<float> &sourceBuffer )
    {
        if (!m_isPlayingFlag){ return; }
        auto outBufferSize = buffer.getNumSamples();
        auto sourceBufferSize = sourceBuffer.getNumSamples();
        auto numChannels = buffer.getNumChannels();
        
        for (int index = 0; index < outBufferSize; index ++)
        {
            auto pos = m_readPos;
            auto envVal = grainEnv( m_samplesPlayedCount/m_grainLength, m_envType );
            for (int channel = 0; channel < numChannels; channel ++)
            {
                auto val = linearInterpolate(sourceBuffer, channel % sourceBuffer.getNumChannels(), pos);
                val *= m_gain;
                val *= envVal;
                val *= pan2( m_pan, channel);
                buffer.addSample( channel, index, val );
            }
            m_readPos += m_playBackSpeed;
            while (m_readPos >= sourceBufferSize) { m_readPos -= sourceBufferSize; }
            
            m_samplesPlayedCount++;
            if (m_samplesPlayedCount >= m_grainLength)
            {
                m_isPlayingFlag = false;
                return;
            }
        }
    }
    //==============================================================================
    void playGrain(juce::AudioBuffer<float> &buffer, juce::AudioBuffer<float> &sourceBuffer, int index, int g )
    {
        if (!m_isPlayingFlag){ return; }
//        auto outBufferSize = buffer.getNumSamples();
        auto sourceBufferSize = sourceBuffer.getNumSamples();
        auto numChannels = buffer.getNumChannels();
        auto envVal = grainEnv( m_samplesPlayedCount/m_grainLength, m_envType );
        for (int channel = 0; channel < numChannels; channel ++)
        {
            auto val = linearInterpolate(sourceBuffer, channel % sourceBuffer.getNumChannels(), m_readPos);
            val *= m_gain;
            val *= envVal;
            val *= pan2( m_pan, channel);
            buffer.addSample( channel, index, val );
        }
        m_readPos += m_playBackSpeed;
        while (m_readPos >= sourceBufferSize) { m_readPos -= sourceBufferSize; }
        
        m_samplesPlayedCount++;
        if (m_samplesPlayedCount >= m_grainLength)
        {
            m_isPlayingFlag = false;
            return;
        }
    }
    //==============================================================================
    bool getPlayingState()
    {
        return m_isPlayingFlag;
    }
private:
    bool m_isPlayingFlag = false;
    float m_readPos = 0.0f, m_grainLength = 4410.0f, m_playBackSpeed = 1.0f, m_gain = 1.0f, m_pan = 0.5f, m_samplesPlayedCount = 0.0f;
    int m_envType;
    
};


class sjf_grainEngine : public sjf_sampler
    {
        
    public:
        sjf_grainEngine() : m_grains(64){ srand((unsigned)time(NULL)); };
        ~sjf_grainEngine(){};
        //==============================================================================
        void newGrain(float grainStartFractional, float grainLengthMS, float transpositionInSemitones, float gain, float pan, int envType)
        {
            if (!m_sampleLoadedFlag) { return; }
            auto grainStartSamps = m_AudioSample.getNumSamples() * grainStartFractional;
            auto playBackSpeed = pow(2.0f, transpositionInSemitones/12.0f);
            m_grains[m_voiceNumber].setPlaying( true, grainStartSamps, grainLengthMS * m_SR/1000.0f, playBackSpeed, gain, pan, envType );
            m_voiceNumber++;
            m_voiceNumber %= m_grains.size();
            return;
        }
        //==============================================================================
        void playGrains( juce::AudioBuffer<float> &buffer )
        {
            if (!m_sampleLoadedFlag) { return; }
            for (int index = 0; index < buffer.getNumSamples(); index++)
            {
                for ( int g = 0; g < m_grains.size(); g++ )
                {
                    m_grains[g].playGrain(buffer, m_AudioSample, index, g);
                }
            }
        }
        //==============================================================================
        void playCloud( juce::AudioBuffer<float> &buffer )
        {
            if (!m_sampleLoadedFlag || !m_canPlayFlag) { return; }
            auto cloudLengthSamps = m_cloudLengthMS * m_SR * 0.001f;
            auto deltaTimeSamps = m_deltaTimeMS * m_SR * 0.001f;
//            auto sourceBufferSize = m_AudioSample.getNumSamples();
            
            for ( int index = 0; index < buffer.getNumSamples(); index ++ )
            {
                if (m_cloudPos >= m_nextTrigger && m_cloudPos <= (cloudLengthSamps - deltaTimeSamps))
                {// only trigger a new grain if we're still within the cloud length
                    auto startWithJitter = m_grainStartFractional + (rand01()*2.0f - 1.0f)*0.1;
                    auto sizeWithJitter = m_grainSizeMS + (rand01()*2.0f - 1.0f)*0.1*m_grainSizeMS;
                    auto transposeWithJitter = m_transposeSemiTones + (rand01()*2.0f - 1.0f);
                    auto gainWithJitter = m_grainGain + (rand01()*2.0f - 1.0f)*0.1;
                    auto panWithJitter = m_pan + (rand01()*2.0f - 1.0f)*0.1;
                    newGrain( startWithJitter,
                             sizeWithJitter,
                             transposeWithJitter,
                             gainWithJitter,
                             panWithJitter,
                             m_envType);
                    m_deltaTimeMS = rand01()*90 + 10; // random deltaTime
                    m_nextTrigger = m_cloudPos + (m_deltaTimeMS * m_SR * 0.001f);
                    //                    newGrain(float grainStartFractional, float grainLengthMS, float transpositionInSemitones, float gain, float pan, int envType)
                }

                    
                for ( int g = 0; g < m_grains.size(); g++ )
                {
                    m_grains[g].playGrain(buffer, m_AudioSample, index, g);
                }
                

                m_cloudPos ++;
                if ( m_cloudPos >= (cloudLengthSamps - deltaTimeSamps) )
                {
                    int playingCount = 0;
                    for ( int g = 0; g < m_grains.size(); g++ )
                    {
                        if ( m_grains[g].getPlayingState() ) { playingCount++; } // if any of the grains are still playing let them finish
                    }
                    if (playingCount == 0)
                    {
                        m_canPlayFlag = false;
                    }
                }
            }
        }
        //==============================================================================
        void triggerNewCloud(bool shouldPlayCloud)
        {
            m_canPlayFlag = shouldPlayCloud;
            m_cloudPos = 0;
            m_nextTrigger = 0;
        }
        //==============================================================================
        void setEnvType( int envType )
        {
            if (envType < 1) { envType = 1; }
            if (envType > 5) { envType = 5; }
            envType -= 1;
            m_envType = envType;
        }
        //==============================================================================
        void setGrainLength( float grainLengthInMS )
        {
            if( grainLengthInMS <= 0 )
            {
                grainLengthInMS = 1;
            }
            m_grainSizeMS = grainLengthInMS;
        }
        //==============================================================================
        void setGrainStart(float grainStart)
        {
            if (grainStart < 0){ grainStart = 0; }
            if (grainStart >= 1){ grainStart = 1; }
            
            m_grainStartFractional = grainStart;
        }
        //==============================================================================
        void setTransposition( float transpositionInSemitones )
        {
            m_transposeSemiTones = transpositionInSemitones;
        }
        //==============================================================================
        void setPan( float pan )
        {
            if (pan < 0) { pan = 0; }
            if (pan > 1) { pan = 1; }
            m_pan = pan;
        }
    private:
        std::vector<sjf_grainVoice> m_grains;
        
        int m_voiceNumber = 0;
        float m_cloudLengthMS = 2000.0f, m_desnity = 1.0f, m_deltaTimeMS = 50.0f, m_cloudPos = 0.0f, m_nextTrigger = 0.0f;
        bool m_canPlayFlag = false;
        
        int m_envType = 0;
        float m_grainStartFractional = 0, m_grainSizeMS = 100, m_transposeSemiTones = 0, m_pan = 0.5, m_grainGain = 0.8f;
    };
#endif /* sjf_granular_h */
