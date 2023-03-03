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

class sjf_samplerPoly{
public:
    float m_revProb = 0, m_speedProb = 0, m_subDivProb = 0, m_ampProb = 0, m_stepShuffleProb = 0, m_sampleChoiceProb = 0;
    bool m_canPlayFlag = false; bool m_randomOnLoopFlag = false; bool m_syncToHostFlag = false;
    bool m_revFlag = false; bool m_speedFlag = false; bool m_speedRampFlag = true;
    bool m_subDivFlag = false, m_ampFlag = false, m_stepShuffleFlag = false, m_sampleChoiceFlag = false;
    int m_interpolationType = 1;
    
    
    
protected:
    std::vector< juce::String > m_samplePath, m_sampleName;
    std::vector< float > m_revPat, m_speedPat, m_subDivPat, m_subDivAmpRampPat, m_ampPat;
    std::vector< std::vector< float > > m_stepPat;
    std::vector< float > m_sampleChoicePat;
    
    
    int m_nSteps = 16;
    std::vector< int > m_nSlices;
    
    std::vector< juce::AudioBuffer<float> > m_AudioSample;
    juce::AudioBuffer<float> m_tempBuffer;
    
    juce::AudioFormatManager m_formatManager;
    std::unique_ptr<juce::FileChooser> m_chooser;
    
