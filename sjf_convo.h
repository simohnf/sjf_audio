//
//  sjf_convo.h
//
//  Created by Simon Fay on 13/04/2023.
//
//  MAC OS ONLY!!!!
//
//  Partitioned convolution algorithm
//      FIR Buffer for first block to ensure zero latency...
//

#ifndef sjf_convo_h
#define sjf_convo_h
#include "sjf_audioUtilities.h"
#include "sjf_interpolators.h"
#include "sjf_delayLine.h"
#include "sjf_buffir2.h" // this uses Apple Accelerate framework
#include <JuceHeader.h>


#include <vector>


//------------------------------------------------
//------------------------------------------------
//------------------------------------------------
template< int NUM_CHANNELS, int FIR_BUFFER_SIZE >
class sjf_convo
{
    
public:
    sjf_convo() { m_formatManager.registerBasicFormats(); };
    ~sjf_convo() {};
   
    void prepare( double sampleRate, int samplesPerBlock )
    {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = samplesPerBlock;
        spec.numChannels = NUM_CHANNELS;
        m_conv.prepare( spec );
        m_conv.reset();
        m_FIRbuffer.setSize( NUM_CHANNELS, samplesPerBlock );
        for ( int c = 0; c < NUM_CHANNELS; c++ )
        {
            m_preDelay[ c ].initialise( sampleRate );
        }
        
    }
   
    void loadImpulse()
    {
        m_chooser = std::make_unique<juce::FileChooser> ("Select a Wave/Aiff file to use as the impulse..." ,
                                                         juce::File{}, "*.aif, *.wav");
        auto chooserFlags = juce::FileBrowserComponent::openMode
        | juce::FileBrowserComponent::canSelectFiles;
        
        m_chooser->launchAsync (chooserFlags, [this] (const juce::FileChooser& fc)
                                {
                                    auto file = fc.getResult();
                                    if (file == juce::File{}) { return; }
                                    std::unique_ptr<juce::AudioFormatReader> reader (m_formatManager.createReaderFor (file));
                                    if (reader.get() != nullptr)
                                    {
                                        m_durationSamps = (int) reader->lengthInSamples;
                                        m_nChannelsInImpulse = (int) reader->numChannels;
                                        m_IRSampleRate = reader->sampleRate;
                                        m_impulseBuffer.setSize( m_nChannelsInImpulse, m_durationSamps );
                                        reader->read (&m_impulseBuffer, 0, (int) reader->lengthInSamples, 0, true, true);
                                        m_samplePath = file.getFullPathName();
                                        m_sampleName = file.getFileName();
                                        m_loadImpulseFlag = true;
                                        setImpulseResponse();
                                    }
                                });
    }
    
    void process( juce::AudioBuffer<float> &buffer )
    {
        auto bufferSize = buffer.getNumSamples();
        auto nBuffChannels = buffer.getNumChannels();
        
        for ( int c = 0; c < nBuffChannels; c++ )
        {
            auto rp = buffer.getReadPointer( c );
            auto wp = buffer.getWritePointer( c );
            bool preDelayIsOn = m_preDelay[ c ].getDelayTimeSamps() > 0 ? true : false;
            for ( int s = 0; s < bufferSize; s++ )
            {
                m_preDelay[ c ].setSample2( rp[ s ] );
                if ( preDelayIsOn )
                {
                    wp[ s ] = m_preDelay[ c ].getSample2();
                }
            }
        }
        
        for ( int c = 0; c < NUM_CHANNELS; c++ )
        {
            m_FIRbuffer.copyFrom( c, 0, buffer, fastMod( c, nBuffChannels ), 0, bufferSize );
            m_FIR[ c ].filterInputBlock( m_FIRbuffer.getWritePointer( c ), bufferSize );
        }
        
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        m_conv.process( ctx );
        for ( int c = 0; c < nBuffChannels; c++ )
        {
            buffer.addFrom( c, 0, m_FIRbuffer, c, 0, bufferSize );
        }
        DBG( "latency " << m_conv.getLatency() );
        DBG( "IR Length Samps " << m_impulseBuffer.getNumSamples()<< " " << m_durationSamps );
    }
    
    void reverseImpulse()
    {
        for ( int c = 0; c < m_nChannelsInImpulse; c++ )
        {
            m_impulseBuffer.reverse( c, 0, m_durationSamps );
        }
        m_loadImpulseFlag = true;
        setImpulseResponse();
    }
    
    void transposeImpulse( float transpositionFactor )
    {
        
    }
    
    void setPreDelay( int preDelayInSamps )
    {
        for ( int c = 0; c < NUM_CHANNELS; c++ )
        {
            m_preDelay[ c ].setDelayTimeSamps( preDelayInSamps );
        }
    }
    
    juce::AudioBuffer< float >& getIRBuffer()
    {
        return m_impulseBuffer;
    }
    
    double getIRSampleRate()
    {
        return m_IRSampleRate;
    }
private:
    void setImpulseResponse()
    {
        if ( !m_loadImpulseFlag ){ return; }
        
        for ( int c = 0; c < NUM_CHANNELS; c++ )
        {
            m_FIR[ c ].setKernel( m_impulseBuffer.getWritePointer( c % m_nChannelsInImpulse ) );
        }
        
        m_fftImpulseBuffer.setSize( NUM_CHANNELS, m_durationSamps - FIR_BUFFER_SIZE );
        for ( int c = 0; c < NUM_CHANNELS; c++ )
        {
            m_fftImpulseBuffer.copyFrom( c, 0, m_impulseBuffer, c%m_nChannelsInImpulse, FIR_BUFFER_SIZE, m_durationSamps - FIR_BUFFER_SIZE );
        }
        m_conv.loadImpulseResponse( juce::AudioBuffer< float >(m_fftImpulseBuffer), m_IRSampleRate, juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::no, juce::dsp::Convolution::Normalise::no );
        
        m_loadImpulseFlag = false;
    }
    
    
    std::array< sjf_delayLine< float >, NUM_CHANNELS > m_preDelay;
    std::array< sjf_buffir< FIR_BUFFER_SIZE >, NUM_CHANNELS > m_FIR;
    
    juce::dsp::ConvolutionMessageQueue m_convBackgroundMessageQueue;
    
    juce::dsp::Convolution m_conv{juce::dsp::Convolution::NonUniform{ FIR_BUFFER_SIZE }, m_convBackgroundMessageQueue };
    
    
    juce::AudioBuffer< float > m_impulseBuffer, m_fftImpulseBuffer, m_FIRbuffer;
    
    bool m_loadImpulseFlag = false;
    double m_IRSampleRate = 0;
    int m_nChannelsInImpulse, m_durationSamps;
    juce::String m_samplePath, m_sampleName;
    juce::AudioFormatManager m_formatManager;
    std::unique_ptr<juce::FileChooser> m_chooser;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_convo )
};




//inline float simdInnerProduct (float* in, float* kernel, int numSamples, float y = 0.0f)
//{
//    constexpr size_t simdN = dsp::SIMDRegister<float>::SIMDNumElements;
//
//    // compute SIMD products
//    int idx = 0;
//    for (; idx <= numSamples - simdN; idx += simdN)
//    {
//        auto simdIn = loadUnaligned (in + idx);
//        auto simdKernel = dsp::SIMDRegister<float>::fromRawArray (kernel + idx);
//        y += (simdIn * simdKernel).sum();
//    }
//
//    // compute leftover samples
//    y = std::inner_product (in + idx, in + numSamples, kernel + idx, y);
//
//    return y;
//    }


#endif /* sjf_convo_h */
