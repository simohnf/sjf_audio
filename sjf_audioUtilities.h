//
//  sjf_audioUtilities.h
//
//  Created by Simon Fay on 25/08/2022.
//

#ifndef sjf_audioUtilities_h
#define sjf_audioUtilities_h

#include <JuceHeader.h>
#include "sjf_audioUtilitiesC++.h"
#include "sjf_compileTimeRandom.h"
//==============================================================================
//==============================================================================
// applies amplitude envelope of breakPoints to juce::AudioBuffer
// envelope should be in the form std::vector< std::array< float, 2 > > where:
//              env[ 0 ][ 0 ] is sample position normalised to 0(start)-->1(end)
//              env[ 0 ][ 1 ] is amplitude
inline void applyEnvelopeToBuffer( juce::AudioBuffer< float >& buffer, std::vector< std::array < float, 2 > > env )
{
    auto nSamps = buffer.getNumSamples();
    auto nPoints = env.size();
//    std::vector< std::array< float, 2 > > env = envelope;
    for ( int i = 0; i < nPoints; i++ )
    {
        env[ i ][ 0 ] *= nSamps - 1; // scale normalised position to sample values
    }
    
    for ( int i = 0; i < nPoints - 1; i++ )
    {
        DBG( env[ i ][ 0 ] << " " << env[ i ][ 1 ] );
        auto startSamp = env[ i ][ 0 ];
        auto startGain = env[ i ][ 1 ];
        auto endSamp = env[ i + 1 ][ 0 ];
        auto endGain = env[ i + 1 ][ 1 ];
        auto rampLen = endSamp - startSamp;
        rampLen = rampLen > 0 ? rampLen : 1;
        if ( startSamp + rampLen < nSamps )
        {
            buffer.applyGainRamp( startSamp, rampLen, startGain, endGain);
        }
    }
    DBG( env[ nPoints - 1 ][ 0 ] << " " << env[ nPoints - 1 ][ 1 ] );
}



template< typename T, int NUM_ROWS, int NUM_COLUMNS >
struct sjf_matrixOfRandomFloats
{
    constexpr sjf_matrixOfRandomFloats( ) : floatArray()
    {
        int count = 1;
        
        for ( int r = 0; r < NUM_ROWS; r++ )
        {
            for (int c = 0; c < NUM_COLUMNS; c ++)
            {
//                floatArray[ r ][ c ] = r*c;
                floatArray[ r ][ c ] = random0to1( count );
                count ++;
            }
        }
    }
    
    const T& getValue ( const int& index1, const int& index2 )const { return floatArray[ index1 ][ index2 ]; }
private:
    //----------------------------------------
    constexpr T random0to1( int count )
    {
        return static_cast<T>( sjf_compileTimeRandom::get_random( count ) ) / static_cast<T>(RAND_MAX);
    }
    //----------------------------------------
    T floatArray[ NUM_ROWS ][ NUM_COLUMNS ];
};

namespace sjf::juceStuff
{
    /** gets unnormalised value from juce parameter */
    template < typename T, typename parameterType >
    T getUnNormalisedParameterValue( parameterType* p )
    {
        return static_cast< juce::RangedAudioParameter* >( p )->convertFrom0to1( p->getValue() );
    }
}
#endif /* sjf_audioUtilities_h */
