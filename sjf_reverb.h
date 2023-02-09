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
#include "sjf_randOSC.h"
//#include "sjf_monoPitchShift.h"
#include "sjf_pitchShift.h"
#include <algorithm>    // std::random_erShuffle
#include <random>       // std::default_random_engine
#include <vector>
#include <time.h>

//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================

template < typename T, size_t NUM_REV_CHANNELS, size_t NUM_ER_STAGES >
class sjf_reverb
{
private:
//    const static int NUM_REV_CHANNELS = 8; // must be a power of 2 because of hadamard matrix!!!!
//    const static int NUM_ER_STAGES = 4;
//    static constexpr T m_maxSize = 150;
//    static constexpr T m_c = 344; // speed of sound
//    static constexpr T m_maxTime = 1000.0f * m_maxSize / m_c;
//    T m_lrTotalLength = m_maxTime * 1.0f/3.0f;
//    T m_erTotalLength = m_maxTime - m_lrTotalLength;
    static constexpr T m_lrTotalLength = 200;
    static constexpr T m_erTotalLength = 300;
    
    
    // early reflections
    std::array< sjf_multiDelay< T, NUM_REV_CHANNELS >, NUM_ER_STAGES > er;
    std::array< std::array< T, NUM_REV_CHANNELS >, NUM_ER_STAGES >  erDT;
    std::array< sjf_lpf< T >, NUM_REV_CHANNELS > erLPF;
//    std::array< std::array<  sjf_randOSC< T >,  NUM_REV_CHANNELS >, NUM_ER_STAGES > erMod;
    std::array< std::array<  T,  NUM_REV_CHANNELS >, NUM_ER_STAGES > erModD;
    
    
    // late reflections
    sjf_multiDelay< T, NUM_REV_CHANNELS > lr;
    std::array< T,  NUM_REV_CHANNELS > lrDT;
    std::array< sjf_lpf< T >,  NUM_REV_CHANNELS > lrLPF;
    std::array< sjf_randOSC< T >,  NUM_REV_CHANNELS > lrMod;
    std::array< T,  NUM_REV_CHANNELS > lrModD;
    
    // shimmer
    sjf_pitchShift< T > shimmer;
    T m_lastSummedOutput = 0;
    
    int m_SR = 44100; //, m_blockSize = 64;

    
    T m_modulationTarget, m_sizeTarget, m_dryTarget = 0.89443f, m_wetTarget = 0.44721f, m_lrFBTarget = 0.85f, m_lrCutOffTarget = 0.8f, m_erCutOffTarget = 0.8f, m_shimmerTransposeTarget = 2.0f, m_shimmerLevelTarget = 0.4f;
    sjf_lpf< T > m_sizeSmooth, m_fbSmooth, m_modSmooth, m_wetSmooth, m_drySmooth, m_lrCutOffSmooth, m_erCutOffSmooth, m_shimmerTransposeSmooth, m_shimmerLevelSmooth;
    
    
    bool m_feedbackControlFlag = false;
    
