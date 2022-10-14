//
//  sjf_reverb2.h
//
//  Created by Simon Fay on 07/10/2022.
//


#ifndef sjf_reverb2_h
#define sjf_reverb2_h
#include <JuceHeader.h>
#include "sjf_audioUtilities.h"
#include "sjf_monoDelay.h"
#include "sjf_lpf.h"
#include <algorithm>    // std::random_erShuffle
#include <random>       // std::default_random_engine
#include <vector>
#include <time.h>


#define PI 3.14159265

//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
class sjf_osc
{
public:
    sjf_osc()
    {
        setSine();
        m_increment = m_frequency * m_wavetableSize / m_SR;
    }
    ~sjf_osc(){}
    
    void initialise( int sampleRate )
    {
        m_SR = sampleRate;
        setFrequency( m_frequency );
    }
    
    void initialise( int sampleRate, float frequency )
    {
        m_SR = sampleRate;
        setFrequency( frequency );
    }
    
    void setFrequency( float frequency )
    {
        m_frequency = frequency;
        m_increment = m_frequency * m_wavetableSize / m_SR;
    }
    
    float getSample( )
    {
        m_readPos +=  m_increment;
        while ( m_readPos >= m_wavetableSize )
        { m_readPos -= m_wavetableSize; }
        return cubicInterpolate( m_readPos );
    }

    
private:
    
    void setSine()
    {
        for (int index = 0; index< m_wavetableSize; index++)
        {
            m_table[index] = sin( index * 2 * PI / m_wavetableSize ) ;
        }
    }
    
    float cubicInterpolate(float readPos)
    {
//        double y0; // previous step value
//        double y1; // this step value
//        double y2; // next step value
//        double y3; // next next step value
//        double mu; // fractional part between step 1 & 2
        findex = readPos;
        if(findex < 0){ findex+= m_wavetableSize;}
        else if(findex > m_wavetableSize){ findex-= m_wavetableSize;}
        
        index = findex;
        mu = findex - index;
        
        if (index == 0)
        {
            y0 = m_table[ m_wavetableSize - 1 ];
        }
        else
        {
            y0 = m_table[ index - 1 ];
        }
        y1 = m_table[ index % m_wavetableSize ];
        y2 = m_table[ (index + 1) % m_wavetableSize ];
        y3 = m_table[ (index + 2) % m_wavetableSize ];
        
        
        mu2 = mu*mu;
        a0 = y3 - y2 - y0 + y1;
        a1 = y0 - y1 - a0;
        a2 = y2 - y0;
        a3 = y1;
        
        return (a0*mu*mu2 + a1*mu2 + a2*mu + a3);
    }
    
    const static int m_wavetableSize = 512;
    float m_SR = 44100.0f, m_frequency = 1.0f, m_readPos = 0, m_increment;
    std::array< float, m_wavetableSize > m_table;
    