    float m_fadeInMs = 1;
    float m_durationSamps = 44100;
    float m_hostBPM = 120;
    std::vector< float > m_sliceLenSamps;
    int m_SR = 44100;
    int m_lastStep = -1;
    float m_phaseRateMultiplier = 1;
    bool m_sampleLoadedFlag = false;
    float m_readPos = 0;
    int m_stepCount = 0;
    
public:
    sjf_samplerPoly()
    {
        for ( int sampleNumber = 0; sampleNumber < m_AudioSample.size(); sampleNumber++ )
        {
            m_AudioSample[ sampleNumber ].clear();
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
    
    void setNumVoices( const int nVoices )
    {
        m_AudioSample.resize( nVoices );
        m_samplePath.resize( nVoices );
        m_sampleName.resize( nVoices );
        m_stepPat.resize( nVoices );
        m_nSlices.resize( nVoices );
    }
    
    const int getNumVoices()
    {
        return m_AudioSample.size();
    }
    //==============================================================================
    float getDuration() { return m_durationSamps; };
    //==============================================================================
    float getDurationMS() { return (float)m_durationSamps * 1000.0f / (float)m_SR; };
    //==============================================================================
    void loadSample( int sampleNumber )
    {
        m_chooser = std::make_unique<juce::FileChooser> ("Select a Wave/Aiff file to play..." ,
                                                         juce::File{}, "*.aif, *.wav");
        auto chooserFlags = juce::FileBrowserComponent::openMode
        | juce::FileBrowserComponent::canSelectFiles;
        
        m_chooser->launchAsync (chooserFlags, [this, sampleNumber] (const juce::FileChooser& fc)
                                {
                                    auto file = fc.getResult();
                                    if (file == juce::File{}) { return; }
                                    std::unique_ptr<juce::AudioFormatReader> reader (m_formatManager.createReaderFor (file));
                                    if (reader.get() != nullptr)
                                    {
                                        bool lastPlayState = m_canPlayFlag;
                                        m_canPlayFlag = false;
                                        m_tempBuffer.clear();
                                        m_tempBuffer.setSize((int) reader->numChannels, (int) reader->lengthInSamples);
                                        m_durationSamps = m_tempBuffer.getNumSamples();
                                        reader->read (&m_tempBuffer, 0, (int) reader->lengthInSamples, 0, true, true);
                                        //                                          m_AudioSample.clear();
                                        m_AudioSample[ sampleNumber ].makeCopyOf(m_tempBuffer);
                                        m_sliceLenSamps[ sampleNumber ] = m_durationSamps/(float)m_nSlices[ sampleNumber ];
                                        //                                          setPatterns();
                                        m_samplePath[ sampleNumber ] = file.getFullPathName();
                                        m_sampleName[ sampleNumber ] = file.getFileName();
                                        m_tempBuffer.setSize(0, 0);
                                        m_sampleLoadedFlag = true;
                                        m_canPlayFlag = lastPlayState;
                                    }
                                });
    };
    //==============================================================================
    void loadSample(juce::Value path, int sampleNumber)
    {
        juce::File file( path.getValue().toString() );
        if (file == juce::File{}) { return; }
        std::unique_ptr<juce::AudioFormatReader> reader (m_formatManager.createReaderFor (file));
        if (reader.get() != nullptr)
        {
            m_AudioSample[ sampleNumber ].clear();
            m_AudioSample[ sampleNumber ].setSize((int) reader->numChannels, (int) reader->lengthInSamples);
            m_durationSamps = m_AudioSample[ sampleNumber ].getNumSamples();
            reader->read (&m_AudioSample[ sampleNumber ], 0, (int) reader->lengthInSamples, 0, true, true);
            m_sliceLenSamps[ sampleNumber ] = m_durationSamps/m_nSlices[ sampleNumber ];
            setPatterns();
            m_samplePath[ sampleNumber ] = file.getFullPathName();
            m_sampleName[ sampleNumber ] = file.getFileName();
            m_sampleLoadedFlag = true;
        }
    };
    //==============================================================================
    const juce::String getFilePath( const int sampleNumber )
    {
        return m_samplePath[ sampleNumber ];
    }
    //==============================================================================
    const juce::String getFileName( const int sampleNumber )
    {
        return m_sampleName[ sampleNumber ];
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
    void setNumSlices( const int slices, const int sampleNumber )
    {
        m_nSlices[ sampleNumber ] = slices;
        m_sliceLenSamps[ sampleNumber ] = m_durationSamps/m_nSlices[ sampleNumber ];
        setPatterns();
    }
    //==============================================================================
    int getNumSlices( const int sampleNumber )
    {
        return m_nSlices[ sampleNumber ];
    }
    //==============================================================================
    void setNumSteps( int steps )
    {
        m_revPat.resize(steps);
        m_speedPat.resize(steps);
        m_subDivPat.resize(steps);
        m_subDivAmpRampPat.resize(steps);
        m_ampPat.resize(steps);
        for ( int sampleNumber = 0; sampleNumber < m_nSlices.size(); sampleNumber++ )
        {
            m_stepPat[ sampleNumber ].resize(steps);
        }
        if (steps > m_nSteps)
            for (int index = m_nSteps; index < steps; index++)
            {
                m_revPat[index] = 0;
                m_speedPat[index] = 0;
                m_subDivPat[index] = 0;
                m_subDivAmpRampPat[index] = rand01();
                m_ampPat[index] = 1.0f;
                for ( int sampleNumber = 0; sampleNumber < m_nSlices.size(); sampleNumber++ )
                {
                    m_stepPat[ sampleNumber ][ index ] = index % m_nSlices[ sampleNumber ];
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
    void setPhaseRateMultiplierIndex(int i)
    {
        m_phaseRateMultiplier = pow(2, i-3);
    }
    //==============================================================================
    int getPhaseRateMultiplierIndex()
    {
        return 3 + log10(m_phaseRateMultiplier)/log10(2);
    }
    //==============================================================================
    void play(juce::AudioBuffer<float> &buffer)
    {
        if (!m_sampleLoadedFlag) { return; }
        auto envLen = m_phaseRateMultiplier * m_fadeInMs * m_SR / 1000;
        envLen = (int)envLen + 1 ;
        
        auto bufferSize = buffer.getNumSamples();
        auto numChannels = buffer.getNumChannels();
        
        auto increment = m_phaseRateMultiplier;
        
        for (int index = 0; index < bufferSize; index++)
        {
            checkForChangeOfBeat( m_stepCount );
            auto sampleNumber = calculateSampleChoice( m_stepCount );
            auto pos = m_readPos;
            auto subDiv = floor(m_subDivPat[m_stepCount] * 8.0f) + 1.0f ;
            auto subDivLenSamps = m_sliceLenSamps[ sampleNumber ] / subDiv ;
            auto subDivAmp = calculateSubDivAmp( m_stepCount, subDiv, int(pos / subDivLenSamps) );
            auto speedVal = calculateSpeedVal( m_stepCount, (m_readPos / m_sliceLenSamps[ sampleNumber ]) );
            while (pos >= subDivLenSamps){ pos -= subDivLenSamps; }
            auto env = envelope( pos , (int)subDivLenSamps - 1, envLen ); // Check this out more
            auto amp = calculateAmpValue( m_stepCount );
            pos = calculateReverse( m_stepCount, pos, subDivLenSamps );
            pos *= speedVal;
            pos += m_stepPat[ sampleNumber ][ m_stepCount ] * m_sliceLenSamps[ sampleNumber ];
            
            for (int channel = 0; channel < numChannels; channel ++)
            {
                auto val = calculateSampleValue( m_AudioSample[ sampleNumber ], channel % m_AudioSample[ sampleNumber ].getNumChannels(), pos );
                val *= subDivAmp * amp;
                val *= env;
                buffer.setSample(channel, index, val);
            }
            m_readPos += increment;
            if (m_readPos >= m_sliceLenSamps[ sampleNumber ]) {m_stepCount++; m_stepCount %= m_nSteps; }
            while (m_readPos >= m_sliceLenSamps[ sampleNumber ]){ m_readPos -= m_sliceLenSamps[ sampleNumber ]; }
        }
    };
    //==============================================================================
    void play(juce::AudioBuffer<float> &buffer, float bpm, double hostPosition)
    {
        if (!m_sampleLoadedFlag) { return; }
        auto bufferSize = buffer.getNumSamples();
        auto numChannels = buffer.getNumChannels();
        
        for (int index = 0; index < bufferSize; index++)
        {
            checkForChangeOfBeat( m_stepCount );
            auto sampleNumber = calculateSampleChoice( m_stepCount );
            auto hostSyncCompenstation =  calculateHostCompensation( bpm, sampleNumber );
            auto envLen = floor(m_fadeInMs * m_SR / 1000) + 1;
            auto increment = m_phaseRateMultiplier * hostSyncCompenstation;
            

            
            auto hostSampQuarter = 60.0f*m_SR/bpm;
            hostPosition *= hostSampQuarter * m_phaseRateMultiplier * hostSyncCompenstation;
            auto leftOver = hostPosition / m_sliceLenSamps[ sampleNumber ];
            m_stepCount = (int)leftOver;
            leftOver = hostPosition - m_stepCount*m_sliceLenSamps[ sampleNumber ];
            
            m_readPos = leftOver;
            m_stepCount %= m_nSteps;
            
            auto pos = m_readPos;
            auto subDiv = floor(m_subDivPat[m_stepCount] * 8.0f) + 1.0f ;
            auto subDivLenSamps = m_sliceLenSamps[ sampleNumber ] / subDiv ;
            auto subDivAmp = calculateSubDivAmp( m_stepCount, subDiv, int(pos / subDivLenSamps) );
            auto speedVal = calculateSpeedVal( m_stepCount, (m_readPos / m_sliceLenSamps[ sampleNumber ]) );
            while (pos >= subDivLenSamps){ pos -= subDivLenSamps; }
            auto env = envelope( floor(pos / increment) , floor(subDivLenSamps / increment), envLen ); // Check this out more
            auto amp = calculateAmpValue( m_stepCount );
            pos = calculateReverse(m_stepCount, pos, subDivLenSamps);
            pos *= speedVal;
            pos += m_stepPat[ sampleNumber ][m_stepCount] * m_sliceLenSamps[ sampleNumber ];
            
            for (int channel = 0; channel < numChannels; channel ++)
            {
                auto val = calculateSampleValue(m_AudioSample[ sampleNumber ], channel % m_AudioSample[ sampleNumber ].getNumChannels(), pos);
                val *= subDivAmp * amp;
                val *= env;
                buffer.setSample(channel, index, val);
            }
            m_readPos +=  increment;
            if (m_readPos >= m_sliceLenSamps[ sampleNumber ]) {m_stepCount++; m_stepCount %= m_nSteps; }
            while (m_readPos >= m_sliceLenSamps[ sampleNumber ]){ m_readPos -= m_sliceLenSamps[ sampleNumber ]; }
            
        }
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
    bool checkForChangeOfBeat(int currentStep)
    {
        bool newStep = false;
        if ( currentStep != m_lastStep )
        {   // This is just to randomise patterns at the start of each loop
            if (currentStep == 0 && m_randomOnLoopFlag) { randomiseAll(); }
            checkPatternRandomisationFlags();
            newStep = true;
        }
        m_lastStep = currentStep;
        return newStep;
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
        return round( m_sampleChoicePat[ currentStep ] * m_AudioSample.size() );
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
    float calculateHostCompensation( float bpm, const int sampleNumber ){
        if (!m_sampleLoadedFlag ) { return 1; }
        m_hostBPM = bpm;
        auto hostQuarterNoteSamps = m_SR*60.0f/bpm;
        auto multiplier = 1.0f;
        if (!m_syncToHostFlag) { return 1; }
        if (m_sliceLenSamps[ sampleNumber ] == hostQuarterNoteSamps){
            return 1;
        }
        else if (m_sliceLenSamps[ sampleNumber ] < hostQuarterNoteSamps){
            while (m_sliceLenSamps[ sampleNumber ] * multiplier < hostQuarterNoteSamps){ multiplier *= 2; }
            auto lastDiff = abs(hostQuarterNoteSamps - m_sliceLenSamps[ sampleNumber ]*multiplier);
            auto halfMultiplier = multiplier * 0.5;
            auto newDiff = abs(hostQuarterNoteSamps - m_sliceLenSamps[ sampleNumber ]*halfMultiplier);
            if ( newDiff < lastDiff) { multiplier = halfMultiplier; }
        }
        else
        {
            while (m_sliceLenSamps[ sampleNumber ] > hostQuarterNoteSamps *multiplier){ multiplier *= 2; }
            float lastDiff = abs(hostQuarterNoteSamps*multiplier - m_sliceLenSamps[ sampleNumber ]);
            auto halfMultiplier = multiplier * 0.5f;
            auto newDiff = abs(hostQuarterNoteSamps*halfMultiplier - m_sliceLenSamps[ sampleNumber ]);
            if (newDiff < lastDiff ) { multiplier = halfMultiplier;}
            multiplier = 1/multiplier;
        }
        return (m_sliceLenSamps[ sampleNumber ]*multiplier) / hostQuarterNoteSamps ;
    };
    
    //==============================================================================
    void setPatterns( )
    {
        m_revPat.resize(m_nSteps);
        m_speedPat.resize(m_nSteps);
        m_subDivPat.resize(m_nSteps);
        m_subDivAmpRampPat.resize(m_nSteps);
        m_ampPat.resize(m_nSteps);
        for ( int sampleNumber = 0; sampleNumber < m_nSlices.size(); sampleNumber++ )
        {
            m_stepPat[ sampleNumber ].resize(m_nSteps);
        }
        for (int index = 0; index < m_nSteps; index++)
        {
            m_revPat[index] = 0;
            m_speedPat[index] = 0;
            m_subDivPat[index] = 0;
            m_subDivAmpRampPat[index] = rand01();
            m_ampPat[index] = 1.0f;
            for ( int sampleNumber = 0; sampleNumber < m_nSlices.size(); sampleNumber++ )
            {
                m_stepPat[ sampleNumber ][index] = index % m_nSlices[ sampleNumber ];
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
        for ( int sampleNumber = 0; sampleNumber < m_stepPat.size(); sampleNumber++ )
        {
            m_stepPat[ sampleNumber ].resize(m_nSteps);
            for (int index = 0; index < m_nSteps; index++)
            {
                if(  m_stepShuffleProb/100.0f <= rand01() )
                {
                    m_stepPat[ sampleNumber ][index] = index;
                }
                else
                {
                    int step = floor(rand01() * m_nSteps);
                    m_stepPat[ sampleNumber ][index] = step;
                }
            }
        }

    };
    //==============================================================================
    void setRandomVoicePattern()
    {
        setRandPat( m_sampleChoicePat, m_sampleChoiceProb/100.0f );
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

