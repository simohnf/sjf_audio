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

class sjf_reverb
{
public:
    //==============================================================================
    sjf_reverb()
    {
        srand((unsigned)time(NULL));
        DBG( rand01() );
        m_erFlip.resize( m_erStages );
        m_erShuffle.resize( m_erStages );
        for ( int s = 0; s < m_erStages; s++ )
        {
            m_erFlip[s].resize(m_erChannels);
            for (int c = 0; c < m_erChannels; c ++)
            {
                m_erShuffle[s].push_back( c );
                er[s][c].setMaxDelayLength( 0.1 );
                er[s][c].intialise( m_SR, 1, 1, m_blockSize );
            }
        }
        
        
        revTemp.setSize(m_erChannels, m_blockSize);
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
        revTemp.setSize(m_erChannels, m_blockSize);
        
        for ( int s = 0; s < m_erStages; s++ )
        {
            for (int c = 0; c < m_erChannels; c ++)
            {
                er[s][c].setMaxDelayLength( 0.1 );
                er[s][c].intialise( m_SR, 1, 1, m_blockSize);
            }
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
                DBG(s << " " << c << " " << dt);
                er[s][c].setDelTimeL( dt );
            }
        }
    }
    //==============================================================================
    void processAudio( juce::AudioBuffer<float> &buffer )
    {
        revTemp.clear();
        
        // copy input to temp buffer
        for ( int c = 0; c < m_erChannels; c++ )
        {
            for ( int i = 0; i < buffer.getNumChannels(); i++ )
            {
                revTemp.copyFrom( c, 0, buffer, i, 0, buffer.getNumSamples() );
//                er[s][c].writeToDelayBuffer( buffer, 0.707f ); // equal power from both channels into every buffer
                rev.applyGain( 0, buffer.getNumSamples(), 0.707f );
            }
        }
        
        
        for (int s = 0; s < m_erStages; s++)
        {
            for ( int c = 0; c < m_nerChannels; c++ )
            {
//                er[s][c].writeToDelayBuffer(
            }
        }
        
        buffer.clear();
        er[m_erStages - 1][0].copyFromDelayBuffer( buffer, 1.0f );
        
        for (int s = 0; s < m_erStages; s++)
        {
            for ( int c = 0; c < m_erChannels; c++)
            {
                er[s][c].updateBufferPositions( buffer.getNumSamples() );
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
            std::string line;
            for ( int c = 0; c < m_erChannels; c ++ )
            {
                line.append( std::to_string( m_erShuffle[s][c]) );
                line.append( " " );
            }
            DBG(line);
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
            for (int i = 0; i < k; i++) {
                for (int j = 0; j < k; j++) {
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
    
    sjf_delayLine er[ m_erStages ][ m_erChannels ]; // early reflections
    int m_SR = 44100, m_blockSize = 64;
    float m_erTotalLength = 300;
    float m_wet = 0.2;
    std::vector< std::vector<float> > m_hadamard, m_erFlip;
    std::vector< std::vector<int> > m_erShuffle;
    
    juce::AudioBuffer< float > revTemp;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (sjf_reverb)
};



#endif /* sjf_reverb_h */
