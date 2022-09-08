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
    sjf_grainVoice()
    {};
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
    void setPlaying(bool shouldBePlaying, float grainStartSamps, float grainLengthSamps, float playBackSpeed, float gain, float pan, int envType, float reverbAmount )
    {
        m_isPlayingFlag = shouldBePlaying;
        m_readPos = grainStartSamps;
        m_grainLength = grainLengthSamps;
        m_playBackSpeed = playBackSpeed;
        m_gain = gain;
        m_pan = pan;
        m_envType = envType;
        m_reverbAmount = reverbAmount;
        m_samplesPlayedCount = 0.0f;
        return;
    }
    //==============================================================================
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
                if (phase < 0.9f) { return pow (phase / 0.9f, 2); }
                else { phase -= 0.9f; phase *= 0.1f; return 1.0f - phase; }
                
        }
        if (phase <= 0.5) { return phase*2.0f; }
        else { return 1 - ( (phase - 0.5)*2.0f) ; }
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
    void playGrain(juce::AudioBuffer<float> &buffer, juce::AudioBuffer<float> &sourceBuffer, juce::AudioBuffer<float> &reverbBuffer, int index )
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
            buffer.addSample( channel, index, (1.0f-m_reverbAmount) * val );
            reverbBuffer.addSample( channel, index, (m_reverbAmount) * val );
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
    float m_readPos = 0.0f, m_grainLength = 4410.0f, m_playBackSpeed = 1.0f, m_gain = 1.0f, m_pan = 0.5f, m_samplesPlayedCount = 0.0f, m_reverbAmount = 0.0f;
    int m_envType;
    
};
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================

