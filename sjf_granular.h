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
    // Initialise a new grain
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
    // Initialise a new grain
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
    // The diffeent types of envelopes that can be applied to each grain
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
    // play an individual grain
    void playGrain(juce::AudioBuffer<float> &buffer, juce::AudioBuffer<float> &sourceBuffer )
    {
        if (!m_isPlayingFlag){ return; } // if this voice isn't playing move on
        auto outBufferSize = buffer.getNumSamples();
        auto sourceBufferSize = sourceBuffer.getNumSamples();
        auto numChannels = buffer.getNumChannels();
        
        for (int index = 0; index < outBufferSize; index ++)
        {
            auto envVal = grainEnv( m_samplesPlayedCount/m_grainLength, m_envType ); // calculate envelope gain based on current sample position
            for (int channel = 0; channel < numChannels; channel ++)
            {
                auto val = linearInterpolate(sourceBuffer, channel % sourceBuffer.getNumChannels(), m_readPos); // calculate the current sample value
                val *= m_gain;
                val *= envVal;
                val *= pan2( m_pan, channel);
                buffer.addSample( channel, index, val );
            }
            m_readPos += m_playBackSpeed;
            while ( m_readPos >= sourceBufferSize ) { m_readPos -= sourceBufferSize; } // just for safety so we don't go over the end of the sample
            
            m_samplesPlayedCount++;
            if (m_samplesPlayedCount >= m_grainLength) // once we pass the grain length turn this voice off
            {
                m_isPlayingFlag = false;
                return;
            }
        }
    }
    //==============================================================================
    // play an individual grain one sample at a time
    void playGrain(juce::AudioBuffer<float> &buffer, juce::AudioBuffer<float> &sourceBuffer, int index /*, int g */ )
    {
        if (!m_isPlayingFlag){ return; } // if this voice isn't playing move on
        auto sourceBufferSize = sourceBuffer.getNumSamples();
        auto numChannels = buffer.getNumChannels();
        auto envVal = grainEnv( m_samplesPlayedCount/m_grainLength, m_envType ); // calculate envelope gain based on current sample position
        for (int channel = 0; channel < numChannels; channel ++)
        {
            auto val = linearInterpolate(sourceBuffer, channel % sourceBuffer.getNumChannels(), m_readPos); // calculate envelope gain based on current sample position
            val *= m_gain;
            val *= envVal;
            val *= pan2( m_pan, channel);
            buffer.addSample( channel, index, val );
        }
        m_readPos += m_playBackSpeed;
        while (m_readPos >= sourceBufferSize) { m_readPos -= sourceBufferSize; } // just for safety so we don't go over the end of the sample
        
        m_samplesPlayedCount++;
        if (m_samplesPlayedCount >= m_grainLength) // once we pass the grain length turn this voice off
        {
            m_isPlayingFlag = false;
            return;
        }
    }
    //==============================================================================
    // play an individual grain one sample at a time with reverb
    void playGrain(juce::AudioBuffer<float> &buffer, juce::AudioBuffer<float> &sourceBuffer, juce::AudioBuffer<float> &reverbBuffer, int index )
    {
        if (!m_isPlayingFlag){ return; } // if this voice isn't playing move on
        auto sourceBufferSize = sourceBuffer.getNumSamples();
        auto numChannels = buffer.getNumChannels();
        auto envVal = grainEnv( m_samplesPlayedCount/m_grainLength, m_envType ); // calculate envelope gain based on current sample position
        for (int channel = 0; channel < numChannels; channel ++)
        {
            auto val = linearInterpolate(sourceBuffer, channel % sourceBuffer.getNumChannels(), m_readPos); // calculate envelope gain based on current sample position
            val *= ( m_gain * envVal * pan2( m_pan, channel) );
//            val *= envVal;
//            val *= pan2( m_pan, channel);
            buffer.addSample( channel, index, (1.0f-m_reverbAmount) * val ); // add dry signal to main buffer
            reverbBuffer.addSample( channel, index, (m_reverbAmount) * val ); // add wet signal to reverb buffer
        }
        m_readPos += m_playBackSpeed; // increment read_position by playback speed
        while (m_readPos >= sourceBufferSize) { m_readPos -= sourceBufferSize; } // just for safety so we don't go over the end of the sample
        
        m_samplesPlayedCount++; // increment sample count to compare with grain length
        if (m_samplesPlayedCount >= m_grainLength) // once we pass the grain length turn this voice off
        {
            m_isPlayingFlag = false;
            return;
        }
    }
    //==============================================================================
    bool getPlayingState() { return m_isPlayingFlag; }
    //==============================================================================
