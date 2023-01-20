//
//  sjf_audioUtilities.h
//
//  Created by Simon Fay on 25/08/2022.
//

#ifndef sjf_audioUtilities_h
#define sjf_audioUtilities_h

#include <JuceHeader.h>


#define PI 3.14159265
//==============================================================================
// A simple phase ramp based envelope
inline
float phaseEnv( float phase, float period, float envLen){
    auto nSegments = period / envLen;
    auto segmentPhase = phase * nSegments;
    auto rampUp = segmentPhase;
    if (rampUp > 1) {rampUp = 1;}
    else if (rampUp < 0) {rampUp = 0;}
    
    float rampDown = segmentPhase - (nSegments - 1);
    if (rampDown > 1) {rampDown = 1;}
    else if (rampDown < 0) {rampDown = 0;}
    rampDown *= -1;
    //    return rampUp+rampDown; // this would give linear fade
    return sin( PI* (rampUp+rampDown)/2 ); // this gives a smooth sinewave based fade
}

//==============================================================================
// simple stereo panning based on 1/4 of a sine wave cycle
inline
float pan2( float pan, int channel){
    if (channel < 0) { channel = 0 ; }
    if (channel > 1) { channel = 1 ; }
    if (pan < 0) { pan = 0; }
    if (pan >= 1) { pan = 1; }
    pan *= 0.5;
    if (channel == 0)
    {
        pan += -1.0f;
        pan += 0.5;
    }
    
    return sin( PI* pan ); // this gives a smooth sinewave based fade
}

//==============================================================================
// simple output of random numbers between 0 and 1 --> requires initialisation with srand
inline
float rand01()
{
    return float( rand() ) / float( RAND_MAX );
}


//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
// generate patterns of random combinations 1 and 2 beats
// with a random initial offset (so that patterns do not always start with a true value
// pattern of bools
inline std::vector<bool> onesAndTwos( int nBeatsToGenerate )
{
    std::vector< bool > output(nBeatsToGenerate, false);
    int beatsLeft = nBeatsToGenerate;
    int index =  rand()%3;
    if ( index != 0 ){ index += 1; }
    while ( beatsLeft > 0 )
    {
        output[index] = true;
        auto i = 1 + rand()%2;
        index += i;
        beatsLeft -= i;
        index %= nBeatsToGenerate;
    }
    return output;
}
//==============================================================================
// generate patterns of random combinations 2 and 3 beats
// with a random initial offset (so that patterns do not always start with a true value
// pattern of bools
inline std::vector<bool> twosAndThrees( int nBeatsToGenerate )
{
    std::vector< bool > output(nBeatsToGenerate, false);
    //    output.resize(nBeatsToGenerate);
    //
    int beatsLeft = nBeatsToGenerate;
    int index =  rand()%3;
    if ( index != 0 ){ index += 1; }
    while ( beatsLeft > 0 )
    {
        output[index] = true;
        if (beatsLeft == 4)
        {
            index += 2;
            beatsLeft -= 2;
        }
        else if (beatsLeft == 3)
        {
            index += 3;
            beatsLeft -= 3;
        }
        else if (beatsLeft == 2)
        {
            index += 2;
            beatsLeft -= 2;
        }
        else
        {
            auto i = 2 + rand()%2;
            index += i;
            beatsLeft -= i;
        }
        index %= nBeatsToGenerate;
    }
    return output;
}
//==============================================================================
// generate patterns of random combinations 3 and 4 beats
// with a random initial offset (so that patterns do not always start with a true value
// pattern of bools
// not as robust as creation of 2s and 3s
inline std::vector<bool> threesAndFours( int nBeatsToGenerate )
{
    std::vector< bool > output(nBeatsToGenerate, false);
    int beatsLeft = nBeatsToGenerate;
    int index =  rand()%3;
    if ( index != 0 ){ index += 2; }
    while ( beatsLeft > 0 )
    {
        output[index] = true;
        if (beatsLeft == 8)
        {
            index += 4;
            beatsLeft -= 4;
        }
        else if (beatsLeft == 6)
        {
            index += 3;
            beatsLeft -= 3;
        }
        else if (beatsLeft == 4)
        {
            index += 4;
            beatsLeft -= 4;
        }
        else if (beatsLeft == 3)
        {
            index += 3;
            beatsLeft -= 3;
        }
        else
        {
            auto i = 3 + rand()%2;
            index += i;
            beatsLeft -= i;
        }
        index %= nBeatsToGenerate;
    }
    return output;
}

//==============================================================================
// generate patterns of random combinations 2, 3 and 4 beats
// with a random initial offset (so that patterns do not always start with a true value
// pattern of bools
// not as robust as creation of 2s and 3s (or 3 and 4s)
inline std::vector<bool> twosThreesAndFours( int nBeatsToGenerate )
{
    std::vector< bool > output(nBeatsToGenerate, false);
    //    output.resize(nBeatsToGenerate);
    //
    int beatsLeft = nBeatsToGenerate;
    int index =  rand()%4;
    if ( index != 0 ){ index += 1; }
    while ( beatsLeft > 0 )
    {
        output[index] = true;
        if (beatsLeft == 4)
        {
            index += 4;
            beatsLeft -= 4;
        }
        else if (beatsLeft == 3)
        {
            index += 3;
            beatsLeft -= 3;
        }
        else if (beatsLeft == 2)
        {
            index += 2;
            beatsLeft -= 2;
        }
        else
        {
            auto i = 2 + rand()%3;
            index += i;
            beatsLeft -= i;
        }
        index %= nBeatsToGenerate;
    }
    return output;
}

////==============================================================================
// faster modulo from www.youtube.com/watch?v=nXaxk27zwlk&t=3394s
// Chandler Carruth
inline
unsigned long fastMod ( const unsigned long input, const unsigned long ceil )
{
    // apply the modulo operator only when needed
    // (i.e. when the input is greater than the ceiling)
    return input < ceil ? input : input % ceil;
    // NB: the assumption here is that the numbers are positive
}
////==============================================================================
inline
unsigned long fastMod2 ( unsigned long input, const unsigned long ceil )
{
    //    while ( input < 0 ) { input += ceil; }
    while ( input >= ceil ) { input -= ceil; }
    return input;
    // assume values are positive
}

////==============================================================================
template < class intType >
void fastMod3( intType &input, const intType &ceil )
{
    while ( input < 0 ) { input += ceil; }
    while ( input >= ceil ) { input -= ceil; }
}
//==============================================================================
//
//void hadamard( std::vector< std::vector<float> > &hadamard, int size )
//{
//    // note size must be a power of 2!!!
//    hadamard.resize(size);
//    for ( int i = 0; i < size; i++ )
//    {
//        hadamard[i].resize(size);
//    }
//    hadamard[0][0] = 1.0f / sqrt( size ); // most simple matrix of size 1 is [1], whole matrix is multiplied by 1 / sqrt(size)
//    for ( int k = 1; k < size; k += k ) {
//        
//        // Loop to copy elements to
//        // other quarters of the matrix
//        for (int i = 0; i < k; i++) {
//            for (int j = 0; j < k; j++) {
//                hadamard[i + k][j] = hadamard[i][j];
//                hadamard[i][j + k] = hadamard[i][j];
//                hadamard[i + k][j + k] = -hadamard[i][j];
//            }
//        }
//    }
//}
//==============================================================================

#endif /* sjf_audioUtilities_h */