    // implemented as arrays rather than vectors for cpu
    std::array< std::array< T,  NUM_REV_CHANNELS >, NUM_ER_STAGES > m_erFlip;
    
    
    
public:
    //==============================================================================
    sjf_reverb()
    {
        srand((unsigned)time(NULL));
        
        m_sizeSmooth.setCutoff( 0.001f );
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
        initialiseDelayLines( );
        initialiseModulators( );
//        setOutputMixer( );
    }
    //==============================================================================
    ~sjf_reverb() {}
    //==============================================================================
    void initialise( int sampleRate , int totalNumInputChannels, int totalNumOutputChannels, int samplesPerBlock)
    {
        if ( sampleRate > 0 ) { m_SR = sampleRate; }
//        if ( samplesPerBlock > 0 ) { m_blockSize = samplesPerBlock; }
        initialiseDelayLines( );
        initialiseModulators( );
        // reset shimmer output values
        m_lastSummedOutput = 0;
    }
    //==============================================================================
    void processAudio( juce::AudioBuffer<T> &buffer )
    {
        auto nInChannels = buffer.getNumChannels();
        auto bufferSize = buffer.getNumSamples();
        const T equalPowerGain = sqrt( 1.0f / nInChannels );
//        auto gainFactor = 1.0f / ( (T)NUM_REV_CHANNELS / (T)nInChannels );
        T erSizeFactor, lrSizeFactor, size, fbFactor, modFactor, lrCutOff, erCutOff, wet, dry;
        std::array< T, NUM_REV_CHANNELS > erData, lrData, outMix;
        erData.fill ( 0 );
        lrData.fill( 0 );
        outMix.fill( 0 );
        static constexpr T shimLevelFactor = 1.0f / (T)NUM_REV_CHANNELS;
        const T hadmardScale = std::sqrt( 1.0f / (T)NUM_REV_CHANNELS );
        const T dtScale = ( m_SR * 0.001f );
        
        for ( int indexThroughCurrentBuffer = 0; indexThroughCurrentBuffer < bufferSize; ++indexThroughCurrentBuffer )
        {
            // first set up control variables

            modFactor = m_modSmooth.filterInput( m_modulationTarget );
            fbFactor = m_fbSmooth.filterInput( m_lrFBTarget );
            size = m_sizeSmooth.filterInput( m_sizeTarget );
            erSizeFactor = ( 0.3f + (size * 0.7f) ) * dtScale;
            lrSizeFactor = ( 0.1f + (size * 0.9f) ) * dtScale;
            lrCutOff = m_lrCutOffSmooth.filterInput( m_lrCutOffTarget );
            erCutOff = m_erCutOffSmooth.filterInput( m_erCutOffTarget );
            wet = m_wetSmooth.filterInput( m_wetTarget );
            dry = m_drySmooth.filterInput( m_dryTarget );
            
            T sum = 0.0f; // reset sum

            // sum left and right and apply equal power gain
            for ( int inC = 0; inC < nInChannels; inC++ )
            {
                sum += buffer.getSample( inC, indexThroughCurrentBuffer );
            }
            sum *= equalPowerGain;
            sum += processShimmer( indexThroughCurrentBuffer, m_lastSummedOutput);
            erData.fill( sum );

            // early reflections
            processEarlyReflections( indexThroughCurrentBuffer, erSizeFactor, modFactor, hadmardScale, erData, lrData ); // last stage of er is in erData
            filterEarlyReflections( erCutOff, erData ); // filter early reflection taps
            processLateReflections( indexThroughCurrentBuffer, lrSizeFactor, fbFactor, modFactor, lrCutOff, erData, lrData );
            
            m_lastSummedOutput = 0;
            for ( int c = 0; c < NUM_REV_CHANNELS; c++ )
            {
                outMix[ c ]  = erData[ c ] + lrData[ c ];
//                outMix[ c ] *= equalPowerGain;
                m_lastSummedOutput += lrData[ c ]; // ER in shimmer ????
            }
            m_lastSummedOutput *= shimLevelFactor;
            Hadamard< T, NUM_REV_CHANNELS >::inPlace( outMix.data(), hadmardScale ); // one last Mix of everything to get output
//            m_lastSummedOutput = outMix[ 0 ];

            for ( int c = 0; c < nInChannels; c ++ )
            {
                sum = outMix[ c ] * wet; // WET
                sum += buffer.getSample( c, indexThroughCurrentBuffer ) * dry; // DRY
                buffer.setSample( c, indexThroughCurrentBuffer, ( sum ) );
            }
        }
//        DBG( "erSizeFactor " << erSizeFactor );
//        updateBuffers( bufferSize );
    }
    //==============================================================================
    void setSize ( const T &newSize )
    {
        m_sizeTarget = pow(newSize * 0.01f, 2.0f);
//        DBG( "m_sizeTarget " << m_sizeTarget );
    }
    //==============================================================================
    void setLrCutOff ( const T &newCutOff )
    {
        m_lrCutOffTarget = newCutOff;
    }
    //==============================================================================
    void setErCutOff ( const T &newCutOff )
    {
        m_erCutOffTarget = newCutOff;
    }
    //==============================================================================
    void setDecay ( const T &newDecay )
    {
        m_lrFBTarget = newDecay * 0.01f;
    }
    //==============================================================================
    void setModulationDepth ( const T &newModulation )
    {
        m_modulationTarget = newModulation * 0.005f; // +/- 50% of max delayTime
    }
    //==============================================================================
    void setModulationRate( const T& newModRate )
    {
        
    }
    //==============================================================================
    void setMix ( const T &newMix )
    {
//        newMix *= 0.01f;
        m_wetTarget = sqrt( newMix * 0.01 );
        m_dryTarget = sqrt( 1.0f - ( newMix * 0.01 ) );
    }
    //==============================================================================
    void setShimmerLevel ( const T &newShimmerLevel )
    {
//        newShimmerLevel *= 0.01f;
        m_shimmerLevelTarget = pow( newShimmerLevel * 0.01, 2.0f );
    }

