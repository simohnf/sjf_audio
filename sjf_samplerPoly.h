//
//  sjf_samplerPoly.h
//
//  Created by Simon Fay on 02/03/2023.
//

#ifndef sjf_samplerPoly_h
#define sjf_samplerPoly_h

#include <vector>
#include "sjf_audioUtilities.h"
#include "sjf_interpolationTypes.h"
#include <time.h>

class sjf_samplerPoly
{
public:
    float m_revProb = 0, m_speedProb = 0, m_subDivProb = 0, m_ampProb = 0, m_stepShuffleProb = 0, m_sampleChoiceProb = 0;
    bool m_canPlayFlag = false, m_randomOnLoopFlag = false, m_syncToHostFlag = false, m_revFlag = false, m_speedFlag = false, m_speedRampFlag = true, m_subDivFlag = false, m_ampFlag = false, m_stepShuffleFlag = false, m_sampleChoiceFlag = false;
    int m_interpolationType = 1;
    int m_voiceNumber = 0;
    
    
protected:
    std::vector< juce::String > m_samplePath, m_sampleName;
    std::vector< float > m_revPat, m_speedPat, m_subDivPat, m_subDivAmpRampPat, m_ampPat;
    std::vector< std::vector< float > > m_stepPat;
    std::vector< float > m_sampleChoicePat;
    std::vector< float > m_durationSamps;
    std::vector< float > m_sliceLenSamps;
    std::vector< float > m_phaseRateMultiplier;
    std::vector< bool > m_sampleLoadedFlag;
    std::vector< float > m_sampleChoiceProbabilities;
    int m_nSteps = 16;
    std::vector< int > m_nSlices;
    
    std::vector< juce::AudioBuffer<float> > m_AudioSample;
    juce::AudioBuffer<float> m_tempBuffer;
    
    juce::AudioFormatManager m_formatManager;
    std::unique_ptr<juce::FileChooser> m_chooser;
    
    float m_fadeInMs = 1;
    
    float m_hostBPM = 120;
    
    int m_SR = 44100;
    int m_lastStep = -1;
    
    
    float m_readPos = 0;
    int m_stepCount = 0;
    
public:
    sjf_samplerPoly()
    {
        setNumVoices( 1 );
        for ( int voiceNumber = 0; voiceNumber < m_AudioSample.size(); voiceNumber++ )
        {
            m_AudioSample[ voiceNumber ].clear();
        }
        m_formatManager.registerBasicFormats();
        setPatterns();
    };
    
    ~sjf_samplerPoly(){};
    
    void initialise(int sampleRate)
    {
        m_SR  = sampleRate;
        srand((unsigned)time(NULL));
    }
    
    void setNumVoices( const int& nVoices )
    {
        if( m_AudioSample.size() == nVoices ){ return; }
        m_AudioSample.resize( nVoices );
        m_samplePath.resize( nVoices );
        m_sampleName.resize( nVoices );
        m_stepPat.resize( nVoices );
        m_sliceLenSamps.resize( nVoices );
        m_durationSamps.resize( nVoices );
        
//        m_sampleLoadedFlag.resize( nVoices );
//        for ( int v = 0; v < m_sampleLoadedFlag.size(); v++ )
//        {
//            m_sampleLoadedFlag[ v ] = false;
//        }
//
        if ( nVoices > m_nSlices.size() )
        {
            while ( nVoices > m_nSlices.size() )
            {
                m_nSlices.push_back( 16 );
                m_phaseRateMultiplier.push_back( 1 );
                m_sampleLoadedFlag.push_back( false );
                m_sampleChoiceProbabilities.push_back( 0.0f );
            }
        }
        else
        {
            m_nSlices.resize( nVoices );
            m_phaseRateMultiplier.resize( nVoices );
            m_sampleLoadedFlag.resize( nVoices );
            m_sampleChoiceProbabilities.resize( nVoices );
        }
    }
    