    // variables for cubic interpolation (to save allocation time)
    double a0, a1, a2, a3, mu, mu2;
    double y0, y1, y2, y3; // fractional part between step 1 & 2
    float findex;
    int index;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_osc )
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
        
        randomPolarityFlips( );
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
            lrMod[c].initialise( m_SR );
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
//        std::array< float, m_erChannels > erDT;
        float frac = 1.0f / c; // fraction of total length for first stage
        for ( int s = 0; s < m_erStages; s++ )
        {
            auto dtC = m_erTotalLength * frac * pow(2, s) / (float)m_erChannels;
            for (int c = 0; c < m_erChannels; c ++)
            {
                // space each channel so that the are randomly spaced but with roughly even distribution
                auto dt = rand01() *  dtC;
                dt += ( dtC * c );
                erDT[ s ][ c ] = dt;
                auto lpfF = sqrt( rand01() );
                erLPF[ s ][ c ].setCutoff( lpfF );
            }
            std::random_shuffle ( std::begin( erDT[ s ] ), std::end( erDT[ s ] ) ); // shuffle channels
            for ( int c = 0; c < m_erChannels; c ++ )
            {
                er[ s ][ c ].setDelayTime( erDT[ s ][ c ] );
                DBG(s << " " << c << " " << er[ s ][ c ].getDelayTimeMS() );
            }
        }
        
        float maxLenForERChannel = 0.0f;
        int channel;
        for (int c = 0; c < m_erChannels; c ++)
        {
            float sum = 0.0f;
            for ( int s = 0; s < m_erStages; s++ ) { sum += er[ s ][ c ].getDelayTimeMS(); }
            if ( sum >  maxLenForERChannel ) { maxLenForERChannel = sum; channel = c; }
        }
        
        auto minLRtime = fmin( m_lrTotalLength * 0.5, maxLenForERChannel * 0.66);
        auto dtC = ( m_lrTotalLength - minLRtime ) / (float)m_erChannels;
        
        for (int c = 0; c < m_erChannels; c ++)
        {
            auto dt = rand01() * dtC;
            dt += (dtC * c) + minLRtime;
            lrDT[c] = dt;
            lrLPF[c].setCutoff( sqrt( rand01() ) );
            lrMod[c].setFrequency( pow( rand01(), 5.0f ) * 0.5f );
            lrModD[c] = pow( rand01(), 4.0f ) * 0.6f;
        }
        std::random_shuffle ( std::begin( lrDT ), std::end( lrDT ) );
        for ( int c = 0; c < m_erChannels; c ++ )
        {
            lr[c].setDelayTime( lrDT[c] ) ;
        }
    }
    
    //==============================================================================
    void processAudio( juce::AudioBuffer<float> &buffer )
    {
        auto nInChannels = buffer.getNumChannels();
        auto bufferSize = buffer.getNumSamples();
        auto equalPowerGain = sqrt( 1.0f / nInChannels );
        
        for ( int c = 0; c < m_erChannels; c ++ )
        {
            for ( int s = 0; s < m_erStages; s++ )
            {
//                er[ s ][ c ].setDelayTime( erDT[ s ][ c ] *  m_size );
                DBG(s << " " << c << " " << er[ s ][ c ].getDelayTimeMS() );
            }
            lr[ c ].setDelayTime( lrDT[ c ] *  m_size );
            DBG( c << " " << lr[ c ].getDelayTimeMS() );
        }
        
        for ( int samp = 0; samp < bufferSize; samp++ )
        {
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
            
            processOutput( buffer, samp, nInChannels );

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
    void setSize ( float newSize )
    {
        DBG( "newSize " << newSize );
        m_size = newSize * 0.01f;
    }
    //==============================================================================
    void setDecay ( float newDecay )
    {
        DBG( "newDecay " << newDecay );
        m_lrFB = newDecay * 0.01f;
    }
    //==============================================================================
    void setModulation ( float newModulation )
    {
        DBG( "newModulation " << newModulation );
        m_modulation = newModulation * 0.01f;
    }
    //==============================================================================
    
private:
    //==============================================================================
    void processOutput( juce::AudioBuffer<float> &buffer, int samp, int nInChannels )
    {
        for ( int c = 0; c < nInChannels; c ++ )
        {
            m_sum = 0.0f;
            for (int rc = 0; rc < m_erChannels; rc++ )
            {
                if ( rc % nInChannels == c )
                {
                    // mix alternating channels from early and late reflections
                    m_sum += v1[ c ] + v2[ rc ];
                }
            }
            m_sum *= m_wet;
//            m_sum += buffer.getSample( c, samp ) * m_dry;
            buffer.setSample( c, samp, ( m_sum ) );
        }
    }
    //==============================================================================
    void processEarlyReflections( int indexThroughCurrentBuffer )
    {
        // count through stage of er
        for ( int s = 0; s < m_erStages; s++ )
        {
            for ( int c = 0; c < m_erChannels; c++ )
            {
                // for each channel
                // write previous to er vector (& flip smoe polarities
                if ( m_erFlip[ s ][ c ] ) { er[ s ][ c ].setSample( indexThroughCurrentBuffer, v1[ c ] * -1.0f ); }
                else { er[ s ][ c ].setSample( indexThroughCurrentBuffer, v1[ c ] ); }
                // copy delayed sample from channel to temporary
                v1[ c ] =  er[ s ][ c ].getSampleRoundedIndex( indexThroughCurrentBuffer );
                v1[ c ] = erLPF[ s ][ c ].filterInput( v1[ c ] );
                
            }
            for (int c = 0; c < m_erChannels; c ++ )
            {
                // mix using hadamard
                m_sum = 0.0f;
                for ( int ch = 0; ch < m_erChannels; ch++ )
                {
                    m_sum += ( v1[ ch ] * m_hadamard[ c ][ ch ] );
                }
                v2[ c ] = m_sum;
            }
            for ( int c = 0; c < m_erChannels; c++ )
            {
                v1[ c ] = v2[ c ];
            }
        }
    }
    //==============================================================================
    void processLateReflections( int indexThroughCurrentBuffer )
    {
        // copy delayed samples into v2
        m_sum = 0.0f;
        for ( int c = 0; c < m_erChannels; c++ )
        {
            lr[c]. setDelayTime( lrDT[c] + ( lrDT[c] * lrMod[c].getSample( ) * lrModD[c] * m_modulation ) );
            
            v2[ c ] = lr[ c ].getSample( indexThroughCurrentBuffer );
            v2[ c ] = lrLPF[ c ].filterInput( v2[ c ] );
            m_sum += v2[ c ];
        }
        m_sum *= m_householderWeight;
        
        for ( int c = 0; c < m_erChannels; c++ )
        { v2[ c ] += m_sum; }
        
        // mixed delayed outputs are in v2
        for ( int c = 0; c < m_erChannels; c++ )
        {
            auto val = ( m_lrFB * v2[c] ) + v1[c];
            // copy last er sample to respective buffer
            lr[ c ].setSample( indexThroughCurrentBuffer, val );
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
    
    void genHadamard( )
    {
        m_hadamard[0][0] = 1.0f / sqrt( (float)m_erChannels ); // most simple matrix of size 1 is [1], whole matrix is multiplied by 1 / sqrt(size)
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
    
    // early reflections
    std::array< std::array< sjf_monoDelay, m_erChannels >, m_erStages >  er;
    std::array< std::array< float, m_erChannels >, m_erStages >  erDT;
    std::array< std::array< sjf_lpf, m_erChannels >, m_erStages > erLPF;
    
    // late reflections
    std::array< sjf_monoDelay, m_erChannels > lr;
    std::array< float,  m_erChannels > lrDT;
    std::array< sjf_lpf,  m_erChannels > lrLPF;
    std::array< sjf_osc,  m_erChannels > lrMod;
    std::array< float,  m_erChannels > lrModD;
    
    int m_SR = 44100, m_blockSize = 64;
    float m_erTotalLength = 300, m_lrTotalLength = 600, m_lrFB = 0.85;
    float m_dry = 1.0f, m_wet = 0.707f, m_modulation, m_size;
    float m_sum; // every little helps with cpu, I reuse this at multiple stages just to add and mix sample values
    float m_householderWeight = ( -2.0f / (float)m_erChannels );
    
    // implemented as arrays rather than vectors for cpu
    std::array< std::array<float, m_erChannels> , m_erChannels > m_hadamard;
    std::array< float,  m_erChannels > v1 , v2; // these will hold one sample for each channel
    std::array< std::array< bool,  m_erChannels >, m_erStages > m_erFlip;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_reverb )
};



#endif /* sjf_reverb_h */