    void setShimmerTransposition( const T &newShimmerTransposition )
    {
        m_shimTranspose = pow( 2.0f, ( newShimmerTransposition / 12.0f ) );
    }
    //==============================================================================
    void setInterpolationType( const int &interpolationType )
    {
//        DBG("interpolation changed");
        lr.setInterpolationType( interpolationType );
        for ( int s = 0; s < NUM_ER_STAGES; s++ )
        {
            er[ s ].setInterpolationType( interpolationType );
        }
        shimmer.setInterpolationType( interpolationType );
    }
    //==============================================================================
    void setFeedbackControl( const bool &feedbackControlFlag )
    {
        m_feedbackControlFlag = feedbackControlFlag;
    }
    
    //==============================================================================
    
private:
//    void setOutputMixer( )
//    {
//        for ( int c = 0; c < NUM_REV_CHANNELS; c++ )
//        {
//            T val = (T)( c + 1 ) / (T)( NUM_REV_CHANNELS + 1 );
//            outputMixer[ c ][ 0 ] = sqrt( val );
//            outputMixer[ c ][ 1 ] = sqrt( 1.0f - val );
//        }
//        std::random_shuffle( std::begin( outputMixer ), std::end( outputMixer ) );
//    }
    //==============================================================================
    T processShimmer( const int &indexThroughCurrentBuffer, const T &inVal )
    {
        // shimmer
        shimmer.setSample( indexThroughCurrentBuffer, inVal );
        return shimmer.pitchShiftOutput( indexThroughCurrentBuffer, m_shimmerTransposeSmooth.filterInput( m_shimmerTransposeTarget ) ) * m_shimmerLevelSmooth.filterInput( m_shimmerLevelTarget );
    }
    
    //==============================================================================

