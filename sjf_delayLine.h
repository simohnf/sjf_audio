//
//  sjf_delayLine.h
//
//  Created by Simon Fay on 24/08/2022.
//

#ifndef sjf_delayLine_h
#define sjf_delayLine_h

class sjf_delayLine {
    
public:
    sjf_delayLine()
    {
        m_delayBuffer.setSize(2, m_SR * m_delBufferLength );
        m_delayBuffer.clear();
        
        m_delL.reset( m_SR, 0.02f ) ;
        m_delR.reset( m_SR, 0.02f ) ;
        
        m_delL.setCurrentAndTargetValue( 500.0f ) ;
        m_delR.setCurrentAndTargetValue( 500.0f ) ;
    };
    //==============================================================================
    virtual ~sjf_delayLine() {};
    //==============================================================================
    virtual void intialise( int sampleRate , int totalNumInputChannels, int totalNumOutputChannels, int samplesPerBlock)
    {
        if (sampleRate > 0 ) { m_SR = sampleRate; }
        m_delayBuffer.setSize( totalNumOutputChannels, m_SR * m_delBufferLength );
        m_delayBuffer.clear();
        
        m_delL.reset( m_SR, 0.02f ) ;
        m_delR.reset( m_SR, 0.02f ) ;
    };
    //==============================================================================
    void writeToDelayBuffer(juce::AudioBuffer<float>& sourceBuffer, float gain)
    {
        auto bufferSize = sourceBuffer.getNumSamples();
        auto delayBufferSize = m_delayBuffer.getNumSamples();
        for (int index = 0; index < bufferSize; index++)
        {
            auto wp = (m_writePos + index) % delayBufferSize;
            for (int channel = 0; channel < sourceBuffer.getNumChannels(); channel++)
            {
                m_delayBuffer.setSample(channel % m_delayBuffer.getNumChannels(), wp, (sourceBuffer.getSample(channel, index) * gain) );
            }
        }
    };
    //==============================================================================
    void copyFromDelayBuffer(juce::AudioBuffer<float>& destinationBuffer, float gain)
    {
        auto bufferSize = destinationBuffer.getNumSamples();
        auto delayBufferSize = m_delayBuffer.getNumSamples();
        auto numChannels = destinationBuffer.getNumChannels();
        auto delTimeL = m_delL.getCurrentValue() * m_SR / 1000.0f;
        auto delTimeR = m_delR.getCurrentValue() * m_SR / 1000.0f;
        for (int index = 0; index < bufferSize; index++)
        {
            for (int channel = 0; channel < numChannels; channel++)
            {
                float channelReadPos;
                if( channel == 0 ) { channelReadPos = m_writePos - delTimeL + index; }
                else if(channel == 1) { channelReadPos =  m_writePos - delTimeR + index; }
                else { return; }
                while ( channelReadPos < 0 ) { channelReadPos += delayBufferSize; }
                while (channelReadPos >= delayBufferSize) { channelReadPos -= delayBufferSize; }
                auto val = cubicInterpolate(m_delayBuffer, channel, channelReadPos) * gain;
                destinationBuffer.addSample(channel, index, val );
            }
            delTimeL = m_delL.getNextValue() * m_SR / 1000.0f;
            delTimeR = m_delR.getNextValue() * m_SR / 1000.0f;
        }
    };
    //==============================================================================
    void addToDelayBuffer (juce::AudioBuffer<float>& sourceBuffer, float gain)
    {
        auto bufferSize = sourceBuffer.getNumSamples();
        auto delayBufferSize = m_delayBuffer.getNumSamples();
        
        for (int channel = 0; channel < sourceBuffer.getNumChannels(); ++channel)
        {
            for (int index = 0; index < bufferSize; index ++)
            {
                float value = sourceBuffer.getSample(channel, index) * gain;
                m_delayBuffer.addSample(channel, ( (m_writePos + index) % delayBufferSize ) , value );
            }
        }
    };
    //==============================================================================
    void addToDelayBuffer (juce::AudioBuffer<float>& sourceBuffer, float gain1, float gain2)
    {
        auto bufferSize = sourceBuffer.getNumSamples();
        auto delayBufferSize = m_delayBuffer.getNumSamples();
        
        for (int channel = 0; channel < sourceBuffer.getNumChannels(); ++channel)
        {
            float gain;
            if (channel == 0){ gain = gain1; }
            else{ gain = gain2; }
            for (int index = 0; index < bufferSize; index ++)
            {
                float value = sourceBuffer.getSample(channel, index) * gain;
                m_delayBuffer.addSample(channel, ( (m_writePos + index) % delayBufferSize ) , value );
            }
        }
    };
    //==============================================================================
    void updateBufferPositions(int bufferSize)
    {
        auto delayBufferSize = m_delayBuffer.getNumSamples();
        //    Update write position ensuring it stays within size of delay buffer
        m_writePos += bufferSize;
        m_writePos %= delayBufferSize;
    };
    //==============================================================================
    // set and retrieve parameters
    void setDelTimeL( float delLMS) { m_delL.setTargetValue( delLMS ); }
    void setDelTimeR( float delRMS) { m_delR.setTargetValue( delRMS ); }
    float getDelTimeL( ) { return m_delL.getTargetValue( ); }
    float getDelTimeR( ) { return m_delR.getTargetValue( ); }
    void clearBuffer() { m_delayBuffer.clear(); }
    //==============================================================================
    
    
protected:
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> m_delL, m_delR;
    juce::AudioBuffer<float> m_delayBuffer; // This is the circular buffer for the delay
    
    int m_SR = 44100;
    int m_writePos = 0; // this is the index to write to in the "m_delayBuffer"
    const int m_delBufferLength = 3; // Maximum delay time equals 2 seconds plus 1 second for safety with stereo spread increase
};

#endif /* sjf_delayLine_h */
