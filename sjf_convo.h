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
#include "sjf_lpf.h"
#include <JuceHeader.h>


#include <vector>


//------------------------------------------------
//------------------------------------------------
//------------------------------------------------
template< int NUM_CHANNELS, int FIR_BUFFER_SIZE >
class sjf_convo
{
    
public:
    sjf_convo()
    {
        m_formatManager.registerBasicFormats();
        m_lpf.setCutoff( 0.999f );
        m_hpf.setCutoff( 0.001f );
    };
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
                                        m_impulseBufferOriginal.makeCopyOf( m_impulseBuffer ); // store for future editing
                                        m_samplePath = file.getFullPathName();
                                        m_sampleName = file.getFileName();
                                        m_loadImpulseFlag = true;
                                        m_reverseFlag = false;
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
        auto nSamps = m_impulseBuffer.getNumSamples();
        for ( int c = 0; c < m_nChannelsInImpulse; c++ )
        {
            m_impulseBuffer.reverse( c, 0, nSamps );
        }
        m_loadImpulseFlag = true;
        m_reverseFlag = !m_reverseFlag;
        setImpulseResponse();
    }
    
    void stretchImpulse( float stretchFactor )
    {
        // need to do this
    }
    
    
    void trimImpulseEnd( )
    {
        int indx =0,  indxC = 0;
        for ( int c = 0; c < m_nChannelsInImpulse; c++ )
        {
            int s = m_durationSamps - 1;
            auto db = 20*log10( abs(m_impulseBufferOriginal.getSample( c, s ) ) );
            while (  (db < -1*80) && s > 0 )
            {
                indxC = s;
                
                
                db = 20*log10( abs(m_impulseBufferOriginal.getSample( c, s ) ) );
                s--;
            }
            if ( indxC > indx ){ indx = indxC; }
        }
        if ( indx == 0 )
        { return; }
        int newLen = indx + 1; // add one sample for safety
        m_impulseBuffer.setSize( m_nChannelsInImpulse, newLen );
        for ( int c = 0; c < m_nChannelsInImpulse; c++ )
        { 
            m_impulseBuffer.copyFrom( c, 0, m_impulseBufferOriginal, c, 0, newLen );
        }
        if ( m_reverseFlag )
        {
            m_reverseFlag = !m_reverseFlag;
            reverseImpulse();
        }
        else
        {
            setImpulseResponse();
        }
    }
    
    
    void setPreDelay( int preDelayInSamps )
    {
        for ( int c = 0; c < NUM_CHANNELS; c++ )
        {
            m_preDelay[ c ].setDelayTimeSamps( preDelayInSamps );
        }
    }
    
    void setLPFCutoff( float cutoffCoefficient )
    {
        m_lpf.setCutoff( cutoffCoefficient );
        for ( int c = 0; c < m_nChannelsInImpulse; c++ )
        {
            m_impulseBuffer.copyFrom( c, 0, m_impulseBufferOriginal, c, 0, m_impulseBuffer.getNumChannels() );
        }
        setImpulseResponse();
    }
    
    float setLPFCutoff( )
    {
        return m_lpf.getCutoff( );
    }
    
    void setHPFCutoff( float cutoffCoefficient )
    {
        m_hpf.setCutoff( cutoffCoefficient );
        for ( int c = 0; c < m_nChannelsInImpulse; c++ )
        {
            m_impulseBuffer.copyFrom( c, 0, m_impulseBufferOriginal, c, 0, m_impulseBuffer.getNumChannels() );
        }
        setImpulseResponse();
    }
    
    void getHPFCutoff( float cutoffCoefficient )
    {
        return m_hpf.getCutoff( );
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
        
        auto nSamps = m_impulseBuffer.getNumSamples();
        // filter buffer here
        // havent checked to make sure this works
        for ( int c = 0; c < m_nChannelsInImpulse; c++ )
        {
            auto ptr = m_impulseBuffer.getWritePointer( c );
            for ( int s = 0; s < nSamps; s++ )
            {
                m_lpf.filterInPlace( ptr[ s ] );
                m_hpf.filterInPlaceHP( ptr[ s ] );
            }
        }
        
        for ( int c = 0; c < NUM_CHANNELS; c++ )
        {
            m_FIR[ c ].setKernel( m_impulseBuffer.getWritePointer( c % m_nChannelsInImpulse ) );
        }
        
        m_fftImpulseBuffer.setSize( NUM_CHANNELS, nSamps - FIR_BUFFER_SIZE );
        for ( int c = 0; c < NUM_CHANNELS; c++ )
        {
            m_fftImpulseBuffer.copyFrom( c, 0, m_impulseBuffer, c%m_nChannelsInImpulse, FIR_BUFFER_SIZE, nSamps - FIR_BUFFER_SIZE );
        }
        m_conv.loadImpulseResponse( juce::AudioBuffer< float >(m_fftImpulseBuffer), m_IRSampleRate, juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::no, juce::dsp::Convolution::Normalise::no );
        
        m_loadImpulseFlag = false;
    }
    
    
    std::array< sjf_delayLine< float >, NUM_CHANNELS > m_preDelay;
    std::array< sjf_buffir< FIR_BUFFER_SIZE >, NUM_CHANNELS > m_FIR;
    
    juce::dsp::ConvolutionMessageQueue m_convBackgroundMessageQueue;
    
    juce::dsp::Convolution m_conv{juce::dsp::Convolution::NonUniform{ FIR_BUFFER_SIZE }, m_convBackgroundMessageQueue };
    
    
    juce::AudioBuffer< float > m_impulseBuffer, m_impulseBufferOriginal, m_fftImpulseBuffer, m_FIRbuffer;
    
    bool m_loadImpulseFlag = false, m_reverseFlag = false;
    double m_IRSampleRate = 0;
    int m_nChannelsInImpulse, m_durationSamps;
    juce::String m_samplePath, m_sampleName;
    juce::AudioFormatManager m_formatManager;
    std::unique_ptr<juce::FileChooser> m_chooser;
    sjf_lpf m_lpf, m_hpf;
    
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