    void randomiseDelayTimes()
    {
//        m_erTotalLength = m_maxTime - m_lrTotalLength
        // first set early reflection times
        T c = 0;
        for ( int s = 0; s < NUM_ER_STAGES; s++ )
        { c += pow(2, s); }
        T frac = 1.0f / c; // fraction of total length for first stage
//        m_erTotalLength = 300;
        DBG( " m_erTotalLength " << m_erTotalLength );
        for ( int s = 0; s < NUM_ER_STAGES; s++ )
        {
//            auto erMax = m_erTotalLength * frac * pow(2, s);
            DBG( s << " erMax " << m_erTotalLength * frac * pow(2, s) );
            auto dtC = m_erTotalLength * frac * pow(2, s) / (T)NUM_REV_CHANNELS;
            for (int c = 0; c < NUM_REV_CHANNELS; c ++)
            {
                T minER = 1.0f + rand01();
                // space each channel so that the are randomly spaced but with roughly even distribution
                auto dt = fmax( rand01() *  dtC, minER );
                dt += ( dtC * c );
                erDT[ s ][ c ] = dt;
            }
        }
        
        auto minLRTime = m_lrTotalLength * 0.5;
//        auto dtC = ( m_lrTotalLength - minLRtime ) / (T)NUM_REV_CHANNELS;
//
//        auto minLRTime = 100;
//        auto maxLRTime = 200;
        for (int c = 0; c < NUM_REV_CHANNELS; c ++)
        {
//            auto dt = rand01() * dtC;
//            dt +=  minLRtime;
            lrDT[ c ] = minLRTime + ( rand01() * ( m_lrTotalLength - minLRTime ) );
        }

        DBG( "er len " << m_erTotalLength << " lr len " << m_lrTotalLength );
        for ( int s = 0; s < NUM_ER_STAGES; s++ )
        {
            for ( int c = 0; c < NUM_REV_CHANNELS; c++ )
            {
                DBG( "early reflection " << s << " " << c << " - " << erDT[ s ][ c ] );
            }
        }
        for ( int c = 0; c < NUM_REV_CHANNELS; c++ )
        {
            DBG( "late reflection " << c << " - " << lrDT[ c ] );
        }

    }
//    void randomiseDelayTimes()
//    {
//        // set each stage to be twice the length of the last
//        T c = 0;
//        for ( int s = 0; s < NUM_ER_STAGES; s++ ) { c += pow(2, s); }
//        T frac = 1.0f / c; // fraction of total length for first stage
//        for ( int s = 0; s < NUM_ER_STAGES; s++ )
//        {
//            auto dtC = m_erTotalLength * frac * pow(2, s) / (T)NUM_REV_CHANNELS;
//            for (int c = 0; c < NUM_REV_CHANNELS; c ++)
//            {
//                T minER = 1.0f + rand01();
//                // space each channel so that the are randomly spaced but with roughly even distribution
//                auto dt = fmax( rand01() *  dtC, minER );
//                dt += ( dtC * c );
//                erDT[ s ][ c ] = dt;
//            }
//        }
//        // shuffle er channels
//        for ( int s = 0; s < NUM_ER_STAGES; s++ )
//        { std::random_shuffle ( std::begin( erDT[ s ] ), std::end( erDT[ s ] ) ); }
//        T maxLenForERChannel = 0.0f;
//        for (int c = 0; c < NUM_REV_CHANNELS; c ++)
//        {
//            T sum = 0.0f;
//            for ( int s = 0; s < NUM_ER_STAGES; s++ ) { sum += erDT[ s ][ c ]; }
//            if ( sum >  maxLenForERChannel ) { maxLenForERChannel = sum; }
//        }
//        auto minLRtime = fmin( m_lrTotalLength * 0.5, maxLenForERChannel * 0.66);
//        auto dtC = ( m_lrTotalLength - minLRtime ) / (T)NUM_REV_CHANNELS;
//
//        for (int c = 0; c < NUM_REV_CHANNELS; c ++)
//        {
//            auto dt = rand01() * dtC;
//            dt += (dtC * c) + minLRtime;
//            lrDT[ c ] = dt;
//        }
//        std::random_shuffle ( std::begin( lrDT ), std::end( lrDT ) );
//
//        std::array< T, NUM_REV_CHANNELS > delayLineLengths;
//        // initialise array...
//        for ( int c = 0; c < NUM_REV_CHANNELS; c++ )
//        {
//            delayLineLengths[ c ] = 0.0f;
//        }
//        for ( int s = 0; s < NUM_ER_STAGES; s++ )
//        {
//            for ( int c = 0; c < NUM_REV_CHANNELS; c++ )
//            {
//                delayLineLengths[ c ] += erDT[ s ][ c ];
//            }
//        }
//        for ( int c = 0; c < NUM_REV_CHANNELS; c++ )
//        {
//            delayLineLengths[ c ] += lrDT[ c ];
//        }
//        T longest = 0;
//        for ( int c = 0; c < NUM_REV_CHANNELS; c++ )
//        {
//            if ( delayLineLengths[ c ] > longest )
//            {
//                longest = delayLineLengths[ c ];
//            }
//        }
//
//        T sum = 0.0f;
//        for ( int c = 0; c < NUM_REV_CHANNELS; c++ )
//        {
//            sum += delayLineLengths[ c ];
//            delayLineLengths[ c ] = 0;
//        }
//        auto avg = sum / (T) NUM_REV_CHANNELS;
//        auto scale = ( m_maxTime / avg );
//        for ( int s = 0; s < NUM_ER_STAGES; s++ )
//        {
//            for ( int c = 0; c < NUM_REV_CHANNELS; c++ )
//            {
//                erDT[ s ][ c ] *= scale;
//            }
//        }
//        for ( int c = 0; c < NUM_REV_CHANNELS; c++ )
//        {
//            lrDT[ c ] *= scale;
//        }
//
//        for ( int s = 0; s < NUM_ER_STAGES; s++ )
//        {
//            for ( int c = 0; c < NUM_REV_CHANNELS; c++ )
//            {
//                DBG( "early reflection " << s << " " << c << " - " << erDT[ s ][ c ] );
//            }
//        }
//        for ( int c = 0; c < NUM_REV_CHANNELS; c++ )
//        {
//            DBG( "late reflection " << c << " - " << lrDT[ c ] );
//        }
//
//    }
    //==============================================================================
    void randomiseModulators()
    {
//        for ( int s = 0; s < NUM_ER_STAGES; s++ )
//        {
//            for (int c = 0; c < NUM_REV_CHANNELS; c ++)
//            {
//                erMod[ s ][ c ].setFrequency( rand01() * 0.5f );
//            }
//        }
        for (int c = 0; c < NUM_REV_CHANNELS; c ++)
        {
            lrMod[ c ].setFrequency( 0.001f );
//            lrMod[ c ].setFrequency( rand01() * 0.1f );
        }
    }
    //==============================================================================
    void randomiseLPF()
    {
        for (int c = 0; c < NUM_REV_CHANNELS; c ++)
        {
            lrLPF[c].setCutoff( sqrt( rand01() ) );
        }
    }
    //==============================================================================
    void filterEarlyReflections( const T& erCutOff, std::array< T, NUM_REV_CHANNELS >& erData )
    {
        for ( int c = 0; c < NUM_REV_CHANNELS; c++ )
        {
            erLPF[ c ].setCutoff( erCutOff );
            erData[ c ] = erLPF[ c ].filterInput( erData[ c ] );
        }
    }
    //==============================================================================
//    void updateBuffers( const int &bufferSize )
//    {
//        lr.updateBufferPositions( bufferSize );
//
//        for ( int s = 0; s < NUM_ER_STAGES; s ++ )
//        {
//            er[s].updateBufferPositions( bufferSize );
//        }
//
//        shimmer.updateBufferPosition( bufferSize );
//    }
    //==============================================================================
//    void processOutputAndShimmer( juce::AudioBuffer<T> &buffer, const int &indexThroughCurrentBuffer, const int &nInChannels, const T &gainFactor, std::array< T, NUM_REV_CHANNELS >& erData, std::array< T, NUM_REV_CHANNELS > lrData )
//    {
//        m_lastSummedOutput = 0.0f;
//        for ( int inC = 0; inC < nInChannels; inC ++ )
//        {
//            T sum = 0.0f;
//            for (int c = 0; c < NUM_REV_CHANNELS; c++ )
//            {
//                sum += ( erData[ c ] + lrData[ c ] ) * outputMixer[ c ][ inC ];
//            }
//            m_lastSummedOutput += sum;
//            sum *= m_wetSmooth.filterInput( m_wetTarget ) * gainFactor;
//
//            sum += buffer.getSample( inC, indexThroughCurrentBuffer ) * m_drySmooth.filterInput( m_dryTarget );
//            buffer.setSample( inC, indexThroughCurrentBuffer, ( sum ) );
//        }
//    }
    
