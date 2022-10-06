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
#include <algorithm>    // std::random_shuffle
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
        m_flip.resize( m_erStages );
        m_shuffle.resize( m_erStages );
        for ( int s = 0; s < m_erStages; s++ )
        {
            m_flip[s].resize(m_erChannels);
            for (int c = 0; c < m_erChannels; c ++)
            {
                m_shuffle[s].push_back( c );
                er[s][c].setMaxDelayLength( 0.1 );
                er[s][c].intialise( m_SR, 1, 1, 64 );
            }
        }
        
        flipAndShuffle();
        hadamard( m_hadamard, m_erChannels );
        randomiseAll();
    }
    //==============================================================================
    ~sjf_reverb() {}
    //==============================================================================
    void intialise( int sampleRate , int totalNumInputChannels, int totalNumOutputChannels, int samplesPerBlock)
    {
        m_SR = sampleRate;
        for ( int s = 0; s < m_erStages; s++ )
        {
            for (int c = 0; c < m_erChannels; c ++)
            {
                er[s][c].setMaxDelayLength( 0.1 );
                er[s][c].intialise( m_SR, 1, 1, samplesPerBlock);
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
                er[s][c].setDelTimeL( dt );
            }
        }
    }
    //==============================================================================
    void processAudio( juce::AudioBuffer<float> &buffer )
    {
        
    }
    //==============================================================================
private:
    //==============================================================================
    void processEarlyReflections( juce::AudioBuffer<float> &buffer )
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
                if ( rn >= 0.5 ) { m_flip[s][c] = 1; }
                else { m_flip[s][c] = -1; }
            }
        }
        
        for ( int s = 0; s < m_erStages; s++ )
        {
            
            std::random_shuffle ( m_shuffle[s].begin(), m_shuffle[s].end() );
            std::string line;
            for ( int c = 0; c < m_erChannels; c ++ )
            {
                line.append( std::to_string( m_shuffle[s][c]) );
                line.append( " " );
            }
            DBG(line);
        }
    }
    //==============================================================================
    void hadamard( std::vector< std::vector<float> > &hadamard, int size )
    {
        // note size must be a power of 2!!!
        hadamard.resize(size);
        for ( int i = 0; i < size; i++ )
        {
            hadamard[i].resize(size);
        }
        hadamard[0][0] = 1.0f / sqrt( size ); // most simple matrix of size 1 is [1], whole matrix is multiplied by 1 / sqrt(size)
        for ( int k = 1; k < size; k += k ) {
            
            // Loop to copy elements to
            // other quarters of the matrix
            for (int i = 0; i < k; i++) {
                for (int j = 0; j < k; j++) {
                    hadamard[i + k][j] = hadamard[i][j];
                    hadamard[i][j + k] = hadamard[i][j];
                    hadamard[i + k][j + k] = -hadamard[i][j];
                }
            }
        }
    }
    //==============================================================================
    const static int m_erChannels = 8; // must be a power of 2 because of hadamard matrix!!!!
    const static int m_erStages = 4;
    
    sjf_delayLine er[ m_erStages ][ m_erChannels ]; // early reflections
    int m_SR = 44100;
    float m_erTotalLength = 300;
    float m_wet = 0.2;
    std::vector< std::vector<float> > m_hadamard, m_flip, m_shuffle;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (sjf_reverb)
};



#endif /* sjf_reverb_h */