    const int getNumVoices()
    {
        return m_AudioSample.size();
    }
    //==============================================================================
    float getDuration( const int& voiceNumber ) { return m_durationSamps[ voiceNumber ]; };
    //==============================================================================
    float getDurationMS( const int& voiceNumber ) { return (float)m_durationSamps[ voiceNumber ] * 1000.0f / (float)m_SR; };
    //==============================================================================
    bool loadSample( const int& voiceNumber )
    {
        m_chooser = std::make_unique<juce::FileChooser> ("Select a Wave/Aiff file to play..." ,
                                                         juce::File{}, "*.aif, *.wav");
        auto chooserFlags = juce::FileBrowserComponent::openMode
        | juce::FileBrowserComponent::canSelectFiles;
        
        m_chooser->launchAsync (chooserFlags, [this, voiceNumber] (const juce::FileChooser& fc)
                                {
                                    auto file = fc.getResult();
                                    if (file == juce::File{}) { return false; }
                                    std::unique_ptr<juce::AudioFormatReader> reader (m_formatManager.createReaderFor (file));
                                    if (reader.get() != nullptr)
                                    {
                                        bool lastPlayState = m_canPlayFlag;
                                        m_canPlayFlag = false;
                                        m_tempBuffer.clear();
                                        m_tempBuffer.setSize((int) reader->numChannels, (int) reader->lengthInSamples);
                                        m_durationSamps[ voiceNumber ] = m_tempBuffer.getNumSamples();
                                        reader->read (&m_tempBuffer, 0, (int) reader->lengthInSamples, 0, true, true);
                                        //                                          m_AudioSample.clear();
                                        m_AudioSample[ voiceNumber ].makeCopyOf(m_tempBuffer);
                                        m_sliceLenSamps[ voiceNumber ] = m_durationSamps[ voiceNumber ]/(float)m_nSlices[ voiceNumber ];
                                        //                                          setPatterns();
                                        m_samplePath[ voiceNumber ] = file.getFullPathName();
                                        m_sampleName[ voiceNumber ] = file.getFileName();
                                        m_tempBuffer.setSize(0, 0);
                                        m_sampleLoadedFlag[ voiceNumber ] = true;
                                        m_canPlayFlag = lastPlayState;
                                    }
                                    else { return false; }
                                });
        return true;
    };
    //==============================================================================
    bool loadSample(juce::Value path, const int& voiceNumber)
    {
        juce::File file( path.getValue().toString() );
        if (file == juce::File{}) { return false; }
        std::unique_ptr<juce::AudioFormatReader> reader (m_formatManager.createReaderFor (file));
        if (reader.get() != nullptr)
        {
            m_AudioSample[ voiceNumber ].clear();
            m_AudioSample[ voiceNumber ].setSize((int) reader->numChannels, (int) reader->lengthInSamples);
            m_durationSamps[ voiceNumber ] = m_AudioSample[ voiceNumber ].getNumSamples();
            reader->read (&m_AudioSample[ voiceNumber ], 0, (int) reader->lengthInSamples, 0, true, true);
            m_sliceLenSamps[ voiceNumber ] = m_durationSamps[ voiceNumber ]/m_nSlices[ voiceNumber ];
            setPatterns();
            m_samplePath[ voiceNumber ] = file.getFullPathName();
            m_sampleName[ voiceNumber ] = file.getFileName();
            m_sampleLoadedFlag[ voiceNumber ] = true;
        }
        else{ return false; }
        return true;
    };
    //==============================================================================
    const juce::String getFilePath( const int voiceNumber )
    {
        return m_samplePath[ voiceNumber ];
    }
    //==============================================================================
    const juce::String getFileName( const int voiceNumber )
    {
        return m_sampleName[ voiceNumber ];
    }
    //==============================================================================
    void setFadeLenMs(float fade)
    {
        m_fadeInMs = fade;
    };
    //==============================================================================
    float getFadeInMs()
    {
        return m_fadeInMs;
    }
    //==============================================================================
    void setNumSlices( const int slices, const int voiceNumber )
    {
        if(slices <= 0){ return; }
        DBG("Change Num Slices " << slices << " voice " << voiceNumber );
        m_nSlices[ voiceNumber ] = slices;
        m_sliceLenSamps[ voiceNumber ] = m_durationSamps[ voiceNumber ]/m_nSlices[ voiceNumber ];
        setPatterns();
    }
    //==============================================================================
    int getNumSlices( const int voiceNumber )
    {
        DBG( "Get Num Slices " << voiceNumber );
        return m_nSlices[ voiceNumber ];
    }
    //==============================================================================
    void setNumSteps( int steps )
    {
        m_revPat.resize(steps);
        m_speedPat.resize(steps);
        m_subDivPat.resize(steps);
        m_subDivAmpRampPat.resize(steps);
        m_ampPat.resize(steps);
        m_sampleChoicePat.resize( steps );
        for ( int voiceNumber = 0; voiceNumber < m_nSlices.size(); voiceNumber++ )
        {
            m_stepPat[ voiceNumber ].resize(steps);
        }
        if (steps > m_nSteps)
        {
            for (int index = m_nSteps; index < steps; index++)
            {
                m_revPat[index] = 0;
                m_speedPat[index] = 0;
                m_subDivPat[index] = 0;
                m_subDivAmpRampPat[index] = rand01();
                m_ampPat[index] = 1.0f;
                m_sampleChoicePat[ index ] = 0;
                for ( int voiceNumber = 0; voiceNumber < m_nSlices.size(); voiceNumber++ )
                {
                    m_stepPat[ voiceNumber ][ index ] = index % m_nSlices[ voiceNumber ];
                }
            }
        }
        m_nSteps = steps;
    };
    //==============================================================================
    int getNumSteps()
    {
        return m_nSteps;
    }
    //==============================================================================
    void setPhaseRateMultiplierIndex( const int& i, const int& voiceNumber )
    {
        m_phaseRateMultiplier[ voiceNumber ] = pow(2, i-3);
    }
    //==============================================================================
    int getPhaseRateMultiplierIndex( const int& voiceNumber )
    {
        return 3 + log10(m_phaseRateMultiplier[ voiceNumber ])/log10(2);
    }
    //==============================================================================
    void play(juce::AudioBuffer<float> &buffer)
    {
        if (!m_sampleLoadedFlag[ m_voiceNumber ])
        {
            bool samples = false;
            for ( int v = 0; v < m_sampleLoadedFlag.size(); v++ )
            {
                if ( m_sampleLoadedFlag[ v ] )
                {
                    m_voiceNumber = v;
                    samples = true;
                }
            }
            if ( !samples ){ return; }
        }

        auto bufferSize = buffer.getNumSamples();
        auto numChannels = buffer.getNumChannels();
        
        auto increment = m_phaseRateMultiplier[ m_voiceNumber ];
        auto envLen =(int)( m_phaseRateMultiplier[ m_voiceNumber ] * m_fadeInMs * m_SR / 1000 ) + 1;
        
        for (int index = 0; index < bufferSize; index++)
        {
            if ( checkForChangeOfBeat( m_stepCount ) )
            {
//                m_voiceNumber = calculateSampleChoice( m_stepCount );
                m_voiceNumber = m_sampleChoicePat[ m_stepCount ];
                if (!m_sampleLoadedFlag[ m_voiceNumber ])
                {
                    bool samples = false;
                    for ( int v = 0; v < m_sampleLoadedFlag.size(); v++ )
                    {
                        if ( m_sampleLoadedFlag[ v ] )
                        {
                            m_voiceNumber = v;
                            samples = true;
                        }
                    }
                    if ( !samples ){ return; }
                }
                envLen = (int)( m_phaseRateMultiplier[ m_voiceNumber ] * m_fadeInMs * m_SR / 1000 ) + 1;
                increment = m_phaseRateMultiplier[ m_voiceNumber ];
            }
            
//            auto pos = calculateMangledReadPosition( m_readPos, 1, m_stepCount, m_voiceNumber );
            auto pos = m_readPos;
            auto subDiv = floor(m_subDivPat[m_stepCount] * 8.0f) + 1.0f ;
            auto subDivLenSamps = m_sliceLenSamps[ m_voiceNumber ] / subDiv ;
            auto subDivAmp = calculateSubDivAmp( m_stepCount, subDiv, int(pos / subDivLenSamps) );
            auto speedVal = calculateSpeedVal( m_stepCount, (m_readPos / m_sliceLenSamps[ m_voiceNumber ]) );
            while (pos >= subDivLenSamps){ pos -= subDivLenSamps; }
            auto env = envelope( pos , (int)subDivLenSamps - 1, envLen ); // Check this out more
            auto amp = calculateAmpValue( m_stepCount );
            pos = calculateReverse( m_stepCount, pos, subDivLenSamps );
            pos *= speedVal;
            pos += m_stepPat[ m_voiceNumber ][ m_stepCount ] * m_sliceLenSamps[ m_voiceNumber ];
            
            for (int channel = 0; channel < numChannels; channel ++)
            {
                auto val = calculateSampleValue( m_AudioSample[ m_voiceNumber ], channel % m_AudioSample[ m_voiceNumber ].getNumChannels(), pos );
                val *= subDivAmp * amp;
                val *= env;
                buffer.setSample(channel, index, val);
            }
            m_readPos += increment;
            if (m_readPos >= m_sliceLenSamps[ m_voiceNumber ]) {m_stepCount++; m_stepCount %= m_nSteps; }
            while (m_readPos >= m_sliceLenSamps[ m_voiceNumber ]){ m_readPos -= m_sliceLenSamps[ m_voiceNumber ]; }
        }
    };
    //==============================================================================
    void play(juce::AudioBuffer<float> &buffer, float bpm, double hostPosition)
    {
//        DBG( "host position " << hostPosition );
        if ( !m_sampleLoadedFlag[ m_voiceNumber ] )
        {
            bool samples = false;
            for ( int v = 0; v < m_sampleLoadedFlag.size(); v++ )
            {
                if ( m_sampleLoadedFlag[ v ] )
                {
                    m_voiceNumber = v;
                    samples = true;
                }
            }
            if ( !samples ){ return; }
        }
//        hostPosition *= 2;
//        fastMod3< double > ( hostPosition, m_nSteps );
//        hostPosition *= 0.5;
        auto bufferSize = buffer.getNumSamples();
        auto numChannels = buffer.getNumChannels();
        
        auto envLen = floor(m_fadeInMs * m_SR / 1000) + 1;
        auto hostSampQuarter = 60.0f*m_SR/bpm;
        
        auto hostSyncCompenstation =  calculateHostCompensation( bpm, m_voiceNumber );
        auto increment = m_phaseRateMultiplier[ m_voiceNumber ] * hostSyncCompenstation;
        auto hostPos = hostPosition * hostSampQuarter * m_phaseRateMultiplier[ m_voiceNumber ] * hostSyncCompenstation;
        m_stepCount = (int)( hostPos / m_sliceLenSamps[ m_voiceNumber ] );
        
        m_readPos = hostPos - m_stepCount*m_sliceLenSamps[ m_voiceNumber ];
        fastMod3< float > ( m_readPos, m_sliceLenSamps[ m_voiceNumber ] );
        m_stepCount %= m_nSteps;

        for (int index = 0; index < bufferSize; index++)
        {
            if ( checkForChangeOfBeat( m_stepCount ) )
            {
//                m_voiceNumber = calculateSampleChoice( m_stepCount );
                m_voiceNumber = m_sampleChoicePat[ m_stepCount ];
                
                DBG( "voice " << m_voiceNumber );
                if ( !m_sampleLoadedFlag[ m_voiceNumber ] )
                {
                    bool samples = false;
                    for ( int v = 0; v < m_sampleLoadedFlag.size(); v++ )
                    {
                        if ( m_sampleLoadedFlag[ v ] )
                        {
                            m_voiceNumber = v;
                            samples = true;
                        }
                    }
                    if ( !samples ){ return; }
                }
                
                hostSyncCompenstation =  calculateHostCompensation( bpm, m_voiceNumber );
                increment = m_phaseRateMultiplier[ m_voiceNumber ] * hostSyncCompenstation;
                hostPos = (hostPosition * hostSampQuarter * m_phaseRateMultiplier[ m_voiceNumber ] * hostSyncCompenstation) + index;
                m_readPos = hostPos - m_stepCount*m_sliceLenSamps[ m_voiceNumber ];
                fastMod3< float > ( m_readPos, m_sliceLenSamps[ m_voiceNumber ] );
            }



            auto pos = m_readPos;
            auto subDiv = floor( m_subDivPat[ m_stepCount ] * 8.0f ) + 1.0f ;
            auto subDivLenSamps = m_sliceLenSamps[ m_voiceNumber ] / subDiv ;
            auto subDivAmp = calculateSubDivAmp( m_stepCount, subDiv, int(pos / subDivLenSamps) );
            auto speedVal = calculateSpeedVal( m_stepCount, ( m_readPos / m_sliceLenSamps[ m_voiceNumber ] ) );
            fastMod3< float >( pos, subDivLenSamps ); //while ( pos >= subDivLenSamps ){ pos -= subDivLenSamps; }
            auto env = envelope( floor( pos / increment ) , floor( subDivLenSamps / increment ), envLen ); // Check this out more
            auto amp = calculateAmpValue( m_stepCount );
            pos = calculateReverse( m_stepCount, pos, subDivLenSamps );
            pos *= speedVal;
            pos += m_stepPat[ m_voiceNumber ][ m_stepCount ] * m_sliceLenSamps[ m_voiceNumber ];
            
            for (int channel = 0; channel < numChannels; channel ++)
            {
                auto val = calculateSampleValue( m_AudioSample[ m_voiceNumber ], channel % m_AudioSample[ m_voiceNumber ].getNumChannels(), pos );
                val *= subDivAmp * amp;
                val *= env;
                buffer.setSample(channel, index, val);
            }
            m_readPos +=  increment;
            if ( m_readPos >= m_sliceLenSamps[ m_voiceNumber ] )
            {
                m_stepCount++;
                m_stepCount %= m_nSteps;
                fastMod3< float > ( m_readPos, m_sliceLenSamps[ m_voiceNumber ] );
            }
//            while (m_readPos >= m_sliceLenSamps[ m_voiceNumber ])
//            { m_readPos -= m_sliceLenSamps[ m_voiceNumber ]; }
            
        }
    }
    //==============================================================================
    void setSampleChoiceProbabilities( const std::vector< float >& probs )
    {
        m_sampleChoiceProbabilities = probs;
    }
    //==============================================================================
    void setSampleChoiceProbabilities( const float& prob, const int& voiceNumber )
    {
        m_sampleChoiceProbabilities[ voiceNumber ] = prob;
    }
    //==============================================================================
    float getSampleChoiceProbability( const int& voiceNumber )
    {
        return m_sampleChoiceProbabilities[ voiceNumber ];
    }
    //==============================================================================
    void randomiseAll()
    {
        m_revFlag = true;
        m_speedFlag = true;
        m_subDivFlag = true;
        m_ampFlag = true;
        m_stepShuffleFlag = true;
        m_sampleChoiceFlag = true;
    };
    //==============================================================================
private:
//    float calculateMangledReadPosition( float pos, const float& increment, const int& step, const int& voiceNumber )
//    {
//        auto subDiv = floor(m_subDivPat[step] * 8.0f) + 1.0f ;
//        auto subDivLenSamps = m_sliceLenSamps[ voiceNumber ] / subDiv ;
//        auto subDivAmp = calculateSubDivAmp( step, subDiv, int(pos / subDivLenSamps) );
//        auto speedVal = calculateSpeedVal( step, ( pos / m_sliceLenSamps[ voiceNumber ]) );
//        while (pos >= subDivLenSamps){ pos -= subDivLenSamps; }
//        auto env = envelope( floor( pos / increment) , floor(subDivLenSamps / increment), envLen ); // Check this out more
//        auto amp = calculateAmpValue( step );
//        pos = calculateReverse( step, pos, subDivLenSamps);
//        pos *= speedVal;
//        pos += m_stepPat[ voiceNumber ][step] * m_sliceLenSamps[ voiceNumber ];
//        return pos;
//
//    }
    //==============================================================================
    bool checkForChangeOfBeat( int currentStep )
    {
//        DBG( "last " << m_lastStep << " current " << currentStep );
        if ( currentStep == m_lastStep )
        {
            return false;
        }
        else
        {   // This is just to randomise patterns at the start of each loop
            if (currentStep == 0 && m_randomOnLoopFlag) { randomiseAll(); }
            checkPatternRandomisationFlags();
            m_lastStep = currentStep;
            return true;
        }
    };
    //==============================================================================
    float envelope( float pos, float period, float envLen)
    {
        if (pos < 0){ pos = 0; }
        else if (pos > period ){ pos = period; }
        auto outVal = 1.0f;
        auto sampsAtEnd = period - envLen;
        if (pos < envLen) { outVal = pos/envLen; }
        else if(pos > sampsAtEnd)
        {
            outVal = 1 - (pos - sampsAtEnd)/envLen;
        }
        //        return outVal; // this would give linear fade
        return sin( PI* (outVal)/2 ); // this gives a smooth sinewave based fade
    };
    //==============================================================================
    float calculateReverse(int currentStep, float pos, float subDivLenSamps)
    {
        //            REVERSE LOGIC
        if ( m_revPat[currentStep] >= 0.25 ){ pos =  (subDivLenSamps - pos) ; }
        return pos;
    };
    //==============================================================================
    float calculateSubDivAmp( int currentStep, float subDiv, float subDivCount )
    {
        auto subDivAmp = 1.0f;
        
        // if subDiv is one, subDiv amp is 1
        if (subDiv == 1)
        {
            subDivAmp = 1.0f;
        }
        // else if subDivPatAmp for current
        else if ( m_subDivAmpRampPat[currentStep] < 0.5 )
        {
            // start subdivision at max amplitude, drop to a fraction of that
            subDivAmp = 1.0f / (subDivCount + 1.0f);
        }
        else
        {
            // start subdivision at a fraction of max, raise to maximum for last division
            subDivAmp = 1.0f / (subDiv - subDivCount);
        }
        return subDivAmp;
    };
    //==============================================================================
    float calculateAmpValue(int currentStep) { return m_ampPat[currentStep]; };
    //==============================================================================
    float calculateSpeedVal( int currentStep, float stepPhase )
    {
        float speedVal = 0;
        if (!m_speedRampFlag) { speedVal = m_speedPat[currentStep] ; }
        else { speedVal = 0 + (m_speedPat[currentStep] * stepPhase) ; }
        speedVal = pow(2, (speedVal*12)/12);
        return speedVal;
    }
    //==============================================================================
    int calculateSampleChoice( const int& currentStep )
    {
        if( m_sampleChoicePat[ currentStep ] >= m_sampleChoiceProb/100.0f ) { return 0; }
        else { return 1; }
//        return round( m_sampleChoicePat[ currentStep ] * ( m_AudioSample.size() - 1 ) );
    }
    //==============================================================================
    float calculateSampleValue(juce::AudioBuffer<float>& buffer, int channel, float pos)
    {
        if (m_interpolationType < 1) { m_interpolationType = 1; }
        else if (m_interpolationType > 6) { m_interpolationType = 6; }
        switch(m_interpolationType)
        {
            case 1:
                return linearInterpolate(buffer, channel, pos);
                break;
            case 2:
                return cubicInterpolate(buffer, channel, pos);
                break;
            case 3:
                return fourPointInterpolatePD(buffer, channel, pos);
                break;
            case 4:
                return fourPointFourthOrderOptimal(buffer, channel, pos);
                break;
            case 5:
                return cubicInterpolateGodot(buffer, channel, pos);
                break;
            case 6:
                return cubicInterpolateHermite(buffer, channel, pos);
                break;
        }
        return linearInterpolate(buffer, channel, pos);
    };
    //==============================================================================
    float calculateHostCompensation( const float& bpm, const int& voiceNumber ){
        if (!m_sampleLoadedFlag[ voiceNumber ] ) { return 1; }
        if (!m_syncToHostFlag) { return 1; }
        
        m_hostBPM = bpm;
        auto hostQuarterNoteSamps = m_SR*60.0f/bpm;
        auto multiplier = 1.0f;
        if ( m_sliceLenSamps[ voiceNumber ] == hostQuarterNoteSamps ) { return 1; }
        else if ( m_sliceLenSamps[ voiceNumber ] < hostQuarterNoteSamps )
        {
            while ( m_sliceLenSamps[ voiceNumber ] * multiplier < hostQuarterNoteSamps ){ multiplier *= 2; }
            auto lastDiff = abs( hostQuarterNoteSamps - m_sliceLenSamps[ voiceNumber ]*multiplier );
            auto halfMultiplier = multiplier * 0.5;
            auto newDiff = abs( hostQuarterNoteSamps - m_sliceLenSamps[ voiceNumber ]*halfMultiplier );
            if ( newDiff < lastDiff ) { multiplier = halfMultiplier; }
        }
        else
        {
            while ( m_sliceLenSamps[ voiceNumber ] > hostQuarterNoteSamps *multiplier ){ multiplier *= 2; }
            float lastDiff = abs( hostQuarterNoteSamps*multiplier - m_sliceLenSamps[ voiceNumber ] );
            auto halfMultiplier = multiplier * 0.5f;
            auto newDiff = abs( hostQuarterNoteSamps*halfMultiplier - m_sliceLenSamps[ voiceNumber ] );
            if ( newDiff < lastDiff ) { multiplier = halfMultiplier; }
            multiplier = 1/multiplier;
        }
        return ( m_sliceLenSamps[ voiceNumber ]*multiplier ) / hostQuarterNoteSamps ;
        
//        if (!m_sampleLoadedFlag[ voiceNumber ] ) { return 1; }
//        if (!m_syncToHostFlag) { return 1; }
//        
//        m_hostBPM = bpm;
//        const auto hostQuarterNoteSamps = m_SR*60.0f/bpm;
//        auto multiplier = 1.0f;
//        auto v0Compensation = 1.0f;
//        // first calculate compensation for first Voice
//        const auto v0SliceLen = m_sliceLenSamps[ 0 ];
//        if ( v0SliceLen == hostQuarterNoteSamps ) { v0Compensation = 1; }
//        else if ( v0SliceLen < hostQuarterNoteSamps )
//        {
//            while ( v0SliceLen * multiplier < hostQuarterNoteSamps ){ multiplier *= 2; }
//            auto lastDiff = abs( hostQuarterNoteSamps - v0SliceLen*multiplier );
//            auto halfMultiplier = multiplier * 0.5;
//            auto newDiff = abs( hostQuarterNoteSamps - v0SliceLen*halfMultiplier );
//            if ( newDiff < lastDiff ) { multiplier = halfMultiplier; }
//        }
//        else
//        {
//            while ( v0SliceLen > hostQuarterNoteSamps *multiplier ){ multiplier *= 2; }
//            float lastDiff = abs( hostQuarterNoteSamps*multiplier - v0SliceLen );
//            auto halfMultiplier = multiplier * 0.5f;
//            auto newDiff = abs( hostQuarterNoteSamps*halfMultiplier - v0SliceLen );
//            if ( newDiff < lastDiff ) { multiplier = halfMultiplier; }
//            multiplier = 1/multiplier;
//        }
//        v0Compensation = ( v0SliceLen*multiplier ) / hostQuarterNoteSamps;
//        if ( voiceNumber == 0 ){ return v0Compensation; }
//        
//        // otherwise calculate voice compensation relative to first voice
//        multiplier = 1.0f;
//        const vSliceLen = m_sliceLenSamps[ voiceNumber ];
//        if ( vSliceLen == hostQuarterNoteSamps ) { return 1; }
//        else if ( vSliceLen < hostQuarterNoteSamps )
//        {
//            while ( vSliceLen * multiplier < hostQuarterNoteSamps ){ multiplier *= 2; }
//            auto lastDiff = abs( hostQuarterNoteSamps - vSliceLen*multiplier );
//            auto halfMultiplier = multiplier * 0.5;
//            auto newDiff = abs( hostQuarterNoteSamps - vSliceLen*halfMultiplier );
//            if ( newDiff < lastDiff ) { multiplier = halfMultiplier; }
//        }
//        else
//        {
//            while ( vSliceLen > hostQuarterNoteSamps *multiplier ){ multiplier *= 2; }
//            float lastDiff = abs( hostQuarterNoteSamps*multiplier - vSliceLen );
//            auto halfMultiplier = multiplier * 0.5f;
//            auto newDiff = abs( hostQuarterNoteSamps*halfMultiplier - vSliceLen );
//            if ( newDiff < lastDiff ) { multiplier = halfMultiplier; }
//            multiplier = 1/multiplier;
//        }
//        return ( vSliceLen*multiplier ) / hostQuarterNoteSamps;
    };
    