    //==============================================================================
    void processEarlyReflections( const int &indexThroughCurrentBuffer, const T &erSizeFactor, const T &modFactor, const T &hadmardScale, std::array< T, NUM_REV_CHANNELS > &erData, std::array< T, NUM_REV_CHANNELS > &lrData )
    {
        T dt;
        std::array< T, NUM_REV_CHANNELS > temp;
        temp.fill( 0 );
        static constexpr T erScale = 1.0f / NUM_ER_STAGES;
        // count through stage of er
        for ( int s = 0; s < NUM_ER_STAGES; s++ )
        {
            // set delayTimes
            for ( int c = 0; c < NUM_REV_CHANNELS; c++ )
            {
                // for each channel
                dt = erDT[ s ][ c ] * erSizeFactor;
//                if ( c == 0 || c == 1 ) // only modulate 2 rev channels
//                {
//                    dt += ( dt * erMod[ s ][ c ].output( ) * modFactor );
//                }
                er[ s ].setDelayTimeSamps( c, dt );
            }
            // flip some polarities
            for ( int c = 0; c < NUM_REV_CHANNELS; c++ )
            {
                erData[ c ] *= m_erFlip[ s ][ c ];
            }
            // write old samples to delay line and get delayed samples
            er[ s ].processAudioInPlaceRoundedIndex( indexThroughCurrentBuffer, erData );
            Hadamard<T, NUM_REV_CHANNELS>::inPlace( erData.data(), hadmardScale );
            for ( int c = 0; c < NUM_REV_CHANNELS; c++ )
            {
                temp[ c ] += erData[ c ]; // accumulate early reflections in temporary buffer --> erTaps
            }
        }
        lrData = erData;
        erData = temp;
        for ( int c = 0; c < NUM_REV_CHANNELS; c++ )
        {
            erData[ c ] *= erScale;
        }
    }
    //==============================================================================
    void processLateReflections( const int &indexThroughCurrentBuffer, const T &lrSizeFactor, const T &fbFactor, const T &modFactor, const T &lrCutOff, std::array< T, NUM_REV_CHANNELS >& erData, std::array< T, NUM_REV_CHANNELS >& lrData )
    {
        T dt;
//        std::array< T, NUM_REV_CHANNELS > temp;
        // copy delayed samples into v2
        for ( int c = 0; c < NUM_REV_CHANNELS; c++ )
        {
            dt = lrDT[ c ] * lrSizeFactor;
//            if ( c == 0 || c == 1 )
//            {
////                dt += ( dt * lrMod[c].getSample( ) * lrModD[c] * modFactor );
////                dt += ( dt * lrMod[c].output( ) * lrModD[c] * modFactor );
//                dt += ( dt * lrMod[c].output( ) * modFactor );
//            }
            dt += ( dt * lrMod[c].output( ) * modFactor );
            lr.setDelayTimeSamps( c, dt );
        }

//        for ( int c = 0; c < NUM_REV_CHANNELS; c++ )
//        {
//            if ( c == 0 || c == 1 )
//            {
//                lrData[ c ] = lr.getSample( c, indexThroughCurrentBuffer );
//            }
//            else
//            {
//                lrData[ c ] = lr.getSampleRoundedIndex( c, indexThroughCurrentBuffer );
//            }
//        }
        lr.popSamplesOutOfDelayLine( indexThroughCurrentBuffer, lrData );
//        lr.popSamplesOutOfDelayLineRoundedIndex( indexThroughCurrentBuffer, lrData );
        for ( int c = 0; c < NUM_REV_CHANNELS; c++ )
        {
            lrLPF[ c ].setCutoff( lrCutOff );
            lrData[ c ] = lrLPF[ c ].filterInput( lrData[ c ] );
        }
        
        Householder< T, NUM_REV_CHANNELS >::mixInPlace( lrData );
        
        if ( m_feedbackControlFlag )
        {
            for ( int c = 0; c < NUM_REV_CHANNELS; c++ )
            {
                lr.setSample( c, indexThroughCurrentBuffer, tanh( ( fbFactor * lrData[c] ) + erData[c] ) );
            }
        }
        else
        {
            for ( int c = 0; c < NUM_REV_CHANNELS; c++ )
            {
                lr.setSample( c, indexThroughCurrentBuffer, ( fbFactor * lrData[c] ) + erData[c] );
            }
        }
//        lr.pushSamplesIntoDelayLine( indexThroughCurrentBuffer, temp );
    }
    //==============================================================================
    void randomPolarityFlips()
    {
        for ( int s = 0; s < NUM_ER_STAGES; s++ )
        {
            for ( int c = 0; c < NUM_REV_CHANNELS; c ++ )
            {
                // randomise some polarities
                auto rn = rand01();
                if ( rn >= 0.5 ) { m_erFlip[s][c] = -1.0f; }
                else { m_erFlip[s][c] = 1.0f; }
            }
        }
    }
    //==============================================================================
    void initialiseDelayLines()
    {
        for ( int s = 0; s < NUM_ER_STAGES; s++ )
        {
            er[ s ].initialise( m_SR , m_erTotalLength * 1.5 );
        }
        lr.initialise( m_SR , m_lrTotalLength * 1.5 );
        
        shimmer.initialise( m_SR, 100.0f );
        shimmer.setDelayTimeSamps( 2 );
    }
    //==============================================================================
    void initialiseModulators()
    {
        for (int c = 0; c < NUM_REV_CHANNELS; c ++)
        {
//            for ( int s = 0; s < NUM_ER_STAGES; s++ )
//            {
//                erMod[ s ][ c ].initialise( m_SR );
//            }
            lrMod[ c ].initialise( m_SR );
        }
    }
    //==============================================================================
//    void setFiltersToFirstOrder()
//    {
//        for ( int i = 0; i < NUM_REV_CHANNELS; i++ )
//        {
//            erLPF[ i ].isFirstOrder( true );
//            lrLPF[ i ].isFirstOrder( true );
//        }
//    }
    //==============================================================================
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_reverb )
};



#endif /* sjf_reverb_h */



