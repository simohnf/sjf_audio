//
//  sjf_reverb2.h
//
//  Created by Simon Fay on 07/10/2022.
//


#ifndef sjf_reverb2_h
#define sjf_reverb2_h
#include <JuceHeader.h>
#include "sjf_audioUtilities.h"
//#include "sjf_monoDelay.h"
#include "sjf_delayLine.h"
#include "sjf_lpf.h"
#include "sjf_osc.h"
#include "sjf_monoPitchShift.h"
#include <algorithm>    // std::random_erShuffle
#include <random>       // std::default_random_engine
#include <vector>
#include <time.h>

//==============================================================================
//==============================================================================
// Use like `Hadamard<double, 8>::inPlace(data)` - size must be a power of 2
template< typename Sample, int size >
class Hadamard
{
public:
    static inline void recursiveUnscaled(Sample * data) {
        if (size <= 1) return;
        constexpr int hSize = size/2;
        
        // Two (unscaled) Hadamards of half the size
        Hadamard<Sample, hSize>::recursiveUnscaled(data);
        Hadamard<Sample, hSize>::recursiveUnscaled(data + hSize);
        
        // Combine the two halves using sum/difference
        for (int i = 0; i < hSize; ++i) {
            double a = data[i];
            double b = data[i + hSize];
            data[i] = (a + b);
            data[i + hSize] = (a - b);
        }
    }
    
    static inline void inPlace(Sample * data) {
        recursiveUnscaled(data);
        
        Sample scalingFactor = std::sqrt(1.0/size);
        for (int c = 0; c < size; ++c) {
            data[c] *= scalingFactor;
        }
    }
};

template< class floatType, int size >
class Householder
{
    static constexpr floatType m_householderWeight = ( -2.0f / (float)size );
public:
    static inline void mixInPlace( std::array< floatType, size >& data )
    {
        floatType sum = 0.0f; // use this to mix all samples with householder matrix
        for( int c = 0; c < size; c++ )
        {
            sum += data[ c ];
        }
        sum *= m_householderWeight;
        for ( int c = 0; c < size; c++ )
        {
            data[ c ] += sum;
        }
    }
};
//inline void mixWithHouseholderInPlace( std::array< float, m_revChannels >& data )
//{
//    float sum = 0.0f; // use this to mix all samples with householder matrix
//    for( int c = 0; c < m_revChannels; c++ )
//    {
//        sum += data[ c ];
//    }
//    sum *= m_householderWeight;
//    for ( int c = 0; c < m_revChannels; c++ )
//    {
//        data[ c ] += sum;
//    } // mixed delayed outputs are in v2
//}
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================

class sjf_reverb
{
private:
    const static int m_revChannels = 8; // must be a power of 2 because of hadamard matrix!!!!
    const static int m_erStages = 4;
    static constexpr float m_maxSize = 150;
    static constexpr float m_c = 344; // speed of sound
    static constexpr float m_maxTime = 1000.0f * m_maxSize / m_c;
    float m_lrTotalLength = m_maxTime * 1.0f/3.0f;
    float m_erTotalLength = m_maxTime - m_lrTotalLength;
    
    
    // early reflections
    std::array< sjf_multiDelay< float, m_revChannels >, m_erStages > er;
    std::array< std::array< float, m_revChannels >, m_erStages >  erDT;
    std::array< sjf_lpf, m_revChannels > erLPF;
    std::array< std::array<  sjf_osc,  m_revChannels >, m_erStages > erMod;
    std::array< std::array<  float,  m_revChannels >, m_erStages > erModD;
    
    
    // late reflections
    sjf_multiDelay< float, m_revChannels > lr;
    std::array< float,  m_revChannels > lrDT;
    std::array< sjf_lpf,  m_revChannels > lrLPF;
    std::array< sjf_osc,  m_revChannels > lrMod;
    std::array< float,  m_revChannels > lrModD;
    
    // shimmer
    sjf_monoPitchShift shimmer;
    float m_lastSummedOutput = 0;
    
