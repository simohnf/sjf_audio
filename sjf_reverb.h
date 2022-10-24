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
#include "sjf_osc.h"
#include "sjf_monoPitchShift.h"
#include <algorithm>    // std::random_erShuffle
#include <random>       // std::default_random_engine
#include <vector>
#include <time.h>


//#define PI 3.14159265f


//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
class sjf_noiseOsc
{
    // random noise generator with frequency determining how fast oscillator moves to a new value
    // outputs values between -1 and 1
public:
    sjf_noiseOsc()
    {
        randomTargetValue();
    }
    ~sjf_noiseOsc(){}
    
    void initialise( int sampleRate )
    {
        if ( sampleRate > 0 ) { m_SR = sampleRate; }
        setFrequency( m_frequency );
    }
    
    void initialise( int sampleRate, float frequency )
    {
        if ( sampleRate > 0 ) { m_SR = sampleRate; }
        setFrequency( frequency );
    }
    

    
    void setFrequency( float frequency )
    {
        m_frequency = frequency;
        m_sampsPerCycle = m_SR / m_frequency;
        determineIncrement();
    }
    
    float getSample( )
    {
        m_lastOutput +=  m_increment;
        if ( m_lastOutput == m_target )
        {
            randomTargetValue();
            determineIncrement();
        }
        return m_lastOutput * 0.00001f;
    }
private:
    void randomTargetValue()
    {
        m_target = ( rand01() * 20000.0f ) - 10000.0f; // random values between -1 and 1
        
    }
    
    void determineIncrement()
    {
        auto dif = m_target - m_lastOutput; // difference between last output and targetValue
        m_increment = dif / m_sampsPerCycle;
    }
    float m_SR = 44100.0f, m_frequency = 1.0f, m_increment = 0.0f, m_target = 0.0f, m_lastOutput = 0.0f, m_sampsPerCycle = 44100.0f;
    
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_noiseOsc )
    
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
        
        m_sizeSmooth.setCutoff( 0.0005f );
        m_fbSmooth.setCutoff( 0.001f );
        m_modSmooth.setCutoff( 0.001f );
        m_wetSmooth.setCutoff( 0.001f );
        m_drySmooth.setCutoff( 0.001f );
        m_lrCutOffSmooth.setCutoff( 0.0001f );
        m_erCutOffSmooth.setCutoff( 0.0001f );
        randomiseDelayTimes( );
        randomPolarityFlips( );
        genHadamard( );
        randomiseModulators( );
        randomiseLPF( );
        initialiseDelayLines( );
        initialiseModulators( );
        setOutputMixer( );
    }
    //==============================================================================
    ~sjf_reverb() {}
    //==============================================================================
    void intialise( int sampleRate , int totalNumInputChannels, int totalNumOutputChannels, int samplesPerBlock)
    {
        if ( sampleRate > 0 ) { m_SR = sampleRate; }
        if ( samplesPerBlock > 0 ) { m_blockSize = samplesPerBlock; }
        initialiseDelayLines( );
        initialiseModulators( );
    }
    //==============================================================================
    void processAudio( juce::AudioBuffer<float> &buffer )
    {
        auto nInChannels = buffer.getNumChannels();
        auto bufferSize = buffer.getNumSamples();
        auto equalPowerGain = sqrt( 1.0f / nInChannels );
        auto gainFactor = 1.0f / ( (float)m_erChannels / (float)nInChannels );
        float erSizeFactor, lrSizeFactor, size, fbFactor, modFactor, lrCutOff, erCutOff;
        for ( int samp = 0; samp < bufferSize; samp++ )
        {
            modFactor = m_modSmooth.filterInput( m_modulationTarget );
            fbFactor = m_fbSmooth.filterInput( m_lrFBTarget );
            size = m_sizeSmooth.filterInput( m_sizeTarget );
            erSizeFactor = 0.6f + (size * 0.4f);
            lrSizeFactor = 0.4f + (size * 0.6f);
            lrCutOff = m_lrCutOffSmooth.filterInput( m_lrCutOffTarget );
            erCutOff = m_erCutOffSmooth.filterInput( m_erCutOffTarget );
            m_sum = 0.0f;
            // sum left and right and apply equal power gain
            for ( int inC = 0; inC < nInChannels; inC++ ) { m_sum += buffer.getSample( inC, samp ) * equalPowerGain; }
            // first copy input sample to temp buffer
            for ( int c = 0; c < m_erChannels; c ++ ){ v1[c] = m_sum; }
            // early reflections
            processEarlyReflections( samp, erSizeFactor, modFactor ); // last stage of er is in v1
            // late reflections
            filterEarlyReflections( erCutOff );
            processLateReflections( samp, lrSizeFactor, fbFactor, modFactor, lrCutOff );
            // mixed sum of lr is in v2
            processOutput( buffer, samp, nInChannels, gainFactor );
        }
        updateBuffers( bufferSize );
    }
    //==============================================================================
    void setSize ( float newSize )
    {
        m_sizeTarget = newSize * 0.01f;
    }
    //==============================================================================
    void setLrCutOff ( float newCutOff )
    {
        m_lrCutOffTarget = newCutOff;
    }
    //==============================================================================
    void setErCutOff ( float newCutOff )
    {
        m_erCutOffTarget = newCutOff;
    }
    //==============================================================================
    void setDecay ( float newDecay )
    {
        m_lrFBTarget = newDecay * 0.01f;
    }
    //==============================================================================
    void setModulation ( float newModulation )
    {
        m_modulationTarget = newModulation * 0.01f;
    }
    //==============================================================================
    void setMix ( float newMix )
    {
        newMix *= 0.01f;
        m_wetTarget = sqrt( newMix );
        m_dryTarget = sqrt( 1.0f - ( newMix ) );
    }
    //==============================================================================
    void setShimmer ( float newShimmerLevel, float newShimmerTransposition )
    {
        newShimmerLevel *= 0.01f;
        m_shimmerLevelTarget = pow( newShimmerLevel, 2.0f );
        m_shimmerTransposeTarget = pow( 2.0f, ( newShimmerTransposition / 12.0f ) );
    }
    //==============================================================================
    void setInterpolationType( int interpolationType )
    {
        for ( int c = 0; c < m_erChannels; c++ )
        {
            for ( int s = 0; s < m_erStages; s++ )
            {
                er[ s ][ c ].setInterpolationType( interpolationType );
            }
            lr[ c ].setInterpolationType( interpolationType );
        }
        for ( int shimV = 0; shimV < m_shimVoices; shimV++ )
        {
            shimmer[ shimV ].setInterpolationType( interpolationType );
        }
    }
    //==============================================================================
    void setFeedbackControl( bool feedbackControlFlag )
    {
        m_feedbackControlFlag = feedbackControlFlag;
    }
    
    //==============================================================================
    
