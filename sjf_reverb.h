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


//#define PI 3.14159265f


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
        
        m_sizeSmooth.setCutoff( 0.0001f );
        m_fbSmooth.setCutoff( 0.001f );
        m_modSmooth.setCutoff( 0.001f );
        m_wetSmooth.setCutoff( 0.001f );
        m_drySmooth.setCutoff( 0.001f );
        randomiseDelayTimes( );
        randomPolarityFlips( );
        genHadamard( );
        randomiseModulators( );
        randomiseLPF( );
        DBG( "max Time " << m_maxTime );
        
        
        for (int c = 0; c < m_erChannels; c ++)
        {
            for ( int s = 0; s < m_erStages; s++ )
            {
                er[ s ][ c ].initialise( m_SR , m_erTotalLength );
            }
            lr[ c ].initialise( m_SR , m_lrTotalLength );
        }
        
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
                er[ s ][ c ].initialise( m_SR , m_erTotalLength );
                erMod[ s ][ c ].initialise( m_SR );
            }
        }
        for (int c = 0; c < m_erChannels; c ++)
        {
            lr[ c ].initialise( m_SR , m_lrTotalLength );
            lrMod[ c ].initialise( m_SR );
        }
    }
    //==============================================================================
    void randomiseDelayTimes()
    {
        // set each stage to be twice the length of the last
        float c = 0;
        for ( int s = 0; s < m_erStages; s++ )
        { c += pow(2, s); }
        
        float frac = 1.0f / c; // fraction of total length for first stage
        for ( int s = 0; s < m_erStages; s++ )
        {
            auto dtC = m_erTotalLength * frac * pow(2, s) / (float)m_erChannels;
            for (int c = 0; c < m_erChannels; c ++)
            {
                float minER = 1.0f + rand01();
                // space each channel so that the are randomly spaced but with roughly even distribution
                auto dt = fmax( rand01() *  dtC, minER );
                dt += ( dtC * c );
                erDT[ s ][ c ] = dt;
            }
            
        }
        // shuffle er channels
        for ( int s = 0; s < m_erStages; s++ )
        { std::random_shuffle ( std::begin( erDT[ s ] ), std::end( erDT[ s ] ) ); }
        
        float maxLenForERChannel = 0.0f;
        for (int c = 0; c < m_erChannels; c ++)
        {
            float sum = 0.0f;
            for ( int s = 0; s < m_erStages; s++ )
            { sum += erDT[ s ][ c ]; }
//            DBG("er sum " << c << " " << sum );
            if ( sum >  maxLenForERChannel )
            { maxLenForERChannel = sum; }
        }
        
//        DBG( " m_lrTotalLength " << m_lrTotalLength << " maxLenForERChannel " << maxLenForERChannel );
        auto minLRtime = fmin( m_lrTotalLength * 0.5, maxLenForERChannel * 0.66);
        auto dtC = ( m_lrTotalLength - minLRtime ) / (float)m_erChannels;
//        DBG( "minLRtime " << minLRtime << " dtC " << dtC );
        
        for (int c = 0; c < m_erChannels; c ++)
        {
            auto dt = rand01() * dtC;
            dt += (dtC * c) + minLRtime;
            lrDT[ c ] = dt;
//            DBG( lrDT[ c ] );
        }
        std::random_shuffle ( std::begin( lrDT ), std::end( lrDT ) );
        
        std::array< float, m_erChannels > delayLineLengths;
        // initialise array...
        for ( int c = 0; c < m_erChannels; c++ ) { delayLineLengths[ c ] = 0.0f; }
        for ( int s = 0; s < m_erStages; s++ )
        {
            for ( int c = 0; c < m_erChannels; c++ ) { delayLineLengths[ c ] += erDT[ s ][ c ]; }
        }
        for ( int c = 0; c < m_erChannels; c++ )
        {
            delayLineLengths[ c ] += lrDT[ c ];
//            DBG( c << " " << delayLineLengths[ c ] );
        }
        float longest = 0;
        for ( int c = 0; c < m_erChannels; c++ )
        {
            if ( delayLineLengths[ c ] > longest ) { longest = delayLineLengths[ c ]; }
//            DBG( c << " " << delayLineLengths[ c ] );
        }
        
        float sum = 0;
        for ( int c = 0; c < m_erChannels; c++ )
        {
            sum += delayLineLengths[ c ];
            delayLineLengths[ c ] = 0;
        }
//        DBG("sum!!!! " << sum);
        auto avg = sum / (float) m_erChannels;
        
//        auto scale = (m_maxTime / longest);
        auto scale = (m_maxTime / avg );
        for ( int s = 0; s < m_erStages; s++ )
        {
            for ( int c = 0; c < m_erChannels; c++ )
            {
                erDT[ s ][ c ] *= scale;
            }
        }
        for ( int c = 0; c < m_erChannels; c++ )
        {
            lrDT[ c ] *= scale;
        }
        
    }
    //==============================================================================
    void randomiseModulators()
    {
        for ( int s = 0; s < m_erStages; s++ )
        {
            for (int c = 0; c < m_erChannels; c ++)
            {
                erMod[ s ][ c ].setFrequency( rand01() * 0.05f );
                erModD[ s ][ c ] = rand01() * 0.4f;
            }
        }
        
        
        for (int c = 0; c < m_erChannels; c ++)
        {
            lrMod[c].setFrequency( rand01() * 0.05f );
            lrModD[c] = rand01() * 0.4f;
        }
    }
    //==============================================================================
    void randomiseLPF()
    {
        for (int c = 0; c < m_erChannels; c ++)
        {
            lrLPF[c].setCutoff( sqrt( rand01() ) );
        }
    }
    //==============================================================================
    void processAudio( juce::AudioBuffer<float> &buffer )
    {
        auto nInChannels = buffer.getNumChannels();
        auto bufferSize = buffer.getNumSamples();
        auto equalPowerGain = sqrt( 1.0f / nInChannels );
        auto gainFactor = 1.0f / ( (float)m_erChannels / (float)nInChannels );
        
        float erSizeFactor, lrSizeFactor, size, fbFactor, modFactor;
        for ( int samp = 0; samp < bufferSize; samp++ )
        {
            modFactor = m_modSmooth.filterInput( m_modulationTarget );
            fbFactor = m_fbSmooth.filterInput( m_lrFB );
            size = m_sizeSmooth.filterInput( m_sizeTarget );
            erSizeFactor = 0.6f + (size * 0.4f);
            lrSizeFactor = 0.4f + (size * 0.6f);
            m_sum = 0.0f;
            // sum left and right and apply equal power gain
            for ( int inC = 0; inC < nInChannels; inC++ ) { m_sum += buffer.getSample( inC, samp ) * equalPowerGain; }
            // first copy input sample to temp buffer
            for ( int c = 0; c < m_erChannels; c ++ ){ v1[c] = m_sum; }
            // early reflections
            processEarlyReflections( samp, erSizeFactor, modFactor );
            // last stage of er is in v1
            // late reflections
            processLateReflections( samp, lrSizeFactor, fbFactor, modFactor );
            // mixed sum of lr is in v2
            
            processOutput( buffer, samp, nInChannels, gainFactor );

//            DBG("");
        }
        updateBuffers( bufferSize );
//        DBG("");
    }
    //==============================================================================
    void setSize ( float newSize )
    {
        m_sizeTarget = newSize * 0.01f;
    }
    //==============================================================================
    void setDecay ( float newDecay )
    {
//        DBG( "newDecay " << newDecay );
        m_lrFB = newDecay * 0.01f;
    }
    //==============================================================================
    void setModulation ( float newModulation )
    {
//        DBG( "newModulation " << newModulation );
        m_modulationTarget = newModulation * 0.01f;
    }
    //==============================================================================
    
    void setMix ( float newMix )
    {
        newMix *= 0.01f;
        //        DBG( "newModulation " << newModulation );
//        m_wet = ( sin( ( 0.5 * newMix ) * PI )  );
//        DBG("wet " << m_wet);
//        m_dry = ( sin( (0.5 + (0.5 * newMix) ) * PI ) );
//        DBG("dry " << m_dry);
        m_wet = sqrt( newMix );
        m_dry = sqrt( 1.0f - ( newMix ) );
    }
    //==============================================================================
    