    int m_SR = 44100, m_blockSize = 64;

    
    float m_modulationTarget, m_sizeTarget, m_dryTarget = 0.89443f, m_wetTarget = 0.44721f, m_lrFBTarget = 0.85f, m_lrCutOffTarget = 0.8f, m_erCutOffTarget = 0.8f, m_shimmerTransposeTarget = 2.0f, m_shimmerLevelTarget = 0.4f;
    sjf_lpf m_sizeSmooth, m_fbSmooth, m_modSmooth, m_wetSmooth, m_drySmooth, m_lrCutOffSmooth, m_erCutOffSmooth, m_shimmerTransposeSmooth, m_shimmerLevelSmooth;
    
    
    bool m_feedbackControlFlag = false;
    
    // implemented as arrays rather than vectors for cpu
    std::array< std::array<float, m_revChannels> , m_revChannels > m_hadamard;
    std::array< std::array< bool,  m_revChannels >, m_erStages > m_erFlip;
    
    
    std::array< std::array< float, 2 >,  m_revChannels > outputMixer; // stereo distribution
    
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
        m_lrCutOffSmooth.setCutoff( 0.0001f );
        m_erCutOffSmooth.setCutoff( 0.0001f );
        randomiseDelayTimes( );
        randomPolarityFlips( );
        randomiseModulators( );
        randomiseLPF( );
        setFiltersToFirstOrder( );
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
        // reset shimmer output values
        m_lastSummedOutput = 0;
    }
    //==============================================================================
    void processAudio( juce::AudioBuffer<float> &buffer )
    {
        auto nInChannels = buffer.getNumChannels();
        auto bufferSize = buffer.getNumSamples();
        auto equalPowerGain = sqrt( 1.0f / nInChannels );
        auto gainFactor = 1.0f / ( (float)m_revChannels / (float)nInChannels );
        float erSizeFactor, lrSizeFactor, size, fbFactor, modFactor, lrCutOff, erCutOff;
        std::array< float, m_revChannels > erData, lrData;
        erData.fill ( 0 );
        lrData.fill( 0 );
        static constexpr auto shimLevelFactor = 1.0f / (float)m_revChannels;
        
        float dtScale = ( m_SR * 0.001f );
        
        for ( int indexThroughCurrentBuffer = 0; indexThroughCurrentBuffer < bufferSize; ++indexThroughCurrentBuffer )
        {
            modFactor = m_modSmooth.filterInput( m_modulationTarget );
            fbFactor = m_fbSmooth.filterInput( m_lrFBTarget );
            size = m_sizeSmooth.filterInput( m_sizeTarget );
            erSizeFactor = ( 0.3f + (size * 0.7f) ) * dtScale;
            lrSizeFactor = ( 0.1f + (size * 0.9f) ) * dtScale;
            lrCutOff = m_lrCutOffSmooth.filterInput( m_lrCutOffTarget );
            erCutOff = m_erCutOffSmooth.filterInput( m_erCutOffTarget );
            float sum = 0.0f; // reset sum
            // sum left and right and apply equal power gain
            for ( int inC = 0; inC < nInChannels; inC++ )
            {
                sum += buffer.getSample( inC, indexThroughCurrentBuffer );
            }
            sum *= equalPowerGain;
            sum += processShimmer( indexThroughCurrentBuffer, m_lastSummedOutput * shimLevelFactor );
            erData.fill( sum );
            // early reflections
            processEarlyReflections( indexThroughCurrentBuffer, erSizeFactor, modFactor, erData, lrData ); // last stage of er is in erData
            filterEarlyReflections( erCutOff, erData ); // filter early reflection taps
            processLateReflections( indexThroughCurrentBuffer, lrSizeFactor, fbFactor, modFactor, lrCutOff, erData, lrData );
            processOutputAndShimmer( buffer, indexThroughCurrentBuffer, nInChannels, gainFactor, erData, lrData );
        }
//        DBG( "erSizeFactor " << erSizeFactor );
        updateBuffers( bufferSize );
    }
    //==============================================================================
    void setSize ( float newSize )
    {
        m_sizeTarget = pow(newSize * 0.01f, 2.0f);
//        DBG( "m_sizeTarget " << m_sizeTarget );
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
//        DBG("interpolation changed");
        lr.setInterpolationType( interpolationType );
        for ( int s = 0; s < m_erStages; s++ )
        {
            er[ s ].setInterpolationType( interpolationType );
        }
        shimmer.setInterpolationType( interpolationType );
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
        for ( int c = 0; c < m_revChannels; c++ )
        {
            float val = (float)( c + 1 ) / (float)( m_revChannels + 1 );
            outputMixer[ c ][ 0 ] = sqrt( val );
            outputMixer[ c ][ 1 ] = sqrt( 1.0f - val );
        }
        std::random_shuffle( std::begin( outputMixer ), std::end( outputMixer ) );
    }
    //==============================================================================
    float processShimmer( const int &indexThroughCurrentBuffer, const float &inVal )
    {
        // shimmer
//        auto shimTranspose  = m_shimmerTransposeSmooth.filterInput( m_shimmerTransposeTarget ); // set shimmer transposition
//        auto shimOutLevel = m_shimmerLevelSmooth.filterInput( m_shimmerLevelTarget );// * shimLevFactor; // set output level using interface
        shimmer.setSample( indexThroughCurrentBuffer, inVal );
        return shimmer.pitchShiftOutput( indexThroughCurrentBuffer, m_shimmerTransposeSmooth.filterInput( m_shimmerTransposeTarget ) ) * m_shimmerLevelSmooth.filterInput( m_shimmerLevelTarget );
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
            auto dtC = m_erTotalLength * frac * pow(2, s) / (float)m_revChannels;
            for (int c = 0; c < m_revChannels; c ++)
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
        for (int c = 0; c < m_revChannels; c ++)
        {
            float sum = 0.0f;
            for ( int s = 0; s < m_erStages; s++ ) { sum += erDT[ s ][ c ]; }
            if ( sum >  maxLenForERChannel ) { maxLenForERChannel = sum; }
        }
        auto minLRtime = fmin( m_lrTotalLength * 0.5, maxLenForERChannel * 0.66);
        auto dtC = ( m_lrTotalLength - minLRtime ) / (float)m_revChannels;
        
        for (int c = 0; c < m_revChannels; c ++)
        {
            auto dt = rand01() * dtC;
            dt += (dtC * c) + minLRtime;
            lrDT[ c ] = dt;
//            DBG("lrDT["<<c<<"] "<<lrDT[c]);
        }
//        DBG("m_lrTotalLength " << m_lrTotalLength);
        std::random_shuffle ( std::begin( lrDT ), std::end( lrDT ) );
        
        std::array< float, m_revChannels > delayLineLengths;
        // initialise array...
        for ( int c = 0; c < m_revChannels; c++ )
        {
            delayLineLengths[ c ] = 0.0f;
        }
        for ( int s = 0; s < m_erStages; s++ )
        {
            for ( int c = 0; c < m_revChannels; c++ )
            {
                delayLineLengths[ c ] += erDT[ s ][ c ];
            }
        }
        for ( int c = 0; c < m_revChannels; c++ )
        {
            delayLineLengths[ c ] += lrDT[ c ];
        }
        float longest = 0;
        for ( int c = 0; c < m_revChannels; c++ )
        {
            if ( delayLineLengths[ c ] > longest )
            {
                longest = delayLineLengths[ c ];
            }
        }
        
        float sum = 0.0f;
        for ( int c = 0; c < m_revChannels; c++ )
        {
            sum += delayLineLengths[ c ];
            delayLineLengths[ c ] = 0;
        }
        auto avg = sum / (float) m_revChannels;
        auto scale = ( m_maxTime / avg );
        for ( int s = 0; s < m_erStages; s++ )
        {
            for ( int c = 0; c < m_revChannels; c++ )
            {
                erDT[ s ][ c ] *= scale;
            }
        }
        for ( int c = 0; c < m_revChannels; c++ )
        {
            lrDT[ c ] *= scale;
        }
        
        for ( int s = 0; s < m_erStages; s++ )
        {
            for ( int c = 0; c < m_revChannels; c++ )
            {
                DBG( "early reflection " << s << " " << c << " - " << erDT[ s ][ c ] );
            }
        }
        for ( int c = 0; c < m_revChannels; c++ )
        {
            DBG( "late reflection " << c << " - " << lrDT[ c ] );
        }
        
    }
    //==============================================================================
    void randomiseModulators()
    {
        for ( int s = 0; s < m_erStages; s++ )
        {
            for (int c = 0; c < m_revChannels; c ++)
            {
                erMod[ s ][ c ].setFrequency( rand01() * 0.5f );
                erModD[ s ][ c ] = 0.25f + rand01() * 0.4f;
            }
        }
        for (int c = 0; c < m_revChannels; c ++)
        {
            lrMod[ c ].setFrequency( rand01() * 0.001f );
//            lrModD[ c ] = 0.25f + rand01() * 0.5f;
            lrModD[ c ] = rand01();
        }
    }
    //==============================================================================
    void randomiseLPF()
    {
        for (int c = 0; c < m_revChannels; c ++)
        {
            lrLPF[c].setCutoff( sqrt( rand01() ) );
        }
    }
    //==============================================================================
    void filterEarlyReflections( const float& erCutOff, std::array< float, m_revChannels >& erData )
    {
        for ( int c = 0; c < m_revChannels; c++ )
        {
            erLPF[ c ].setCutoff( erCutOff );
            erData[ c ] = erLPF[ c ].filterInput( erData[ c ] );
        }
    }
    //==============================================================================
    void updateBuffers( int bufferSize )
    {
        lr.updateBufferPositions( bufferSize );

        for ( int s = 0; s < m_erStages; s ++ )
        {
            er[s].updateBufferPositions( bufferSize );
        }

        shimmer.updateBufferPositions( bufferSize );
    }
    //==============================================================================
    void processOutputAndShimmer( juce::AudioBuffer<float> &buffer, const int &indexThroughCurrentBuffer, const int &nInChannels, const float &gainFactor, std::array< float, m_revChannels >& erData, std::array< float, m_revChannels > lrData )
    {
        m_lastSummedOutput = 0.0f;
        for ( int inC = 0; inC < nInChannels; inC ++ )
        {
            float sum = 0.0f;
            for (int c = 0; c < m_revChannels; c++ )
            {
                sum += ( erData[ c ] + lrData[ c ] ) * outputMixer[ c ][ inC ];
            }
//            sum = tanh( sum ); // limit reverb output
//            m_lastSummedOutput[ inC ] = processShimmer( indexThroughCurrentBuffer, inC, shimTranspose, sum, shimInLevel, shimOutLevel );
            m_lastSummedOutput += sum;
            sum *= m_wetSmooth.filterInput( m_wetTarget ) * gainFactor;
            
            sum += buffer.getSample( inC, indexThroughCurrentBuffer ) * m_drySmooth.filterInput( m_dryTarget );
            buffer.setSample( inC, indexThroughCurrentBuffer, ( sum ) );
        }
    }
    //==============================================================================
    void processEarlyReflections( const int &indexThroughCurrentBuffer, const float &erSizeFactor, const float &modFactor, std::array< float, m_revChannels >& erData, std::array< float, m_revChannels >& lrData )
    {
        float dt;
//        float dtScale = ( m_SR * 0.001f );
        std::array< float, m_revChannels > temp;
        temp.fill( 0 );
        static constexpr float erScale = 1.0f / m_erStages;
        // count through stage of er
        for ( int s = 0; s < m_erStages; s++ )
        {
            // set delayTimes
            for ( int c = 0; c < m_revChannels; c++ )
            {
                // for each channel
                dt = erDT[ s ][ c ] * erSizeFactor;
//                dt *= dtScale;
                if ( c == 0 || c == 1 ) // only modulate 2 rev channels
                {
                    dt += ( dt * erMod[ s ][ c ].getSampleCalculated( ) * erModD[ s ][ c ] * modFactor );
                }
                er[ s ].setDelayTimeSamps( c, dt );
            }
            // flip some polarities
            for ( int c = 0; c < m_revChannels; c++ )
            { if ( m_erFlip[ s ][ c ] ) { erData[ c ] *= -1.0f ; } }
            // write old samples to delay line and get delayed samples
            er[ s ].processAudioInPlaceRoundedIndex( indexThroughCurrentBuffer, erData );
            Hadamard<float, m_revChannels>::inPlace( erData.data() );
            for ( int c = 0; c < m_revChannels; c++ )
            {
                temp[ c ] += erData[ c ]; // accumulate early reflections in temporary buffer --> erTaps
            }
        }
        lrData = erData;
        erData = temp;
        for ( int c = 0; c < m_revChannels; c++ )
        {
            erData[ c ] *= erScale;
        }
    }
    //==============================================================================
    void processLateReflections( const int &indexThroughCurrentBuffer, const float &lrSizeFactor, const float &fbFactor, const float &modFactor, const float &lrCutOff, std::array< float, m_revChannels >& erData, std::array< float, m_revChannels >& lrData )
    {
        float dt;
//        float dtScale = ( m_SR * 0.001f );
        // copy delayed samples into v2
        for ( int c = 0; c < m_revChannels; c++ )
        {
//            dt = pow( lrDT[ c ], lrSizeFactor );
            dt = lrDT[ c ] * lrSizeFactor;
//            dt *= dtScale;
            if ( c == 0 || c == 1 )
            {
                dt += ( dt * lrMod[c].getSample( ) * lrModD[c] * modFactor );
            }
            lr.setDelayTimeSamps( c, dt );
        }

        for ( int c = 0; c < m_revChannels; c++ )
        {
            if ( c == 0 || c == 1 )
            {
                lrData[ c ] = lr.getSample( c, indexThroughCurrentBuffer );
            }
            else
            {
                lrData[ c ] = lr.getSampleRoundedIndex( c, indexThroughCurrentBuffer );
            }
        }
//        lr.popSamplesOutOfDelayLine( indexThroughCurrentBuffer, lrData );
//        lr.popSamplesOutOfDelayLineRoundedIndex( indexThroughCurrentBuffer, lrData );
        for ( int c = 0; c < m_revChannels; c++ )
        {
            lrLPF[ c ].setCutoff( lrCutOff );
            lrData[ c ] = lrLPF[ c ].filterInput( lrData[ c ] );
        }
        
        Householder< float, m_revChannels >::mixInPlace( lrData );
        
//        mixWithHouseholderInPlace( lrData );
//        std::array< float, m_revChannels > temp;
//        for ( int c = 0; c < m_revChannels; c++ )
//        {
//            auto val = ( fbFactor * lrData[c] ) + erData[c];
//            if ( m_feedbackControlFlag ) { val = tanh( val ); }
//            // copy last er sample to respective buffer
//            lr.setSample( c, indexThroughCurrentBuffer, val );
//        }
        if ( m_feedbackControlFlag )
        {
            for ( int c = 0; c < m_revChannels; c++ )
            {
//                temp[ c ] = tanh( ( fbFactor * lrData[c] ) + erData[c] );
                lr.setSample( c, indexThroughCurrentBuffer, tanh( ( fbFactor * lrData[c] ) + erData[c] ) );
            }
        }
        else
        {
            for ( int c = 0; c < m_revChannels; c++ )
            {
//                temp[ c ] = ( fbFactor * lrData[c] ) + erData[c];
                lr.setSample( c, indexThroughCurrentBuffer, ( fbFactor * lrData[c] ) + erData[c] );
            }
        }
//        lr.pushSamplesIntoDelayLine( indexThroughCurrentBuffer, temp );
    }
    //==============================================================================
    void randomPolarityFlips()
    {
        for ( int s = 0; s < m_erStages; s++ )
        {
            for ( int c = 0; c < m_revChannels; c ++ )
            {
                // randomise some polarities
                auto rn = rand01();
                if ( rn >= 0.5 ) { m_erFlip[s][c] = true; }
                else { m_erFlip[s][c] = false; }
            }
        }
    }
    //==============================================================================
    void initialiseDelayLines()
    {
        for ( int s = 0; s < m_erStages; s++ )
        {
            er[ s ].initialise( m_SR , m_erTotalLength );
        }
        lr.initialise( m_SR , m_lrTotalLength );
        
        shimmer.initialise( m_SR, 100.0f );
        shimmer.setDelayTimeSamps( 2 );
    }
    //==============================================================================
    void initialiseModulators()
    {
        for (int c = 0; c < m_revChannels; c ++)
        {
            for ( int s = 0; s < m_erStages; s++ )
            {
                erMod[ s ][ c ].initialise( m_SR );
            }
            lrMod[ c ].initialise( m_SR );
        }
    }
    //==============================================================================
    void setFiltersToFirstOrder()
    {
        for ( int i = 0; i < m_revChannels; i++ )
        {
            erLPF[ i ].isFirstOrder( true );
            lrLPF[ i ].isFirstOrder( true );
        }
    }
    //==============================================================================
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_reverb )
};



#endif /* sjf_reverb_h */