    //==============================================================================
    void setPatterns( )
    {
        DBG( "Setting Patterns. nSteps " << m_nSteps << " nVoices " << m_stepPat.size() );
        m_revPat.resize(m_nSteps);
        m_speedPat.resize(m_nSteps);
        m_subDivPat.resize(m_nSteps);
        m_subDivAmpRampPat.resize(m_nSteps);
        m_ampPat.resize(m_nSteps);
        for ( int voiceNumber = 0; voiceNumber < m_nSlices.size(); voiceNumber++ )
        {
            m_stepPat[ voiceNumber ].resize(m_nSteps);
        }
        for (int index = 0; index < m_nSteps; index++)
        {
            m_revPat[index] = 0;
            m_speedPat[index] = 0;
            m_subDivPat[index] = 0;
            m_subDivAmpRampPat[index] = rand01();
            m_ampPat[index] = 1.0f;
            for ( int voiceNumber = 0; voiceNumber < m_stepPat.size(); voiceNumber++ )
            {
                m_stepPat[ voiceNumber ][index] = index % m_nSlices[ voiceNumber ];
            }
            
        }
    };
    //==============================================================================
    void checkPatternRandomisationFlags(){
        if(m_revFlag){ setRandRevPat(); m_revFlag = false;}
        if(m_speedFlag) { setRandSpeedPat(); m_speedFlag = false;}
        if(m_subDivFlag) { setRandSubDivPat(); m_subDivFlag = false;}
        if(m_ampFlag) { setRandAmpPat(); m_ampFlag = false;}
        if(m_stepShuffleFlag) { setRandStepPat(); m_stepShuffleFlag = false;}
        if(m_sampleChoiceFlag) { setRandomVoicePattern(); m_sampleChoiceFlag = false;}
    };
    //==============================================================================
    void setRandRevPat()
    {
        setRandPat(m_revPat, m_revProb/100.0f);
    };
    //==============================================================================
    void setRandSpeedPat()
    {
        setRandPat(m_speedPat, m_speedProb/100.0f);
        
        for (int index = 0; index < m_nSteps; index++)
        {
            if (rand01() >=0.5) { m_speedPat[index]  *= -1.0f; }
        }
        
    };
    //==============================================================================
    void setRandSubDivPat()
    {
        setRandPat(m_subDivPat, m_subDivProb/100.0f);
        for (int index = 0; index < m_nSteps; index++)
        {
            m_subDivAmpRampPat[index] = rand01();
        }
    };
    //==============================================================================
    void setRandAmpPat()
    {
        setRandPat(m_ampPat, 1 - pow(m_ampProb/100.0f, 4));
    };
    //==============================================================================
    void setRandStepPat()
    {
        for ( int voiceNumber = 0; voiceNumber < m_stepPat.size(); voiceNumber++ )
        {
            m_stepPat[ voiceNumber ].resize(m_nSteps);
            for (int index = 0; index < m_nSteps; index++)
            {
                if(  m_stepShuffleProb/100.0f <= rand01() )
                {
                    m_stepPat[ voiceNumber ][index] = index;
                }
                else
                {
                    int step = floor(rand01() * m_nSteps);
                    m_stepPat[ voiceNumber ][index] = step;
                }
            }
        }

    };
    //==============================================================================
    void setRandomVoicePattern()
    {
        m_sampleChoicePat.resize(m_nSteps);
        float maxProb = *std::max_element( m_sampleChoiceProbabilities.begin(), m_sampleChoiceProbabilities.end() );
        if (  maxProb <= 0 )
        {
            for (int index = 0; index < m_nSteps; index++)
            {
                m_sampleChoicePat[index] = 0;
            }
        }
        else
        {
            float sum = 0.0f;
            for ( auto& n : m_sampleChoiceProbabilities )
                sum += n;
            for (int index = 0; index < m_nSteps; index++)
            {
//                auto sum2 = sum;
                auto rand = rand01() * sum;
                for ( int v = 0; v < m_sampleChoiceProbabilities.size(); v++ )
                {
                    if ( rand <= m_sampleChoiceProbabilities[ v ] )
                    {
                        m_sampleChoicePat[index] = v;
                        break;
                    }
                    else
                    {
                        rand -= m_sampleChoiceProbabilities[ v ];
                    }
                }
//                m_sampleChoicePat[index] = 0;
            }
        }
//        for (int index = 0; index < m_nSteps; index++)
//        {
//            if ( m_sampleChoiceProb == 0 ) { m_sampleChoicePat[index] = 0; }
//            else { m_sampleChoicePat[index] = rand01(); }
//        }
//        setRandPat( m_sampleChoicePat, m_sampleChoiceProb/100.0f );
    }
    //==============================================================================
    void setRandPat(std::vector<float>& pattern, float prob)
    {
        pattern.resize(m_nSteps);
        auto exponent = (1 - prob) * 20;
        for (int index = 0; index < m_nSteps; index++)
        {
            if (prob == 0) {pattern[index] = 0;}
            else {pattern[index] = pow( rand01(), exponent );}
        }
    };
    

    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (sjf_samplerPoly)
    //    END OF sjf_samplerPoly CLASS
    //==============================================================================
};
#endif /* sjf_samplerPoly_h */

