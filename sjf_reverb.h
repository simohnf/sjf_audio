//
//  sjf_reverb2.h
//
//  Created by Simon Fay on 07/10/2022.
//


#ifndef sjf_reverb2_h
#define sjf_reverb2_h
#include <JuceHeader.h>
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
        m_delayBufferSize = size;
        m_delayLine.resize( size, 0 );
    }
    
    void setDelayTime( float delayInMS )
    {
        m_delayTimeInSamps = round(delayInMS * m_SR * 0.001);
    }
    
    void writeToBuffer( std::vector<float> &sourceBuffer )
    {
        auto bufferSize = sourceBuffer.size();
        for (int index = 0; index < bufferSize; index++)
        {
            auto wp = (m_writePos + index) % m_delayBufferSize;
            m_delayLine[wp] = sourceBuffer[ index ];
        }
    }
    
    void writeToBuffer( std::vector<float> &sourceBuffer, float gain )
    {
        auto bufferSize = sourceBuffer.size();
        for (int index = 0; index < bufferSize; index++)
        {
            auto wp = (m_writePos + index) % m_delayBufferSize;
            m_delayLine[wp] = sourceBuffer[ index ] * gain;
        }
    }
    
    void addToBuffer( std::vector<float> &sourceBuffer, float gain )
    {
        auto bufferSize = sourceBuffer.size();
        for (int index = 0; index < bufferSize; index++)
        {
            auto wp = (m_writePos + index) % m_delayBufferSize;
            m_delayLine[ wp ]  += sourceBuffer[ index ] * gain;
        }
    }
    
    
    void addFromBuffer( std::vector<float> &destinationBuffer, float gain)
    {
        auto bufferSize = destinationBuffer.size();
        for (int index = 0; index < bufferSize; index++)
        {
            auto readPos = (int)(m_delayBufferSize + m_writePos - m_delayTimeInSamps + index) % m_delayBufferSize;
            destinationBuffer[ index ] += m_delayLine [ readPos ] * gain;
        }
    };
    
    
    void copyFromBuffer( std::vector<float> &destinationBuffer )
    {
        auto bufferSize = destinationBuffer.size();
        for (int index = 0; index < bufferSize; index++)
        {
            int readPos = m_writePos - m_delayTimeInSamps + index;
            while ( readPos < 0 ) { readPos += m_delayBufferSize; }
            while (readPos >= m_delayBufferSize) { readPos -= m_delayBufferSize; }
            auto val = m_delayLine [ readPos ];
            destinationBuffer[ index ] =  val;
        }
    };
    
    float getSample( int indexThroughCurrentBuffer )
    {
        float readPos = m_delayBufferSize + m_writePos - m_delayTimeInSamps + indexThroughCurrentBuffer;
        while (readPos >= m_delayBufferSize) { readPos -= m_delayBufferSize; }
        return m_delayLine [ readPos ];
    }
    
    void setSample( int indexThroughCurrentBuffer, float value )
    {
        auto wp = m_writePos + indexThroughCurrentBuffer + m_delayBufferSize;
        wp %= m_delayBufferSize;
        m_delayLine[ wp ]  = value;
    }
    
    int updateBufferPositions(int bufferSize)
    {
        //    Update write position ensuring it stays within size of delay buffer
        m_writePos += bufferSize;
        while ( m_writePos >= m_delayBufferSize )
        {
            m_writePos -= m_delayBufferSize;
        }
        return m_writePos;
    };
    
private:
    float m_delayTimeInSamps, m_SR = 44100;
    int m_writePos = 0, m_delayBufferSize;
    std::vector<float> m_delayLine;
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
        
        
        for (int c = 0; c < m_erChannels; c ++)
        {
            for ( int s = 0; s < m_erStages; s++ )
            {
                er[s][c].initialise( m_SR , m_erTotalLength );
            }
            lr[c].initialise( m_SR , m_lrTotalLength );
        }
        
        randomPolarityFlips();
        randomShuffles();
        genHadamard( );
        randomiseAll( );
    }
    //==============================================================================
    ~sjf_reverb() {}
    //==============================================================================
    void intialise( int sampleRate , int totalNumInputChannels, int totalNumOutputChannels, int samplesPerBlock)
    {
        m_SR = sampleRate;
        m_blockSize = samplesPerBlock;
        
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
        
        for ( int samp = 0; samp < bufferSize; samp++ )
        {
//            float sum = 0.0f;
            m_sum = 0.0f;
            // sum left and right and apply equal power gain
            for ( int inC = 0; inC < nInChannels; inC++ ) { m_sum += buffer.getSample( inC, samp ) * equalPowerGain; }
            // first copy input sample to temp buffer
            for ( int c = 0; c < m_erChannels; c ++ ){ v1[c] = m_sum; }
            // early reflections
            processEarlyReflections( samp );
            // last stage of er is in v1
            // late reflections
            processLateReflections( samp );
            // mixed sum of lr is in v2
            
            for ( int c = 0; c < nInChannels; c ++ )
            {
                buffer.setSample( c, samp, ( v1[c] + v2[c] ) );
            }
        }
        
        for ( int c = 0; c < m_erChannels; c++ )
        {
            for ( int s = 0; s < m_erStages; s ++ )
            {
               er[s][c].updateBufferPositions( bufferSize );
            }
            lr[c].updateBufferPositions( bufferSize );
        }
    }
    //==============================================================================
    