private:
    bool m_isPlayingFlag = false;
    float m_readPos = 0.0f, m_grainLength = 4410.0f, m_playBackSpeed = 1.0f, m_gain = 1.0f, m_pan = 0.5f, m_samplesPlayedCount = 0.0f, m_reverbAmount = 0.0f;
    int m_envType = 0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (sjf_grainVoice)
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
        sjf_grainEngine() /*: m_grains(128) */
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
        // turn on new grain voice so it wil play a grain
        void newGrain(float grainStartFractional, float grainLengthMS, float transpositionInSemitones, float gain, float pan, int envType)
        {
            if (!m_sampleLoadedFlag) { return; }
            auto grainStartSamps = m_AudioSample.getNumSamples() * grainStartFractional;
            auto playBackSpeed = pow(2.0f, transpositionInSemitones/12.0f);
            m_grains[m_voiceNumber].setPlaying( true, grainStartSamps, grainLengthMS * m_SR/1000.0f, playBackSpeed, gain, pan, envType );
            m_voiceNumber++;
            m_voiceNumber %= m_nGrainVoices;
            return;
        }
        //==============================================================================
        // turn on new grain voice so it wil play a grain with reverb
        void newGrain( float grainStartFractional, float grainLengthMS, float transpositionInSemitones, float gain, float pan, int envType, float reverbAmount )
        {
            if (!m_sampleLoadedFlag) { return; }
            auto grainStartSamps = m_AudioSample.getNumSamples() * grainStartFractional;
            auto playBackSpeed = pow(2.0f, transpositionInSemitones/12.0f);
            m_grains[m_voiceNumber].setPlaying( true, grainStartSamps, grainLengthMS * m_SR/1000.0f, playBackSpeed, gain, pan, envType, reverbAmount );
            m_voiceNumber++;
            m_voiceNumber %= m_nGrainVoices;
            return;
        }
        //==============================================================================
        // play all of the currently active grains
        void playGrains( juce::AudioBuffer<float> &buffer )
        {
            if (!m_sampleLoadedFlag) { return; }
            for (int index = 0; index < buffer.getNumSamples(); index++)
            {
                for ( int g = 0; g < m_nGrainVoices; g++ )
                {
                    m_grains[g].playGrain( buffer, m_AudioSample, index );
                }
            }
        }

        //==============================================================================
        //==============================================================================
        //==============================================================================
        // This is the main function used to trigger clouds from vectors
        void playCloudFromVectors( juce::AudioBuffer<float> &buffer )
        {
            buffer.clear();
            if (!m_sampleLoadedFlag || !m_canPlayFlag)
            { // if either there is no sample loaded or no cloud has been triggered, reset things and turn the synth off
                m_canPlayFlag = false;
                // Continue to run reverb just so the tail isn't cut off unnaturally
                auto block = juce::dsp::AudioBlock<float> ( buffer );
                auto context = juce::dsp::ProcessContextReplacing<float> (block);
                m_reverb.process( context );
                return;
            }
            m_reverbBuffer.makeCopyOf( buffer ); // I copy the empty buffer into the reverb buffer to ensure they are the same size...
            auto cloudLengthSamps = m_cloudLengthMS * (static_cast<float>m_SR * 0.001f);
            auto deltaTimeSamps = m_deltaTimeMS * (static_cast<float>m_SR * 0.001f);
            
            for ( int index = 0; index < buffer.getNumSamples(); index ++ )
            {
                if (m_cloudPos >= m_nextTrigger && m_cloudPos <= (cloudLengthSamps - deltaTimeSamps))
                {// only trigger a new grain if we're still within the cloud length
                    auto phaseThroughCloud = static_cast<float>(m_cloudPos) / static_cast<float>(cloudLengthSamps);
                    // calculate time to next grain
                    m_deltaTimeMS = 1.0f + 99.0f * linearInterpolate( m_grainDeltaVector, m_grainDeltaVector.size() * phaseThroughCloud );
                    // calculate the time at which the next grain should be calculated
                    m_nextTrigger = m_cloudPos + (m_deltaTimeMS * m_SR * 0.001f);
                    // calculate the position within the sample at which the new gain should be started
                    auto grainStart = linearInterpolate( m_grainPositionVector, m_grainPositionVector.size() * phaseThroughCloud );
                    // calculate the panning for the new grain
                    auto grainPan = linearInterpolate( m_grainPanVector, m_grainPanVector.size() * phaseThroughCloud );
                    // calculate the transposition for the new grain
                    auto grainTransposition = linearInterpolate( m_grainTranspositionVector, m_grainTranspositionVector.size() * phaseThroughCloud );
                    grainTransposition *= 24.0f;
                    grainTransposition -= 12.0f;
                    // calculate how long the new grain should play for
                    auto grainSize = linearInterpolate( m_grainSizeVector, m_grainSizeVector.size() * phaseThroughCloud );
                    // if linked grain size is a maximum of 10 times delta time
                    if (m_linkSizeAndDeltaFlag) { grainSize = 1.0f + fmin( grainSize * m_deltaTimeMS * 10.0f , grainSize * 99.0f ); }
                    else { grainSize = 1.0f + (grainSize * 99.0f); }
                    // calculate the gain of the new grain
                    auto grainGain = linearInterpolate( m_grainGainVector, m_grainGainVector.size() * phaseThroughCloud );
                    // calculate the reverb amount for the new grain
                    auto grainReverb = linearInterpolate( m_grainReverbVector, m_grainReverbVector.size() * phaseThroughCloud );
                    // turn on the voice for the new grain
                    newGrain( grainStart, grainSize, grainTransposition, grainGain, grainPan, m_envType, grainReverb );
                }
                
                //
                for ( int g = 0; g < m_nGrainVoices; g++ )
                { // run through each grain voice and get it to play (assuming it is active of course)
                    m_grains[g].playGrain(buffer, m_AudioSample, m_reverbBuffer, index);
                }
                
                
                m_cloudPos ++; // increment cloud position
                if ( m_cloudPos >= (cloudLengthSamps - deltaTimeSamps) )
                { // if we have reached the end of the cloud, start turning the synth off
                    int playingCount = 0;
                    for ( int g = 0; g < m_nGrainVoices; g++ )
                    { // go through each grain voice and check whether it is playing or not
                        if ( m_grains[g].getPlayingState() ) { playingCount++; } // if any of the grains are still playing let them finish
                    }
                    if (playingCount == 0)
                    { // if no voices are playing turn the synth of and go to the end of the cloud (this step is just for the display)
                        m_canPlayFlag = false;
                        m_cloudPos = cloudLengthSamps;
                    }
                }
            }
            // apply creverb to the reverb buffer
            auto block = juce::dsp::AudioBlock<float> (m_reverbBuffer);
            auto context = juce::dsp::ProcessContextReplacing<float> (block);
            m_reverb.process( context );
            
            for (int channel = 0; channel < buffer.getNumChannels(); channel ++)
            { // add the two buffers together
                buffer.addFrom( channel, 0, m_reverbBuffer, channel, 0, buffer.getNumSamples() );
            }
        }
        //==============================================================================
        //==============================================================================
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
        void linkSizeAndDeltaTime( bool deltaTimeIsLinkedToGrainSize )
        { m_linkSizeAndDeltaFlag = deltaTimeIsLinkedToGrainSize; }
        //==============================================================================
        int getEnvType( ) { return 1 + m_envType; } // 1 + m_envType because of the Juce combobox
        //==============================================================================
        //==============================================================================
        //==============================================================================
        //==============================================================================
        //==============================================================================
        //==============================================================================
        //==============================================================================
        // Functions for non vector based clouds
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
        // play the cloud of grains
        void playCloud( juce::AudioBuffer<float> &buffer )
        {
            if (!m_sampleLoadedFlag || !m_canPlayFlag) { m_canPlayFlag = false;  return; }
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
                    newGrain( startWithJitter, sizeWithJitter, transposeWithJitter, gainWithJitter, panWithJitter, m_envType );
                    m_deltaTimeMS = rand01()*90 + 10; // random deltaTime
                    m_nextTrigger = m_cloudPos + (m_deltaTimeMS * m_SR * 0.001f);
                }
                
                
                for ( int g = 0; g < m_nGrainVoices; g++ )
                {
                    m_grains[g].playGrain( buffer, m_AudioSample, index );
                }
                
                
                m_cloudPos ++;
                if ( m_cloudPos >= (cloudLengthSamps - deltaTimeSamps) )
                {
                    int playingCount = 0;
                    for ( int g = 0; g < m_nGrainVoices; g++ )
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
        //==============================================================================
        //==============================================================================
        //==============================================================================
        //==============================================================================
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
            return m_cloudPos / ( m_cloudLengthMS  * 0.001f * static_cast<float>(m_SR) );
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
        
    private:
        static const int m_nGrainVoices = 128;
//        std::vector< sjf_grainVoice > m_grains;
        sjf_grainVoice m_grains[ m_nGrainVoices ];
        std::vector<float> m_grainPositionVector, m_grainPanVector, m_grainTranspositionVector, m_grainSizeVector, m_grainGainVector, m_grainDeltaVector, m_grainReverbVector;
        int m_voiceNumber = 0, m_envType = 0, m_samplesPerBlock = 128;
        double m_cloudLengthMS = 2000.0f, m_desnity = 1.0f, m_deltaTimeMS = 50.0f, m_cloudPos = 0.0f, m_nextTrigger = 0.0f;
        bool m_canPlayFlag = false, m_linkSizeAndDeltaFlag = false;
        
        float m_grainStartFractional = 0, m_grainSizeMS = 100, m_transposeSemiTones = 0, m_pan = 0.5, m_grainGain = 0.8f;
        
        juce::dsp::Reverb m_reverb;
        juce::Reverb::Parameters m_revParams;
        juce::AudioBuffer<float> m_reverbBuffer;
        float m_reverbRoomSize = 0.5f, m_reverbDamping = 0.5f;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (sjf_grainEngine)
    };
#endif /* sjf_granular_h */
