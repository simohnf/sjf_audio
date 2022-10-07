//
//  sjf_reverb.h
//
//  Created by Simon Fay on 05/10/2022.
//


#ifndef sjf_reverb_h
#define sjf_reverb_h
#include <JuceHeader.h>
#include "sjf_delayLine.h"
#include "sjf_audioUtilities.h"
#include <algorithm>    // std::random_erShuffle
#include <random>       // std::default_random_engine
#include <vector>
#include <time.h>

class sjf_monoDelay
{
public:
    sjf_monoDelay(){}
    ~sjf_monoDelay(){};
    
    void initialise( int sampleRate , int sizeMS )
    {
        m_SR = sampleRate;
        int size = round(m_SR * 0.001 * sizeMS);
        m_delayLine.setSize( 1, size );
        m_delayLine.clear();
    }
    
    void setDelayTime( float delayInMS )
    {
        m_delayTimeInSamps = round(delayInMS * m_SR * 0.001);
    }
    
    void clearBuffer()
    {
        m_delayLine.clear();
    }
    
    void writeToBuffer( juce::AudioBuffer<float>& sourceBuffer, float gain )
    {
        auto bufferSize = sourceBuffer.getNumSamples();
        auto delayBufferSize = m_delayLine.getNumSamples();
        for (int index = 0; index < bufferSize; index++)
        {
            auto wp = (m_writePos + index) % delayBufferSize;
            m_delayLine.setSample(0, wp, sourceBuffer.getSample(0, index) * gain);
        }
    }
    
    void addFromBuffer(juce::AudioBuffer<float>& destinationBuffer, float gain)
    {
        auto bufferSize = destinationBuffer.getNumSamples();
        auto delayBufferSize = m_delayLine.getNumSamples();
        auto numChannels = destinationBuffer.getNumChannels();
        for (int index = 0; index < bufferSize; index++)
        {
            for (int channel = 0; channel < numChannels; channel++)
            {
                float channelReadPos = m_writePos - m_delayTimeInSamps + index;
                while ( channelReadPos < 0 ) { channelReadPos += delayBufferSize; }
                while (channelReadPos >= delayBufferSize) { channelReadPos -= delayBufferSize; }
                auto val = cubicInterpolate(m_delayLine, channel % m_delayLine.getNumChannels(), channelReadPos) * gain;
                destinationBuffer.addSample(channel, index, val );
            }
        }
    };
    
    
    void copyFromBuffer( juce::AudioBuffer<float>& destinationBuffer, float gain )
    {
        auto bufferSize = destinationBuffer.getNumSamples();
        auto delayBufferSize = m_delayLine.getNumSamples();
        for (int index = 0; index < bufferSize; index++)
        {
            int readPos = m_writePos - m_delayTimeInSamps + index;
            while ( readPos < 0 ) { readPos += delayBufferSize; }
            while (readPos >= delayBufferSize) { readPos -= delayBufferSize; }
            auto val = m_delayLine.getSample( 0, readPos ) * gain;
            destinationBuffer.setSample( 0, index, val );
        }
    }
    
    void updateBufferPositions(int bufferSize)
    {
        auto delayBufferSize = m_delayLine.getNumSamples();
        //    Update write position ensuring it stays within size of delay buffer
        m_writePos += bufferSize;
        while ( m_writePos >= delayBufferSize )
        {
            m_writePos -= delayBufferSize;
        }
    };
    
private:
    float m_delayTimeInSamps, m_SR = 44100;
    int m_writePos = 0;
    juce::AudioBuffer<float> m_delayLine;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_monoDelay )
};

//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================

