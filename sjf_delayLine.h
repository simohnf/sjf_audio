//
//  sjf_delayLine.h
//  sjf_granSynth
//
//  Created by Simon Fay on 24/08/2022.
//

#ifndef sjf_delayLine_h
#define sjf_delayLine_h

class sjf_delayLine {
    
public:
    sjf_delayLine()
    {
        delayBuffer.setSize(2, SR * del_buff_len );
        delayBuffer.clear();
        
        delL.reset( SR, 0.02f ) ;
        delR.reset( SR, 0.02f ) ;
        
        delL.setCurrentAndTargetValue( 500.0f ) ;
        delR.setCurrentAndTargetValue( 500.0f ) ;
    };
    //==============================================================================
    virtual ~sjf_delayLine() {};
    //==============================================================================
    virtual void intialise( int sampleRate , int totalNumInputChannels, int totalNumOutputChannels, int samplesPerBlock)
    {
        if (sampleRate > 0 ) { SR = sampleRate; }
        delayBuffer.setSize( totalNumOutputChannels, SR * del_buff_len );
        delayBuffer.clear();
        
        delL.reset( SR, 0.02f ) ;
        delR.reset( SR, 0.02f ) ;
    };
    //==============================================================================
    void writeToDelayBuffer(juce::AudioBuffer<float>& sourceBuffer, float gain)
    {
        auto bufferSize = sourceBuffer.getNumSamples();
        auto delayBufferSize = delayBuffer.getNumSamples();
        for (int index = 0; index < bufferSize; index++)
        {
            auto wp = (write_pos + index) % delayBufferSize;
            for (int channel = 0; channel < sourceBuffer.getNumChannels(); channel++)
            {
                delayBuffer.setSample(channel % delayBuffer.getNumChannels(), wp, (sourceBuffer.getSample(channel, index) * gain) );
            }
        }
    };
    //==============================================================================
    void copyFromDelayBuffer(juce::AudioBuffer<float>& destinationBuffer, float gain)
    {
        auto bufferSize = destinationBuffer.getNumSamples();
        auto delayBufferSize = delayBuffer.getNumSamples();
        auto numChannels = destinationBuffer.getNumChannels();
        auto delTimeL = delL.getCurrentValue() * SR / 1000.0f;
        auto delTimeR = delR.getCurrentValue() * SR / 1000.0f;
        for (int index = 0; index < bufferSize; index++)
        {
            for (int channel = 0; channel < numChannels; channel++)
            {
                float channelReadPos;
                if( channel == 0 ) { channelReadPos = write_pos - delTimeL + index; }
                else if(channel == 1) { channelReadPos =  write_pos - delTimeR + index; }
                else { return; }
                while ( channelReadPos < 0 ) { channelReadPos += delayBufferSize; }
                while (channelReadPos >= delayBufferSize) { channelReadPos -= delayBufferSize; }
                auto val = cubicInterpolate(delayBuffer, channel, channelReadPos) * gain;
                destinationBuffer.addSample(channel, index, val );
            }
            delTimeL = delL.getNextValue() * SR / 1000.0f;
            delTimeR = delR.getNextValue() * SR / 1000.0f;
        }
    };
    //==============================================================================
    void addToDelayBuffer (juce::AudioBuffer<float>& sourceBuffer, float gain)
    {
        auto bufferSize = sourceBuffer.getNumSamples();
        auto delayBufferSize = delayBuffer.getNumSamples();
        
        for (int channel = 0; channel < sourceBuffer.getNumChannels(); ++channel)
        {
            for (int index = 0; index < bufferSize; index ++)
            {
                float value = sourceBuffer.getSample(channel, index) * gain;
                delayBuffer.addSample(channel, ( (write_pos + index) % delayBufferSize ) , value );
            }
        }
    };
    //==============================================================================
    void addToDelayBuffer (juce::AudioBuffer<float>& sourceBuffer, float gain1, float gain2)
    {
        auto bufferSize = sourceBuffer.getNumSamples();
        auto delayBufferSize = delayBuffer.getNumSamples();
        
        for (int channel = 0; channel < sourceBuffer.getNumChannels(); ++channel)
        {
            float gain;
            if (channel == 0){ gain = gain1; }
            else{ gain = gain2; }
            for (int index = 0; index < bufferSize; index ++)
            {
                float value = sourceBuffer.getSample(channel, index) * gain;
                delayBuffer.addSample(channel, ( (write_pos + index) % delayBufferSize ) , value );
            }
        }
    };
    //==============================================================================
    void updateBufferPositions(int bufferSize)
    {
        auto delayBufferSize = delayBuffer.getNumSamples();
        //    Update write position ensuring it stays within size of delay buffer
        write_pos += bufferSize;
        write_pos %= delayBufferSize;
    };
    //==============================================================================
    // set and retrieve parameters
    void setDelTimeL( float delLMS) { delL.setTargetValue( delLMS ); }
    void setDelTimeR( float delRMS) { delR.setTargetValue( delRMS ); }
    float getDelTimeL( ) { return delL.getTargetValue( ); }
    float getDelTimeR( ) { return delR.getTargetValue( ); }
    void clearBuffer() { delayBuffer.clear(); }
    //==============================================================================
    
    
protected:
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> delL, delR;
    juce::AudioBuffer<float> delayBuffer; // This is the circular buffer for the delay
    
    int SR = 44100;
    int write_pos = 0; // this is the index to write to in the "delayBuffer"
    const int del_buff_len = 3; // Maximum delay time equals 2 seconds plus 1 second for safety with stereo spread increase
};

#endif /* sjf_delayLine_h */
