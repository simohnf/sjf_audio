//
//  sjf_audio.cpp
//  
//
//  Created by Simon Fay on 22/07/2022.
//

#include "sjf_audio.hpp"
#include <JuceHeader.h>
#include <math.h>


//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//
//void sjf_delayLine::copyFromDelayBuffer(juce::AudioBuffer<float>& destinationBuffer, float gain)
//{
//    auto bufferSize = destinationBuffer.getNumSamples();
//    auto delayBufferSize = delayBuffer.getNumSamples();
//    auto numChannels = destinationBuffer.getNumChannels();
//
//
//    float delTimeL = delL.getCurrentValue() * SR / 1000.0f;
//    float delTimeR = delR.getCurrentValue() * SR / 1000.0f;
//    //    float read_pos = write_pos;
//    for (int index = 0; index < bufferSize; index++)
//    {
//
//        for (int channel = 0; channel < numChannels; channel++){
//            float channelReadPos;
//            if(channel == 0){
//                channelReadPos = write_pos - delTimeL + index;
//            }
//            else if(channel == 1){
//                channelReadPos =  write_pos - delTimeR + index;
//            }
//            else{
//                return;
//            }
//            while (channelReadPos < 0)
//            {
//                channelReadPos += delayBufferSize;
//            }
//            while (channelReadPos >= delayBufferSize)
//            {
//                channelReadPos -= delayBufferSize;
//            }
//            auto val = cubicInterpolate(delayBuffer, channel, channelReadPos) * gain;
//            destinationBuffer.addSample(channel, index, val );
//        }
//        delTimeL = delL.getNextValue() * SR / 1000.0f;
//        delTimeR = delR.getNextValue() * SR / 1000.0f;
//    }
//}

//==============================================================================
//==============================================================================

//void sjf_delayLine::copyFromDelayBuffer(juce::AudioBuffer<float>& destinationBuffer, float del_time_ms, float gain)
//{
//    auto bufferSize = destinationBuffer.getNumSamples();
//    auto delayBufferSize = delayBuffer.getNumSamples();
//    auto numChannels = destinationBuffer.getNumChannels();
//
//    float del_time_samps = ( SR *  del_time_ms / 1000 );
//    float stSpread = ( stereo_spread/100.0f ) * del_time_samps * 0.25 ;
//
//    float delTimeL = delL.smooth(del_time_samps + stSpread);
//    float delTimeR = delR.smooth(del_time_samps - stSpread);;
//    //    float read_pos = write_pos;
//    for (int index = 0; index < bufferSize; index++)
//    {
//
//        for (int channel = 0; channel < numChannels; channel++){
//            float channelReadPos;
//            if(channel == 0){
//                channelReadPos = write_pos - delTimeL + index;
//            }
//            else if(channel == 1){
//                channelReadPos =  write_pos - delTimeR + index;
//            }
//            else{
//                return;
//            }
//            while (channelReadPos < 0)
//            {
//                channelReadPos += delayBufferSize;
//            }
//            while (channelReadPos >= delayBufferSize)
//            {
//                channelReadPos -= delayBufferSize;
//            }
//            auto val = cubicInterpolate(delayBuffer, channel, channelReadPos) * gain;
//            destinationBuffer.addSample(channel, index, val );
//        }
//        delTimeL = delL.smooth(del_time_samps + stSpread);
//        delTimeR = delR.smooth(del_time_samps - stSpread);;
//    }
//}

//==============================================================================
//==============================================================================

//void sjf_stereoDelay::intialise( int sampleRate , int totalNumInputChannels, int totalNumOutputChannels, int samplesPerBlock)
//{
//    SR = sampleRate;
//    
//    delayLine.intialise(sampleRate, totalNumInputChannels, totalNumOutputChannels, samplesPerBlock);
//    
//    tempBuffer.setSize(totalNumOutputChannels, samplesPerBlock);
//    
//    juce::dsp::ProcessSpec spec;
//    spec.maximumBlockSize = samplesPerBlock;
//    spec.numChannels = totalNumOutputChannels;
//    spec.sampleRate = sampleRate;
//    filter.reset();
//    filter.prepare(spec);
//    
//    drive.reset(sampleRate, 0.001f);
//    drive.setCurrentAndTargetValue(0.0f);
//    
//    fb.reset(sampleRate, 0.001f);
//    fb.setCurrentAndTargetValue(20.0f);
//    
//    lpfCutOff.reset(sampleRate, 0.01f);
//    lpfCutOff.setCurrentAndTargetValue(10000.0f);
//    
//    
//    wet_level.reset(sampleRate, 0.0f);
//    wet_level.setCurrentAndTargetValue(40.0f);
//    
//    dry_level.reset(sampleRate, 0.001f);
//    dry_level.setCurrentAndTargetValue(100.0f);
//    
//    
//    
//    lfoD.reset(sampleRate, 0.001f);
//    lfoD.setCurrentAndTargetValue(0.001f);
//    
//    lfoF.reset( sampleRate, 0.001f );
//    lfoF.setCurrentAndTargetValue( 3.0f );
//    
//    lfo.initialise( sampleRate, lfoF.getCurrentValue() );
//}
//
////==============================================================================
////==============================================================================
//
//void sjf_stereoDelay::delay( juce::AudioBuffer<float>& buffer, int totalNumInputChannels, int totalNumOutputChannels )
//{
//    if (tempBuffer.getNumSamples()!= buffer.getNumSamples())
//    {
//        tempBuffer.setSize(buffer.getNumChannels(), buffer.getNumSamples());
//    }
//    tempBuffer.clear();
//    
//    juce::dsp::AudioBlock<float> block ( tempBuffer );
//    
//    
//    delayLine.writeToDelayBuffer( buffer, 1.0f ); // write Dry to delBuffer
//    
//    
//    lfo.setFrequency(lfoF.smooth(lfo_frequency));
//    float LFOSamp = lfo.outputSample(buffer.getNumSamples());
//    float dT = del_time_ms + ( LFOSamp * del_time_ms * lfoD.smooth(lfo_depth / 100.0f) );
//    
//    delayLine.copyFromDelayBuffer( tempBuffer, dT, 1.0f ); // add delayed signal to tempbuffer
//    
//    driver.distort( tempBuffer, drive.getNextValue()/100.0f );
//    
//    
//    filter.state->setCutOffFrequency( SR, lpfCutOff.getNextValue(), 0.1f );
//    filter.process( juce::dsp::ProcessContextReplacing<float> (block) );
//    
//    delayLine.addToDelayBuffer( tempBuffer, fb.getNextValue()/100.0f );
//    
//    addBuffers( buffer, tempBuffer, dry_level.getNextValue()/100.0f , wet_level.getNextValue()/100.0f );
//    
//    delayLine.updateBufferPositions( buffer.getNumSamples() );
//}
