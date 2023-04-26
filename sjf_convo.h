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
#include "sjf_interpolationTypes.h"
#include "sjf_delayLine.h"
#include "sjf_buffir2.h" // this uses Apple Accelerate framework
#include "sjf_lpf.h"
#include <JuceHeader.h>


#include <vector>

// need to check FFT reset position because occassionally it throws error in debug???
//------------------------------------------------//------------------------------------------------
//------------------------------------------------//------------------------------------------------
//------------------------------------------------//------------------------------------------------
template< int NUM_CHANNELS, int FIR_BUFFER_SIZE >
class sjf_convo
{
    
public:
    sjf_convo( bool shouldTrimImpulse = false )
    {
        m_trimFlag = shouldTrimImpulse;
        m_formatManager.registerBasicFormats();
        for ( int c = 0; c < NUM_CHANNELS; c++ )
        {
            m_lpf[ c ].setCutoff( 0.999f );
            m_hpf[ c ].setCutoff( 0.001f );
        }
        
        m_env.resize( 2 ); // default envelope
        m_env[ 0 ] = { 0, 1 };
        m_env[ 1 ] = { 1, 1 };
    };
    //------------------------------------------------//------------------------------------------------
    ~sjf_convo() {};
   //------------------------------------------------//------------------------------------------------
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
    //==============================================================================
    void loadSample( juce::Value path )
    {
        juce::File file( path.getValue().toString() );
        if (file == juce::File{}) { return; }
        std::unique_ptr<juce::AudioFormatReader> reader (m_formatManager.createReaderFor (file));
        if (reader.get() != nullptr)
        {
            auto nSamps = (int) reader->lengthInSamples;
            auto nChannels = (int) reader->numChannels;
            m_IRSampleRate = reader->sampleRate;
            m_impulseBufferOriginal.setSize( nChannels, nSamps );
            reader->read (&m_impulseBufferOriginal, 0, nSamps, 0, true, true);
            m_samplePath = file.getFullPathName();
            m_sampleName = file.getFileName();
            m_impulseChangedFlag = true;
            m_impulseLoadedFlag = true;
            setImpulseResponse();
        }
    }
    //------------------------------------------------//------------------------------------------------
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
                                        auto nSamps = (int) reader->lengthInSamples;
                                        auto nChannels = (int) reader->numChannels;
                                        m_IRSampleRate = reader->sampleRate;
                                        m_impulseBufferOriginal.setSize( nChannels, nSamps );
                                        reader->read (&m_impulseBufferOriginal, 0, nSamps, 0, true, true);
                                        m_samplePath = file.getFullPathName();
                                        m_sampleName = file.getFileName();
                                        m_impulseChangedFlag = true;
                                        m_impulseLoadedFlag = true;
                                        setImpulseResponse();
                                    }
                                });
    }
    //------------------------------------------------//------------------------------------------------
    void process( juce::AudioBuffer<float> &buffer )
    {
        if ( m_impulseBuffer.getNumChannels() < 1 )
        {
//            DBG("NO IMPULSE");
            return;
        }
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
        
        if ( m_fftFlag )
        {
            juce::dsp::AudioBlock<float> block (buffer);
            juce::dsp::ProcessContextReplacing<float> ctx (block);
            m_conv.process( ctx );
        }
        for ( int c = 0; c < nBuffChannels; c++ )
        {
            buffer.addFrom( c, 0, m_FIRbuffer, c, 0, bufferSize );
        }
        if ( m_filterPosition == filterOutput )
        {
            filterBuffer( buffer );
        }
//        DBG( "latency " << m_conv.getLatency() );
//        DBG( "IR Length Samps " << m_impulseBuffer.getNumSamples()<< " " << m_impulseBufferOriginal.getNumSamples() );
    }
    //------------------------------------------------//------------------------------------------------
    void PANIC()
    {
        for ( int c = 0; c < NUM_CHANNELS; c++ )
        {
            m_FIR[ c ].clearDelays();
        }
        m_conv.reset();
    }
    //------------------------------------------------//------------------------------------------------
    void setPreDelay( int preDelayInSamps )
    {
        for ( int c = 0; c < NUM_CHANNELS; c++ )
        {
            m_preDelay[ c ].setDelayTimeSamps( preDelayInSamps );
        }
    }
    //------------------------------------------------//------------------------------------------------
    void reverseImpulse( bool shouldReverseImpulse )
    {
        if ( m_reverseFlag == shouldReverseImpulse ){ return; }
        m_reverseFlag = shouldReverseImpulse;
        m_impulseChangedFlag = true;
        setImpulseResponse();
    }
    //------------------------------------------------//------------------------------------------------
    bool getReverseState( )
    {
        return m_reverseFlag;
    }
    //------------------------------------------------//------------------------------------------------
    void setStretchFactor( float stretchFactor )
    {
        // need to do this
        if ( stretchFactor <= 0 || stretchFactor == m_stretchFactor ){ return; }
        m_stretchFactor = stretchFactor;
        DBG( "STRETCH " << m_stretchFactor );
        m_impulseChangedFlag = true;
        setImpulseResponse();
    }
    //------------------------------------------------//------------------------------------------------
    float getStretchFactor( )
    {
        DBG( "STRETCH RESTORED " << m_stretchFactor );
        return m_stretchFactor;
    }
    //------------------------------------------------//------------------------------------------------
    void trimImpulseEnd( bool shouldTrimEndOfImpulse )
    {
        m_trimFlag = shouldTrimEndOfImpulse;
        m_impulseChangedFlag = true;
        setImpulseResponse();
    }
   //------------------------------------------------//------------------------------------------------
    void setImpulseStartAndEnd( float start0to1, float end0to1 )
    {
        // normalised to 0-->1
        m_impulseChangedFlag = false;
        if ( start0to1 == m_startPoint && end0to1 == m_endPoint ) { return; }
        m_startPoint = ( start0to1 > 0 ) ? ( ( start0to1 < 1 ) ? start0to1 : 1 ) : 0;
        m_endPoint = ( end0to1 > 0 ) ? ( ( end0to1 < 1 ) ? end0to1 : 1 ) : 0;
        
        DBG("START " << m_startPoint << " END " << m_endPoint);
        m_impulseChangedFlag = true;
        setImpulseResponse();
    }
    //------------------------------------------------//------------------------------------------------
    std::array< float, 2 > getImpulseStartAndEnd()
    {
        std::array< float, 2 > startEnd;
        startEnd[ 0 ] = m_startPoint;
        startEnd[ 1 ] = m_endPoint;
        
        DBG("RESTORED --- START " << m_startPoint << " END " << m_endPoint);
        return startEnd;
    }
    //------------------------------------------------//------------------------------------------------
    void setLPFCutoff( float cutoffCoefficient )
    {
        for ( int c = 0; c < NUM_CHANNELS; c++ )
        {
            m_lpf[ c ].setCutoff( cutoffCoefficient );
        }
        // buffer logic here
        if( m_filterPosition != filterIR )
        {
            return;
        }
        m_impulseChangedFlag = true;
        setImpulseResponse();
    }
    //------------------------------------------------//------------------------------------------------
    float getLPFCutoff( )
    {
        return m_lpf[ 0 ].getCutoff( );
    }
    //------------------------------------------------//------------------------------------------------
    void setHPFCutoff( float cutoffCoefficient )
    {
        for ( int c = 0; c < NUM_CHANNELS; c++ )
        {
            m_hpf[ c ].setCutoff( cutoffCoefficient );
        }
        // buffer logic here
        if( m_filterPosition != filterIR )
        {
            return;
        }
        m_impulseChangedFlag = true;
        setImpulseResponse();
    }
    //------------------------------------------------//------------------------------------------------
    void getHPFCutoff( float cutoffCoefficient )
    {
        return m_hpf[ 0 ].getCutoff( );
    }
    //------------------------------------------------//------------------------------------------------
    void setFilterPosition( int filterPos )
    {
        if ( filterPos < filterOff || filterPos > filterOutput ){ return; }
        if (filterPos != filterIR && m_filterPosition != filterIR )
        {
            m_filterPosition = filterPos;
            return;
        }
        m_filterPosition = filterPos;
        m_impulseChangedFlag = true;
        setImpulseResponse();
    }
    //------------------------------------------------//------------------------------------------------
    void setAmplitudeEnvelope( std::vector< std::array < float, 2 > >& envelope )
    {
        DBG( "Amp env set " );
        if ( m_env.size() < 2 || envelope == m_env ){ return; }
        m_env = envelope;
        m_envelopeFlag = true;
        m_impulseChangedFlag = true;
        setImpulseResponse();
    }
    //------------------------------------------------//------------------------------------------------
    std::vector< std::array < float, 2 > > getAmplitudeEnvelope( )
    {
        return m_env;
    }
    //------------------------------------------------//------------------------------------------------
    juce::AudioBuffer< float >& getIRBuffer()
    {
        return m_impulseBuffer;
    }
    //------------------------------------------------//------------------------------------------------
    double getIRSampleRate()
    {
        return m_IRSampleRate;
    }
    //------------------------------------------------//------------------------------------------------
    enum filterPositions
    {
        filterOff = 1, filterIR, filterOutput
    };
    //------------------------------------------------//------------------------------------------------
