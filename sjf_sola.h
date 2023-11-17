//
//  sjf_sola_h
//
//  Created by Simon Fay on 28/11/2022.
//

#ifndef sjf_sola_h
#define sjf_sola_h

#include <JuceHeader.h>
#include "sjf_audioUtilities.h"
template< typename T >
class sjf_sola
{
public:
//    sjf_sola(){}
//    ~sjf_sola(){}
    
    static inline void stretch( juce::AudioBuffer< T > &sampleToStretch, juce::AudioBuffer< T > &stretchedSample, T stretchFactor, int N = 2048, int hopSize = 256, int fadeType = linear )
    {
        if ( sampleToStretch.getNumSamples() == 0 )
            return;
        auto nChannels = sampleToStretch.getNumChannels();
        auto originalSize = sampleToStretch.getNumSamples();
        auto stretchSize = std::ceil(originalSize * stretchFactor);
        
        // start by establishing parameters for SOLA algorithm
        // N --> block size
        // Sa --> samples between start of successive segments in original sample
        // Ss --> samples between start of successive segments in sola sample
        // L --> samples of overlap between successive segments in sola
        // M --> number of segments of original sample
        // norm --> normalisation factor for cross correlation calculation
        
        auto Sa = std::min( hopSize,  static_cast< int >( N / ( stretchFactor * 4 ) ) );
//        auto Sa = hopSize;
        auto M = std::ceil( static_cast< T >( originalSize ) / ( static_cast< T > ( Sa ) ) );
        // zero pad sample so last blocks don't run over edge
        auto newSize = ( M * Sa ) + N;
        sampleToStretch.setSize( sampleToStretch.getNumChannels(), newSize, true, true, true );
        
        auto Ss = std::ceil( Sa * stretchFactor );
        // slightly larger than necessary so we don't run into overruns
        stretchedSample.setSize( nChannels, Ss * M, true, true, true );
        
//        auto L = std::min( static_cast< int >( Ss / 2 ), static_cast< int >( Sa / 2 ) );
        auto L = static_cast< int >( Ss / 2 );
        auto norm = 1.0 / static_cast< T >( L );
        // create fade for between sections
        std::vector< T > fade;
        fade.reserve( L );
        switch ( fadeType ) {
            case linear:
                for ( auto i = 0; i < L; i++ )
                    fade.emplace_back( static_cast< T >( i ) / static_cast< T >( L ) );
                break;
            case hann :
                for ( auto i = 0; i < L; i++ )
                    fade.emplace_back( 0.5 - ( 0.5 * std::cos( 2.0 * M_PI * ( static_cast< T >( i ) / static_cast< T >( ( L - 1 ) * 2 ) ) ) ) );
                break;
            default:
                for ( auto i = 0; i < L; i++ )
                    fade.emplace_back( static_cast< T >( i ) / static_cast< T >( L ) );
                break;
        }
        
        
        // start by writing first block to stretchedBuffer
        // 0 --> Ss samples
        for ( auto c = 0; c < nChannels; c++ )
            stretchedSample.copyFrom( c, 0, sampleToStretch, c, 0, Ss );
        int solaIndex = Ss;
        int fadeIndexIn, fadeIndexOut ;
        T fadeIn, fadeOut, sampVal;
        for ( auto b = 1; b < M; b++ )
        {
            // calculate cross correlation between end of block A and beginning of block B
            //      Block A overlap start is (b-1)*Sa + Ss
            auto blockAIndex = (b-1)*Sa + Ss;
            //      block B start is b*Sa
            auto blockBIndex = b*Sa;
            auto timeLag = calculateCrossCorrelationTimeLag( sampleToStretch, blockAIndex, blockBIndex, L, norm );
            // then do fade out of block A as block B fades in
            //      and copy to stretched sample buffer
            auto crossOverLength = timeLag + L;
            for ( auto s = 0; s < crossOverLength; s++ )
            {
                fadeIndexIn = s - timeLag;
                fadeIndexOut = L - fadeIndexIn;
                fadeIn = fadeIndexIn < 0 ? 0 : fadeIndexIn < L ? fade[ fadeIndexIn ] : 1;
                fadeOut = fadeIndexOut < 0 ? 0 : fadeIndexOut < L ? fade[ fadeIndexOut ] : 1;
//                auto segAIndex = blockAOverlapStart + s;
//                auto segBIndex = blockBStart + s;
                for ( auto c = 0; c < nChannels; c++ )
                {
                    sampVal = fadeOut <= 0 ? 0 : sampleToStretch.getSample( c, blockAIndex ) * fadeOut;
                    sampVal += fadeIn <= 0 ? 0 : sampleToStretch.getSample( c, blockBIndex ) * fadeIn;
                    stretchedSample.setSample( c, solaIndex, sampVal );
                }
                blockAIndex++;
                blockBIndex++;
                solaIndex++;
            }
            // then copy (Ss-crossoverLength) samples from block B to stretched sample buffer
            // block B becomes block A for next loop
            for ( auto s = 0; s < Ss - crossOverLength; s++ )
            {
//                auto segBIndex = blockBStart + timeLag + L + s;
                for ( auto c = 0; c < nChannels; c++ )
                    stretchedSample.setSample( c, solaIndex, sampleToStretch.getSample( c, blockBIndex ) );
                solaIndex++;
                blockBIndex++;
            }
        }
        // resize samples
        sampleToStretch.setSize( sampleToStretch.getNumChannels(), originalSize, true, true, true );
        stretchedSample.setSize( nChannels, stretchSize, true, true, true );
    }
    
 
    enum fadeTypes
    { linear, hann };
    
private:
    
    static inline auto calculateCrossCorrelationTimeLag( juce::AudioBuffer< T > &sampleToStretch, int segStartA, int segStartB, int L, T norm )
    {
        auto nChannels = sampleToStretch.getNumChannels();

        auto maxIndex = 0;
        auto maxVal = -10000.0f; // default arbitrarily low value
        T newVal = 0;
        for ( auto i = 0; i < L; i++ )
        {
            newVal = 0;
            for ( auto c = 0; c < nChannels; c++ )
            {
                auto rpA = sampleToStretch.getReadPointer( c, segStartA );
                auto rpB = sampleToStretch.getReadPointer( c, segStartB+i );
                newVal += crossCorrelation( rpA, rpB, L-i ) * norm;
            }
            if ( newVal > maxVal )
            {
                maxIndex = i;
                maxVal = newVal;
            }
        }
        return maxIndex;
    }
    
    
    static inline auto crossCorrelation( const T* val1, const T* val2, const size_t nSteps, const T norm = 1 )
    {
        T sum = 0;
        for ( auto i = 0; i < nSteps; i++ )
            sum += val1[ i ] * val2[ i ];
        return sum * norm;
    }
    
    
};


#endif /* sjf_sola_h */