private:
    //==============================================================================
    
    void updateBuffers( int bufferSize )
    {
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
    void processOutput( juce::AudioBuffer<float> &buffer, int samp, int nInChannels, float gainFactor )
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
            m_sum *= m_wetSmooth.filterInput( m_wet ) * gainFactor;
            m_sum = tanh( m_sum ); // limit reverb output
            m_sum += buffer.getSample( c, samp ) * m_drySmooth.filterInput( m_dry );
            buffer.setSample( c, samp, ( m_sum ) );
        }
    }
    //==============================================================================
    void processEarlyReflections( int indexThroughCurrentBuffer, float erSizeFactor, float modFactor )
    {
        // count through stage of er
        for ( int s = 0; s < m_erStages; s++ )
        {
            for ( int c = 0; c < m_erChannels; c++ )
            {
                // for each channel
                auto dt = pow( erDT[ s ][ c ], erSizeFactor );
                er[ s ][ c ].setDelayTime( dt + ( dt * erMod[ s ][ c ].getSample( ) * erModD[ s ][ c ] * modFactor ) );
//                DBG( s << " " << c << " " << er[ s ][ c ]. getDelayTimeMS() );
                // flip some polarities
                if ( m_erFlip[ s ][ c ] ) { v1[ c ] *= -1.0f ; }
                // write previous to er vector
                er[ s ][ c ].setSample( indexThroughCurrentBuffer, v1[ c ] );
                // copy delayed sample from channel to temporary
                v1[ c ] =  er[ s ][ c ].getSample( indexThroughCurrentBuffer );
//                v1[ c ] = erLPF[ s ][ c ].filterInput( v1[ c ] );
                
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
    void processLateReflections( int indexThroughCurrentBuffer, float lrSizeFactor, float fbFactor, float modFactor )
    {
        // copy delayed samples into v2
        m_sum = 0.0f; // use this to mix all samples with householder matrix
        for ( int c = 0; c < m_erChannels; c++ )
        {
            auto dt = pow( lrDT[ c ], lrSizeFactor );
            lr[c]. setDelayTime( dt + ( dt * lrMod[c].getSample( ) * lrModD[c] * modFactor ) );
//            DBG( c << " " << lr[ c ]. getDelayTimeMS() );
            v2[ c ] = lr[ c ].getSample( indexThroughCurrentBuffer );
            v2[ c ] = lrLPF[ c ].filterInput( v2[ c ] );
            m_sum += v2[ c ];
        }
        m_sum *= m_householderWeight;
        
        for ( int c = 0; c < m_erChannels; c++ ) { v2[ c ] += m_sum; }
        
        // mixed delayed outputs are in v2
        for ( int c = 0; c < m_erChannels; c++ )
        {
            auto val = ( fbFactor * v2[c] ) + v1[c];
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
    static constexpr float m_maxSize = 300;
    static constexpr float m_c = 344; // speed of sound
    static constexpr float m_maxTime = 1000.0f * m_maxSize / m_c;
    
    
    // early reflections
    std::array< std::array< sjf_monoDelay, m_erChannels >, m_erStages >  er;
    std::array< std::array< float, m_erChannels >, m_erStages >  erDT;
//    std::array< std::array< sjf_lpf, m_erChannels >, m_erStages > erLPF;
    std::array< std::array<  sjf_osc,  m_erChannels >, m_erStages > erMod;
    std::array< std::array<  float,  m_erChannels >, m_erStages > erModD;
    
    
    // late reflections
    std::array< sjf_monoDelay, m_erChannels > lr;
    std::array< float,  m_erChannels > lrDT;
    std::array< sjf_lpf,  m_erChannels > lrLPF;
    std::array< sjf_osc,  m_erChannels > lrMod;
    std::array< float,  m_erChannels > lrModD;
    
    int m_SR = 44100, m_blockSize = 64;
    float m_lrTotalLength = m_maxTime * 2.0f/3.0f;
    float m_erTotalLength = m_maxTime - m_lrTotalLength;
//    float m_erTotalLength = fmin( 150, m_maxTime * 1.0f/3.0f );
//    float m_lrTotalLength = m_maxTime - m_erTotalLength;;

    float m_lrFB = 0.85;
    float m_dry = 1.0f, m_wet = 0.707f, m_modulationTarget, m_sizeTarget;
    sjf_lpf m_sizeSmooth, m_fbSmooth, m_modSmooth, m_wetSmooth, m_drySmooth;
    float m_sum; // every little helps with cpu, I reuse this at multiple stages just to add and mix sample values
    float m_householderWeight = ( -2.0f / (float)m_erChannels );
    
    
    
    // implemented as arrays rather than vectors for cpu
    std::array< std::array<float, m_erChannels> , m_erChannels > m_hadamard;
    std::array< float,  m_erChannels > v1 , v2; // these will hold one sample for each channel
    std::array< std::array< bool,  m_erChannels >, m_erStages > m_erFlip;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_reverb )
};



#endif /* sjf_reverb_h */