private:
    void setImpulseResponse()
    {
        if ( !m_impulseLoadedFlag ){ return; }
        auto nSamps = m_impulseBufferOriginal.getNumSamples( );
        auto nChannels = m_impulseBufferOriginal.getNumChannels( );
        if ( !m_impulseChangedFlag || nChannels < 1 || nSamps < 1 )
        { return; }
        
        // process impulse before setting FIR and FFT
        if ( m_trimFlag ){ trimSilenceAtEndOfBuffer( m_impulseBuffer,  m_impulseBufferOriginal ); }
        else{ m_impulseBuffer = m_impulseBufferOriginal; }
        if ( m_stretchFactor != 1 ){ stretchBuffer( m_impulseBuffer,  m_impulseBufferOriginal ); }
        if ( m_filterPosition == filterIR ) { filterBuffer( m_impulseBuffer ); }
        if ( m_reverseFlag ){ reverseBuffer( m_impulseBuffer ); }
        if ( m_envelopeFlag ){ applyEnvelopeToBuffer( m_impulseBuffer, m_env ); }
        
        nSamps = m_impulseBuffer.getNumSamples();
        
        if ( m_startPoint == 0 && m_endPoint == 1 )
        {
            setFIRandFFT( m_impulseBuffer );
            return;
        }
        
        
        int start = nSamps * m_startPoint;
        int end = nSamps * m_endPoint;
        auto newLen = end - start;
        
        juce::AudioBuffer< float > trimmedIRBuffer{ nChannels, newLen };
        for ( int c = 0; c < nChannels; c++ )
        {
            trimmedIRBuffer.copyFrom( c, 0, m_impulseBuffer, c, start, newLen );
        }
        setFIRandFFT( trimmedIRBuffer );

        m_impulseChangedFlag = false;
    }
    //------------------------------------------------//------------------------------------------------
    void trimSilenceAtEndOfBuffer( juce::AudioBuffer< float >& buffer, juce::AudioBuffer< float >& bufferOriginal )
    {
        auto nSamps = bufferOriginal.getNumSamples();
        auto nChannels  = bufferOriginal.getNumChannels();
        int indx = 0,  indxC = 0;
        for ( int c = 0; c < nChannels; c++ )
        {
            auto rp = bufferOriginal.getReadPointer( c );
            int s = nSamps - 1;
            auto db = 20*log10( abs( rp [ s ] ) ); // not sure if I need this
            while (  (db < -1*80) && s > 0 )
            {
                indxC = s;
                db = 20*log10( abs( rp [ s ] ) );
                s--;
            }
            if ( indxC > indx ){ indx = indxC; }
        }
        if ( indx == 0 ) { return; }
        int newLen = indx + 1; // add one sample for safety
        buffer.setSize( nChannels, newLen );
        for ( int c = 0; c < nChannels; c++ )
        {
            buffer.copyFrom( c, 0, bufferOriginal, c, 0, newLen );
        }
    }
    //------------------------------------------------//------------------------------------------------
    void reverseBuffer( juce::AudioBuffer< float >& buffer )
    {
        auto nSamps = buffer.getNumSamples();
        auto nChannels  = buffer.getNumChannels();
        for ( int c = 0; c < nChannels; c++ )
        {
            buffer.reverse( c, 0, nSamps );
        }
    }
    //------------------------------------------------//------------------------------------------------
    void stretchBuffer( juce::AudioBuffer< float >& buffer, juce::AudioBuffer< float >& bufferOriginal )
    {
        auto nSamps = buffer.getNumSamples();
        auto nChannels  = buffer.getNumChannels();
        auto nSampsOriginal = bufferOriginal.getNumSamples();
        auto nSampsStretched = (int) (nSamps * m_stretchFactor);
        auto stride = 1.0f / m_stretchFactor;
        buffer.setSize( nChannels, nSampsStretched );
        
        for ( int c = 0; c < nChannels; c++ )
        {
            auto rp = bufferOriginal.getReadPointer( c );
            auto wp = buffer.getWritePointer( c );
            
            for ( int s = 0; s < nSampsStretched; s++ )
            {
                wp[ s ] = cubicInterpolateHermite( rp, s*stride, nSampsOriginal );
            }
        }
    }
    //------------------------------------------------//------------------------------------------------
    void filterBuffer( juce::AudioBuffer< float >& buffer )
    {
        auto nSamps = buffer.getNumSamples();
        auto nChannels  = buffer.getNumChannels();
        // filter buffer here
        // havent checked to make sure this works
        for ( int c = 0; c < nChannels; c++ )
        {
            auto ptr = buffer.getWritePointer( c );
            for ( int s = 0; s < nSamps; s++ )
            {
                m_lpf[ c ].filterInPlace( ptr[ s ] );
                m_hpf[ c ].filterInPlaceHP( ptr[ s ] );
            }
        }
    }
    //------------------------------------------------//------------------------------------------------
    void setFIRandFFT( juce::AudioBuffer< float >& buffer )
    {
        auto nSamps = buffer.getNumSamples();
        auto nChannels  = buffer.getNumChannels();
        
        
        for ( int c = 0; c < NUM_CHANNELS; c++ )
        {
            m_FIR[ c ].clear();
            if ( nSamps <= 0 ){ return; }
            m_FIR[ c ].setKernel( buffer.getWritePointer( c % nChannels, 0 ), nSamps );
        }
        
        m_conv.reset();
        if ( nSamps <= FIR_BUFFER_SIZE )
        {
            m_fftFlag = false;
            return;
        }
        
        m_fftFlag = true;
        m_fftImpulseBuffer.setSize( NUM_CHANNELS, nSamps - FIR_BUFFER_SIZE );
        for ( int c = 0; c < NUM_CHANNELS; c++ )
        {
            m_fftImpulseBuffer.copyFrom( c, 0, buffer, c%nChannels, FIR_BUFFER_SIZE, nSamps - FIR_BUFFER_SIZE );
        }
        m_conv.loadImpulseResponse( juce::AudioBuffer< float >(m_fftImpulseBuffer), m_IRSampleRate, juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::no, juce::dsp::Convolution::Normalise::no );
        m_conv.reset();
    }
    //------------------------------------------------//------------------------------------------------
    
    std::array< sjf_delayLine< float >, NUM_CHANNELS > m_preDelay;
    std::array< sjf_buffir< FIR_BUFFER_SIZE >, NUM_CHANNELS > m_FIR;
    
    juce::dsp::ConvolutionMessageQueue m_convBackgroundMessageQueue;
    
    juce::dsp::Convolution m_conv{juce::dsp::Convolution::NonUniform{ FIR_BUFFER_SIZE }, m_convBackgroundMessageQueue };
    
    juce::AudioBuffer< float > m_impulseBuffer, m_impulseBufferOriginal, m_fftImpulseBuffer, m_FIRbuffer;
    
    // buffer flags
    bool m_impulseLoadedFlag = false, m_impulseChangedFlag = false, m_reverseFlag = false, m_trimFlag = false, m_fftFlag = false, m_envelopeFlag = false;
    float m_stretchFactor = 1, m_startPoint = 0, m_endPoint = 1, attack = 0, decay = 0;
    
    int m_filterPosition = 1;
    double m_IRSampleRate = 0;
    juce::String m_samplePath, m_sampleName;
    juce::AudioFormatManager m_formatManager;
    std::unique_ptr<juce::FileChooser> m_chooser;
    std::array< sjf_lpf< float >, NUM_CHANNELS > m_lpf, m_hpf;
    
    std::vector< std::array< float, 2 > > m_env; // normalised amplitude envelope envPoint{ position0to1, amplitude0to1 }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_convo )
};

#endif /* sjf_convo_h */