class sjf_grainEngine : public sjf_sampler
    {
        
    public:
        sjf_grainEngine() : m_grains(128)
        {
            srand((unsigned)time(NULL));
            m_revParams.roomSize = m_reverbRoomSize;
            m_revParams.damping = m_reverbDamping;
            m_revParams.wetLevel = 1.0f;
            m_revParams.dryLevel = 0.0f;
            m_reverb.setParameters( m_revParams );
        };
        ~sjf_grainEngine(){};
        //==============================================================================
        void initialiseGranSynth(int sampleRate, int samplesPerBlock)
        {
            m_SR  = sampleRate;
            srand((unsigned)time(NULL));
            m_samplesPerBlock = samplesPerBlock;
            prepareReverb( m_SR, m_samplesPerBlock );
        };
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
        void newGrain(float grainStartFractional, float grainLengthMS, float transpositionInSemitones, float gain, float pan, int envType, float reverbAmount)
        {
            if (!m_sampleLoadedFlag) { return; }
            auto grainStartSamps = m_AudioSample.getNumSamples() * grainStartFractional;
            auto playBackSpeed = pow(2.0f, transpositionInSemitones/12.0f);
            m_grains[m_voiceNumber].setPlaying( true, grainStartSamps, grainLengthMS * m_SR/1000.0f, playBackSpeed, gain, pan, envType, reverbAmount );
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
            if (!m_sampleLoadedFlag || !m_canPlayFlag)
            { m_canPlayFlag = false;  return; }
            auto cloudLengthSamps = m_cloudLengthMS * m_SR * 0.001f;
            auto deltaTimeSamps = m_deltaTimeMS * m_SR * 0.001f;
            
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
        void playCloudFromVectors( juce::AudioBuffer<float> &buffer )
        {
            buffer.clear();
            if (!m_sampleLoadedFlag || !m_canPlayFlag)
            {
                m_canPlayFlag = false;
                auto block = juce::dsp::AudioBlock<float> ( buffer );
                auto context = juce::dsp::ProcessContextReplacing<float> (block);
                m_reverb.process( context );
                return;
            }
            m_reverbBuffer.makeCopyOf( buffer );
            auto cloudLengthSamps = m_cloudLengthMS * m_SR * 0.001f;
            auto deltaTimeSamps = m_deltaTimeMS * m_SR * 0.001f;
            
            for ( int index = 0; index < buffer.getNumSamples(); index ++ )
            {
                if (m_cloudPos >= m_nextTrigger && m_cloudPos <= (cloudLengthSamps - deltaTimeSamps))
                {// only trigger a new grain if we're still within the cloud length
                    auto phaseThroughCloud = (float)m_cloudPos / (float)cloudLengthSamps;
                    m_deltaTimeMS = 1.0f + 99.0f * linearInterpolate( m_grainDeltaVector, m_grainDeltaVector.size() * phaseThroughCloud );
                    m_nextTrigger = m_cloudPos + (m_deltaTimeMS * m_SR * 0.001f);
                    auto grainStart = linearInterpolate( m_grainPositionVector, m_grainPositionVector.size() * phaseThroughCloud );
                    auto grainPan = linearInterpolate( m_grainPanVector, m_grainPanVector.size() * phaseThroughCloud );
                    auto grainTransposition = linearInterpolate( m_grainTranspositionVector, m_grainTranspositionVector.size() * phaseThroughCloud );
                    grainTransposition *= 24.0f;
                    grainTransposition -= 12.0f;
                    auto grainSize = linearInterpolate( m_grainSizeVector, m_grainSizeVector.size() * phaseThroughCloud );
                    // if linked grain size is a maximum of 10 times delta time
                    if (m_linkSizeAndDeltaFlag) { grainSize = 1.0f + fmin( grainSize * m_deltaTimeMS * 10.0f , grainSize * 99.0f ); }
                    else { grainSize = 1.0f + (grainSize * 99.0f); }
                    auto grainGain = linearInterpolate( m_grainGainVector, m_grainGainVector.size() * phaseThroughCloud );
                    auto grainReverb = linearInterpolate( m_grainReverbVector, m_grainReverbVector.size() * phaseThroughCloud );
                    newGrain( grainStart,
                             grainSize,
                             grainTransposition,
                             grainGain,
                             grainPan,
                             m_envType,
                             grainReverb);
                    // newGrain(float grainStartFractional, float grainLengthMS, float transpositionInSemitones, float gain, float pan, int envType, float reverbAmount)
                }
                
                
                for ( int g = 0; g < m_grains.size(); g++ )
                {
                    m_grains[g].playGrain(buffer, m_AudioSample, m_reverbBuffer, index);
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
                        m_cloudPos = cloudLengthSamps;
                    }
                }
            }
            auto block = juce::dsp::AudioBlock<float> (m_reverbBuffer);
            auto context = juce::dsp::ProcessContextReplacing<float> (block);
            m_reverb.process( context );
            
            for (int channel = 0; channel < buffer.getNumChannels(); channel ++)
            {
                buffer.addFrom( channel, 0, m_reverbBuffer, channel, 0, buffer.getNumSamples() );
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
        int getEnvType( )
        {
            return 1 + m_envType;
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
        //==============================================================================
        void setGrainPositionVector(const std::vector<float> &grainPositionVector )
        {
            m_grainPositionVector = grainPositionVector;
        }
        //==============================================================================
        std::vector<float> getGrainPositionVector( )
        {
            return m_grainPositionVector;
        }
        //==============================================================================
        void setGrainPanVector(const std::vector<float> &grainPanVector )
        {
            m_grainPanVector = grainPanVector;
        }
        //==============================================================================
        std::vector<float> getGrainPanVector( )
        {
            return m_grainPanVector;
        }
        //==============================================================================
        void setGrainTranspositionVector(const std::vector<float> &grainTranspositionVector )
        {
            m_grainTranspositionVector = grainTranspositionVector;
        }
        //==============================================================================
        std::vector<float> getGrainTranspositionVector( )
        {
            return m_grainTranspositionVector;
        }
        //==============================================================================
        void setGrainSizeVector(const std::vector<float> &grainSizeVector )
        {
            m_grainSizeVector = grainSizeVector;
        }
        //==============================================================================
        std::vector<float> getGrainSizeVector( )
        {
            return m_grainSizeVector;
        }
        //==============================================================================
        void setGrainGainVector(const std::vector<float> &grainGainVector )
        {
            m_grainGainVector = grainGainVector;
        }
        //==============================================================================
        std::vector<float> getGrainGainVector( )
        {
            return m_grainGainVector;
        }
        //==============================================================================
        void setGrainDeltaVector(const std::vector<float> &grainDeltaVector )
        {
            m_grainDeltaVector = grainDeltaVector;
        }
        //==============================================================================
        std::vector<float> getGrainDeltaVector( )
        {
            return m_grainDeltaVector;
        }
        //==============================================================================
        void setGrainReverbVector(const std::vector<float> &grainReverbVector )
        {
            m_grainReverbVector = grainReverbVector;
        }
        //==============================================================================
        std::vector<float> getGrainReverbVector( )
        {
            return m_grainReverbVector;
        }
        //==============================================================================
        void setCloudLength(float cloudLengthMS )
        {
            m_cloudLengthMS = cloudLengthMS;
        }
        //==============================================================================
        float getCloudLengthMS( )
        {
            return m_cloudLengthMS;
        }
        //==============================================================================
        void prepareReverb( double sampleRate, int samplesPerBlock )
        {
            juce::dsp::ProcessSpec spec;
            spec.maximumBlockSize = samplesPerBlock;
            spec.numChannels = 2; // only stereo
            spec.sampleRate = sampleRate;
            
            m_reverb.prepare( spec );
        }
        //==============================================================================
        float getCurrentCloudPhase()
        {
            return m_cloudPos / ( m_cloudLengthMS  * 0.001f * m_SR );
        }
        //==============================================================================
        float getCloudLength()
        {
            return m_cloudLengthMS;
        }
        //==============================================================================
        void setReverbSize( float roomSize0to1 )
        {
            m_reverbRoomSize = roomSize0to1;
            m_revParams.roomSize = m_reverbRoomSize;
            m_reverb.setParameters( m_revParams );
        }
        //==============================================================================
        float getReverbSize( )
        {
            return m_reverbRoomSize;
        }
        //==============================================================================
        void setReverbDamping( float damping0to1 )
        {
            m_reverbDamping = damping0to1;
            m_revParams.damping = m_reverbDamping;
            m_reverb.setParameters( m_revParams );
        }
        //==============================================================================
        float getReverbDamping(  )
        {
            return m_reverbDamping; 
        }
        //==============================================================================
        void linkSizeAndDeltaTime( bool deltaTimeIsLinkedToGrainSize )
        {
            m_linkSizeAndDeltaFlag = deltaTimeIsLinkedToGrainSize;
        }
    private:
        std::vector<sjf_grainVoice> m_grains;
        std::vector<float> m_grainPositionVector, m_grainPanVector, m_grainTranspositionVector, m_grainSizeVector, m_grainGainVector, m_grainDeltaVector, m_grainReverbVector;
        int m_voiceNumber = 0, m_envType = 0, m_samplesPerBlock = 128;
        float m_cloudLengthMS = 2000.0f, m_desnity = 1.0f, m_deltaTimeMS = 50.0f, m_cloudPos = 0.0f, m_nextTrigger = 0.0f;
        bool m_canPlayFlag = false, m_linkSizeAndDeltaFlag = false;
        
        float m_grainStartFractional = 0, m_grainSizeMS = 100, m_transposeSemiTones = 0, m_pan = 0.5, m_grainGain = 0.8f;
        
        juce::dsp::Reverb m_reverb;
        juce::Reverb::Parameters m_revParams;
        juce::AudioBuffer<float> m_reverbBuffer;
        float m_reverbRoomSize = 0.5f, m_reverbDamping = 0.5f;
    };
#endif /* sjf_granular_h */