class sjf_reverb
{
public:
    //==============================================================================
    sjf_reverb()
    {
        srand((unsigned)time(NULL));
        m_erFlip.resize( m_erStages );
        m_erShuffle.resize( m_erStages );
        for ( int s = 0; s < m_erStages; s++ )
        {
            m_erFlip[s].resize(m_erChannels);
            for (int c = 0; c < m_erChannels; c ++)
            {
                m_erShuffle[s].push_back( c );
                er[s][c].initialise( m_SR , m_erTotalLength );
            }
        }
        
        for (int c = 0; c < m_erChannels; c ++)
        {
            lr[c].initialise( m_SR , m_lrTotalLength );
        }
        
        revTemp.setSize(1, m_blockSize);
        
        flipAndShuffle();
        genHadamard( m_hadamard, m_erChannels );
        randomiseAll();
    }
    //==============================================================================
    ~sjf_reverb() {}
    //==============================================================================
    void intialise( int sampleRate , int totalNumInputChannels, int totalNumOutputChannels, int samplesPerBlock)
    {
        m_SR = sampleRate;
        m_blockSize = samplesPerBlock;
        
        
        revTemp.setSize(1, m_blockSize);
        
        for ( int s = 0; s < m_erStages; s++ )
        {
            for (int c = 0; c < m_erChannels; c ++)
            {
                er[s][c].initialise( m_SR , m_erTotalLength );
            }
        }
        for (int c = 0; c < m_erChannels; c ++)
        {
            lr[c].initialise( m_SR , m_lrTotalLength );
        }
    }
    //==============================================================================
    void randomiseAll()
    {
        // set each stage to be twice the length of the last
        float c = 0;
        for ( int s = 0; s < m_erStages; s++ )
        {
            c += pow(2, s);
        }
        float frac = 1.0f / c; // fraction of total length for first stage
        for ( int s = 0; s < m_erStages; s++ )
        {
            auto dtC = m_erTotalLength * frac * pow(2, s) / (float)m_erChannels;
            for (int c = 0; c < m_erChannels; c ++)
            {
                // space each channel so that the are randomly spaced but with roughly even distribution
                auto dt = rand01() *  dtC;
                dt += ( dtC * c );
                er[s][c].setDelayTime( dt );
            }
        }
        
        auto minLRtime = m_lrTotalLength * 0.25;
        auto dtC = ( m_lrTotalLength - minLRtime ) / m_erChannels;
        
        for (int c = 0; c < m_erChannels; c ++)
        {
            auto dt = rand01() * dtC;
            dt += (dtC * c) + minLRtime;
            lr[c].setDelayTime(dt);
        }
    }
    //==============================================================================
    void processAudio( juce::AudioBuffer<float> &buffer )
    {
        auto nInChannels = buffer.getNumChannels();
        auto bufferSize = buffer.getNumSamples();
        auto equalPowerGain = sqrt( 1.0f / nInChannels );
        
//         copy input to temp buffer
        revTemp.clear();
        for ( int i = 0; i < nInChannels; i++ )
        {
            revTemp.addFrom( 0, 0, buffer, i, 0, bufferSize, equalPowerGain );
        }
        
        for ( int s = 0; s < m_erStages; s ++ )
        {
            if ( s == 0 )
            {
                for ( int c = 0; c < m_erChannels; c ++)
                {
                    er[s][c].writeToBuffer( revTemp, m_erFlip[s][c] );
                }
            }
            else
            {
                for ( int c = 0; c < m_erChannels; c ++)
                {
                    revTemp.clear(); // first Clear temp buffer
                    for ( int cp = 0; cp < m_erChannels; cp++ )
                    {
                        auto preStage = s-1;
                        auto shuffledChannel = m_erShuffle[preStage][cp];
                        auto mult = m_hadamard[ preStage ][ shuffledChannel ];
                        er[ preStage ][ shuffledChannel ].addFromBuffer( revTemp, mult );
                    }
                    er[s][c].writeToBuffer( revTemp, m_erFlip[s][c] );
                }
            }
        }
        // late reflections
        for ( int c = 0; c < m_erChannels; c ++)
        {
            revTemp.clear(); // first Clear temp buffer
            for ( int cp = 0; cp < m_erChannels; cp++ )
            {
                auto preStage = m_erStages - 1;
                auto shuffledChannel = m_erShuffle[preStage][cp];
                auto mult = m_hadamard[ preStage ][ shuffledChannel ];
                er[ preStage ][ shuffledChannel ].addFromBuffer( revTemp, mult );
            }
            lr[c].writeToBuffer( revTemp, 1.0f );
        }
        
        buffer.clear();


        for ( int c = 0; c < nInChannels; c ++ )
        {
            revTemp.clear(); // clear temporary buffer
            er[m_erStages-1][ m_erShuffle[m_erStages-1][c%m_erChannels] ].copyFromBuffer(revTemp, 1.0f);
            buffer.addFrom( c, 0, revTemp, 0, 0, bufferSize );
        }
        
        for ( int s = 0; s < m_erStages; s ++ )
        {
            for ( int c = 0; c < m_erChannels; c ++)
            {
                er[s][c].updateBufferPositions( bufferSize );
            }
        }
            
    }
    //==============================================================================
private:
    //==============================================================================
    void processEarlyReflections( juce::AudioBuffer<float> &buffer, int stage )
    {
        
    }
    //==============================================================================
    void flipAndShuffle( )
    {
        
        for ( int s = 0; s < m_erStages; s++ )
        {
            
            for ( int c = 0; c < m_erChannels; c ++ )
            {
                // randomise some polarities
                auto rn = rand01();
                if ( rn >= 0.5 ) { m_erFlip[s][c] = 1; }
                else { m_erFlip[s][c] = -1; }
            }
        }
        
        for ( int s = 0; s < m_erStages; s++ )
        {
            std::random_shuffle ( m_erShuffle[s].begin(), m_erShuffle[s].end() );
        }
    }
    //==============================================================================

    
    void genHadamard( std::vector< std::vector<float> > &h, int size )
    {
        // note size must be a power of 2!!!
        h.resize(size);
        for ( int i = 0; i < size; i++ )
        {
            h[i].resize(size);
        }
        h[0][0] = 1.0f / sqrt( size ); // most simple matrix of size 1 is [1], whole matrix is multiplied by 1 / sqrt(size)
        for ( int k = 1; k < size; k += k ) {
            
            // Loop to copy elements to
            // other quarters of the matrix
            for (int i = 0; i < k; i++)
            {
                for (int j = 0; j < k; j++)
                {
                    h[i + k][j] = h[i][j];
                    h[i][j + k] = h[i][j];
                    h[i + k][j + k] = -h[i][j];
                }
            }
        }
        

    }
    //==============================================================================
    const static int m_erChannels = 8; // must be a power of 2 because of hadamard matrix!!!!
    const static int m_erStages = 4;
    
    sjf_monoDelay er[ m_erStages ][ m_erChannels ]; // early reflections
    sjf_monoDelay lr[ m_erChannels ]; // late reflections
    int m_SR = 44100, m_blockSize = 64;
    float m_erTotalLength = 300, m_lrTotalLength = 200;
    float m_wet = 0.2;
    std::vector< std::vector<float> > m_hadamard, m_erFlip;
    std::vector< std::vector<int> > m_erShuffle;
    
    juce::AudioBuffer< float > revTemp;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (sjf_reverb)
};



#endif /* sjf_reverb_h */