private:
    void setOutputMixer( )
    {
        for ( int c = 0; c < m_erChannels; c++ )
        {
            float val = (float)( c + 1 ) / (float)( m_erChannels + 1 );
            outputMixer[ c ][ 0 ] = sqrt( val );
            outputMixer[ c ][ 1 ] = sqrt( 1.0f - val );
//            DBG( c << " " << outputMixer[ c ][ 0 ] << " " << outputMixer[ c ][ 1 ] );
        }
    }
    //==============================================================================
    void addShimmerInLR( int indexThroughCurrentBuffer )
    {
        // shimmer
        auto shimInLevel = (float)m_shimVoices/(float)m_erChannels;
        auto shimTranspose  = m_shimmerTransposeSmooth.filterInput( m_shimmerTransposeTarget );
        auto shimOutLevel = m_shimmerLevelSmooth.filterInput( m_shimmerLevelTarget );
        for ( int shimV = 0; shimV < m_shimVoices; shimV++ )
        {
            m_sum = 0.0f;
            for ( int c = 0; c < m_erChannels; c++ )
            {
                if ( c % 2 == shimV ) { m_sum += v2[ c ]; }
            }
            shimmer[ shimV ].setSample( indexThroughCurrentBuffer, m_sum * shimInLevel );
            auto shimOutput = shimmer[ shimV ].pitchShiftOutput( indexThroughCurrentBuffer, shimTranspose ) * shimOutLevel;
            for ( int c = 0; c < m_erChannels; c++ )
            {
                if ( c % 2 == shimV ) { v2[ c ] += shimOutput; }
            }
        }
        
        
    }
    //==============================================================================
    void randomiseDelayTimes()
    {
        // set each stage to be twice the length of the last
        float c = 0;
        for ( int s = 0; s < m_erStages; s++ ) { c += pow(2, s); }
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
        for ( int s = 0; s < m_erStages; s++ ) { std::random_shuffle ( std::begin( erDT[ s ] ), std::end( erDT[ s ] ) ); }
        float maxLenForERChannel = 0.0f;
        for (int c = 0; c < m_erChannels; c ++)
        {
            float sum = 0.0f;
            for ( int s = 0; s < m_erStages; s++ ) { sum += erDT[ s ][ c ]; }
            if ( sum >  maxLenForERChannel ) { maxLenForERChannel = sum; }
        }
        auto minLRtime = fmin( m_lrTotalLength * 0.5, maxLenForERChannel * 0.66);
        auto dtC = ( m_lrTotalLength - minLRtime ) / (float)m_erChannels;
        
        for (int c = 0; c < m_erChannels; c ++)
        {
            auto dt = rand01() * dtC;
            dt += (dtC * c) + minLRtime;
            lrDT[ c ] = dt;
        }
        std::random_shuffle ( std::begin( lrDT ), std::end( lrDT ) );
        
        std::array< float, m_erChannels > delayLineLengths;
        // initialise array...
        for ( int c = 0; c < m_erChannels; c++ ) { delayLineLengths[ c ] = 0.0f; }
        for ( int s = 0; s < m_erStages; s++ )
        {
            for ( int c = 0; c < m_erChannels; c++ ) { delayLineLengths[ c ] += erDT[ s ][ c ]; }
        }
        for ( int c = 0; c < m_erChannels; c++ ) { delayLineLengths[ c ] += lrDT[ c ]; }
        float longest = 0;
        for ( int c = 0; c < m_erChannels; c++ )
        {
            if ( delayLineLengths[ c ] > longest ) { longest = delayLineLengths[ c ]; }
        }
        
        float sum = 0;
        for ( int c = 0; c < m_erChannels; c++ )
        {
            sum += delayLineLengths[ c ];
            delayLineLengths[ c ] = 0;
        }
        auto avg = sum / (float) m_erChannels;
        auto scale = (m_maxTime / avg );
        for ( int s = 0; s < m_erStages; s++ )
        {
            for ( int c = 0; c < m_erChannels; c++ )
            {
                erDT[ s ][ c ] *= scale;
            }
        }
        for ( int c = 0; c < m_erChannels; c++ ) { lrDT[ c ] *= scale; }
        
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
            lrMod[ c ].setFrequency( rand01() * 0.001f );
            lrModD[ c ] = rand01() * 0.5f;
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
    void filterEarlyReflections( float erCutOff )
    {
        for ( int c = 0; c < m_erChannels; c++ )
        {
            erLPF[ c ].setCutoff( erCutOff );
            v1[ c ] = erLPF[ c ].filterInput( v1[ c ] );
        }
    }
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
        for ( int shimV = 0; shimV < m_shimVoices; shimV++ )
        {
            shimmer[ shimV ].updateBufferPositions( bufferSize );
        }
        
    }
    //==============================================================================
    void processOutput( juce::AudioBuffer<float> &buffer, int samp, int nInChannels, float gainFactor )
    {
//        DBG("why is this not working " );
        for ( int inc = 0; inc < nInChannels; inc ++ )
        {
            m_sum = 0.0f;
            for (int c = 0; c < m_erChannels; c++ )
            {
//                if ( c % nInChannels == inc )
//                {
//                    // mix alternating channels from early and late reflections
//                    m_sum += v1[ c ] + v2[ c ];
//                }
//                DBG( c << " " << inc << " " << outputMixer[ c ][ inc ] );
                m_sum += ( v1[ c ] + v2[ c ] ) * outputMixer[ c ][ inc ];
            }
            m_sum *= m_wetSmooth.filterInput( m_wetTarget ) * gainFactor;
            m_sum = tanh( m_sum ); // limit reverb output
            m_sum += buffer.getSample( inc, samp ) * m_drySmooth.filterInput( m_dryTarget );
            buffer.setSample( inc, samp, ( m_sum ) );
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
                dt += ( dt * erMod[ s ][ c ].getSample( ) * erModD[ s ][ c ] * modFactor );
                er[ s ][ c ].setDelayTime( dt );
                if ( m_erFlip[ s ][ c ] ) { v1[ c ] *= -1.0f ; } // flip some polarities
                // write previous to er vector
                er[ s ][ c ].setSample( indexThroughCurrentBuffer, v1[ c ] );
                // copy delayed sample from channel to temporary
                v1[ c ] =  er[ s ][ c ].getSample( indexThroughCurrentBuffer );
            }
            for (int c = 0; c < m_erChannels; c ++ )
            {
                // mix using hadamard
                m_sum = 0.0f;
                for ( int ch = 0; ch < m_erChannels; ch++ ) { m_sum += ( v1[ ch ] * m_hadamard[ c ][ ch ] ); }
                v2[ c ] = m_sum;
            }
            // copy v2 into v1 for next phase
            for ( int c = 0; c < m_erChannels; c++ ) { v1[ c ] = v2[ c ]; }
        }
    }
    //==============================================================================
    void processLateReflections( int indexThroughCurrentBuffer, float lrSizeFactor, float fbFactor, float modFactor, float lrCutOff )
    {
        // copy delayed samples into v2
        
        for ( int c = 0; c < m_erChannels; c++ )
        {
            auto dt = pow( lrDT[ c ], lrSizeFactor );
            dt += ( dt * lrMod[c].getSample( ) * lrModD[c] * modFactor );
            lr[c]. setDelayTime( dt );
            v2[ c ] = lr[ c ].getSample( indexThroughCurrentBuffer );
            lrLPF[ c ].setCutoff( lrCutOff );
            v2[ c ] = lrLPF[ c ].filterInput( v2[ c ] );
        }
        
        
        m_sum = 0.0f; // use this to mix all samples with householder matrix
        for( int c = 0; c < m_erChannels; c++ )
        {
            m_sum += v2[ c ];
        }
        m_sum *= m_householderWeight;
        for ( int c = 0; c < m_erChannels; c++ ) { v2[ c ] += m_sum; } // mixed delayed outputs are in v2
        addShimmerInLR( indexThroughCurrentBuffer );
        for ( int c = 0; c < m_erChannels; c++ )
        {
            auto val = ( fbFactor * v2[c] ) + v1[c];
            if ( m_feedbackControlFlag ) { val = tanh( val ); }
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
    void initialiseDelayLines()
    {
        for (int c = 0; c < m_erChannels; c ++)
        {
            for ( int s = 0; s < m_erStages; s++ )
            {
                er[ s ][ c ].initialise( m_SR , m_erTotalLength );
            }
            lr[ c ].initialise( m_SR , m_lrTotalLength );
        }
        for ( int shimV = 0; shimV < m_shimVoices; shimV++ )
        {
            shimmer[ shimV ].initialise( m_SR, 100.0f );
            shimmer[ shimV ].setDelayTimeSamps( 2 );
        }
    }
    //==============================================================================
    void initialiseModulators()
    {
        for (int c = 0; c < m_erChannels; c ++)
        {
            for ( int s = 0; s < m_erStages; s++ )
            {
                erMod[ s ][ c ].initialise( m_SR );
            }
            lrMod[ c ].initialise( m_SR );
        }
    }
    //==============================================================================
    const static int m_erChannels = 8; // must be a power of 2 because of hadamard matrix!!!!
    const static int m_erStages = 4;
    static constexpr float m_maxSize = 300;
    static constexpr float m_c = 344; // speed of sound
    static constexpr float m_maxTime = 1000.0f * m_maxSize / m_c;
    const static int m_shimVoices = 2;
    
    // early reflections
    std::array< std::array< sjf_monoDelay, m_erChannels >, m_erStages >  er;
    std::array< std::array< float, m_erChannels >, m_erStages >  erDT;
    std::array< sjf_lpf, m_erChannels > erLPF;
    std::array< std::array<  sjf_osc,  m_erChannels >, m_erStages > erMod;
    std::array< std::array<  float,  m_erChannels >, m_erStages > erModD;
    
    
    // late reflections
    std::array< sjf_monoDelay, m_erChannels > lr;
    std::array< float,  m_erChannels > lrDT;
    std::array< sjf_lpf,  m_erChannels > lrLPF;
    std::array< sjf_osc,  m_erChannels > lrMod;
    std::array< float,  m_erChannels > lrModD;
    
    // shimmer
    std::array< sjf_monoPitchShift, m_shimVoices > shimmer;
    
    int m_SR = 44100, m_blockSize = 64;
    float m_lrTotalLength = m_maxTime * 2.0f/3.0f;
    float m_erTotalLength = m_maxTime - m_lrTotalLength;
//    float m_erTotalLength = fmin( 150, m_maxTime * 1.0f/3.0f );
//    float m_lrTotalLength = m_maxTime - m_erTotalLength;;

    float m_modulationTarget, m_sizeTarget, m_dryTarget = 0.89443f, m_wetTarget = 0.44721f, m_lrFBTarget = 0.85f, m_lrCutOffTarget = 0.8f, m_erCutOffTarget = 0.8f, m_shimmerTransposeTarget = 2.0f, m_shimmerLevelTarget = 0.4f;
    sjf_lpf m_sizeSmooth, m_fbSmooth, m_modSmooth, m_wetSmooth, m_drySmooth, m_lrCutOffSmooth, m_erCutOffSmooth, m_shimmerTransposeSmooth, m_shimmerLevelSmooth;
    float m_sum; // every little helps with cpu, I reuse this at multiple stages just to add and mix sample values
    float m_householderWeight = ( -2.0f / (float)m_erChannels );
    
    bool m_feedbackControlFlag = false;
    
    // implemented as arrays rather than vectors for cpu
    std::array< std::array<float, m_erChannels> , m_erChannels > m_hadamard;
    std::array< float,  m_erChannels > v1 , v2; // these will hold one sample for each channel
    std::array< std::array< bool,  m_erChannels >, m_erStages > m_erFlip;
    
    std::array< std::array< float, 2 >,  m_erChannels > outputMixer; // stereo distribution
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_reverb )
};



#endif /* sjf_reverb_h */