private:
    //==============================================================================
    void processEarlyReflections( int indexThroughCurrentBuffer )
    {
//        float sum = 0.0f;
//        m_sum = 0.0f;
        // then count through stage of er
        for ( int s = 0; s < m_erStages; s++ )
        {
            for ( int c = 0; c < m_erChannels; c ++ )
            {
                // for each channel
                // write previous to er vector
                er[s][c].setSample( indexThroughCurrentBuffer, v1[c] );
                // copy delayed sample from shuffled channel to temporary
                v1[ c ] =  er[ s ][ m_erShuffle[ s ][ c ] ].getSample( indexThroughCurrentBuffer );
                // flip some polarities
                if ( m_erFlip[s][c] ){ v1[ c ] *= -1.0f; }
            }
            for (int c = 0; c < m_erChannels; c ++ )
            {
                // mix using hadamard
                m_sum = 0.0f;
                for ( int ch = 0; ch < m_erChannels; ch++ )
                {
                    m_sum += ( v1[ ch ] * m_hadamard[ c ][ ch ] );
                }
                v2[c] = m_sum;
            }
            for ( int c = 0; c < m_erChannels; c++ )
            { v1[c] = v2[c]; }
        }
    }
    //==============================================================================
    void processLateReflections( int indexThroughCurrentBuffer )
    {
        // copy delayed samples into temp3
//        float sum = 0.0f;
        m_sum = 0.0f;
        for ( int c = 0; c < m_erChannels; c++ )
        {
            v2[c] = lr[c].getSample( indexThroughCurrentBuffer );
            m_sum += v2[c];
        }
        m_sum *= m_householderWeight;
        
        for ( int c = 0; c < m_erChannels; c++ ) { v2[c] += m_sum; }
        
        // mixed delayed outputs are in temp3
        for ( int c = 0; c < m_erChannels; c++ )
        {
            auto val = ( m_lrFB * v2[c] ) + v1[c];
            // copy last er sample to respective buffer
            lr[c].setSample( indexThroughCurrentBuffer, val );
        }
    }
    //==============================================================================
    void randomPolarityFlips()
    {
        for ( int s = 0; s < m_erStages; s++ )
        {
            for ( int c = 0; c < m_erChannels; c ++ )
            {
                // randomise some polarities
                auto rn = rand01();
                if ( rn >= 0.5 ) { m_erFlip[s][c] = true; }
                else { m_erFlip[s][c] = false; }
            }
        }
    }
    //==============================================================================
    void randomShuffles( )
    {
        for ( int s = 0; s < m_erStages; s++ )
        {
            for ( int c = 0; c < m_erChannels; c ++ )
            {
                m_erShuffle[s][c] = c;
            }
            std::random_shuffle ( std::begin(m_erShuffle[s]), std::end(m_erShuffle[s]) );
        }
    }
    //==============================================================================
    
    void genHadamard( )
    {
        m_hadamard[0][0] = 1.0f / sqrt( m_erChannels ); // most simple matrix of size 1 is [1], whole matrix is multiplied by 1 / sqrt(size)
        for ( int k = 1; k < m_erChannels; k += k ) {
            // Loop to copy elements to
            // other quarters of the matrix
            for (int i = 0; i < k; i++)
            {
                for (int j = 0; j < k; j++)
                {
                    m_hadamard[i + k][j] = m_hadamard[i][j];
                    m_hadamard[i][j + k] = m_hadamard[i][j];
                    m_hadamard[i + k][j + k] = -m_hadamard[i][j];
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
    float m_erTotalLength = 300, m_lrTotalLength = 200, m_lrFB = 0.85;
    float m_dry = 1.0f, m_wet = 0.5f;
    float m_sum; // every little helps with cpu, I reuse this at multiple stages just to add and mix sample values
    float m_householderWeight = -2.0f / m_erChannels;
    
    // implemented as arrays rather than vectors for cpu
    float m_hadamard[ m_erChannels ][ m_erChannels ];
    float v1[ m_erChannels ], v2[ m_erChannels ]; // these will hold one sample for each channel
    int m_erShuffle[ m_erStages ][ m_erChannels ];
    bool m_erFlip[ m_erStages ][ m_erChannels ];
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (sjf_reverb)
};



#endif /* sjf_reverb_h */



