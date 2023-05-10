//
//  sjf_audioUtilities.h
//
//  Created by Simon Fay on 25/08/2022.
//

#ifndef sjf_audioUtilities_h
#define sjf_audioUtilities_h

#include <JuceHeader.h>
#include "sjf_compileTimeRandom.h"

#define PI 3.14159265
template< typename T >
void clipInPlace( T& value, const T& min, const T& max )
{
    if ( value < min )
        value = min;
    else if ( value > max )
        value = max;
}

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
// phase envelopVersion 2
template < typename T >
T phaseEnvelope( const T& phase, const T& nRampSegments )
{
    T up, down;
    
    up = down = phase * nRampSegments;
    clipInPlace< T >( up, 0, 1 );
    
    down -= ( nRampSegments - 1 );
    down *= -1;
    clipInPlace< T > ( down, -1, 0 );
    
    return up + down;
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
unsigned long fastMod ( const unsigned long input, const unsigned long &ceil )
{
    // apply the modulo operator only when needed
    // (i.e. when the input is greater than the ceiling)
    return input < ceil ? input : input % ceil;
    // NB: the assumption here is that the numbers are positive
}
////==============================================================================
inline
unsigned long fastMod2 ( unsigned long input, const unsigned long &ceil )
{
    //    while ( input < 0 ) { input += ceil; }
    while ( input >= ceil ) { input -= ceil; }
    return input;
    // assume values are positive
}

////==============================================================================
template < class type >
void fastMod3( type &input, const type &ceil )
{
    while ( input < 0 ) { input += ceil; }
    while ( input >= ceil ) { input -= ceil; }
}
////==============================================================================
template < class type >
type fastMod4( type input, const type &ceil )
{
    while ( input < 0 ) { input += ceil; }
    while ( input >= ceil ) { input -= ceil; }
    return input;
    
}
//==============================================================================
template< class T >
T sjf_scale( const T& valueToScale, const T& inMin, const T& inMax, const T& outMin, const T& outMax )
{
//    DBG("SCALE " << valueToScale << " " << inMin << " " << inMax << " " << outMin << " " << outMax );
    return ( ( ( valueToScale - inMin ) / ( inMax - inMin ) ) * ( outMax - outMin ) ) + outMin;
}
//==============================================================================
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
    
    static inline void inPlace(Sample * data, const Sample &scalingFactor) {
        recursiveUnscaled(data);
        
        //        Sample scalingFactor = std::sqrt(1.0/size);
        for (int c = 0; c < size; ++c) {
            data[c] *= scalingFactor;
        }
    }
};
//==============================================================================
//==============================================================================
//==============================================================================
template< typename T, int size >
class Householder
{
    static constexpr T m_householderWeight = ( -2.0f / (T)size );
public:
    static inline void mixInPlace( std::array< T, size >& data )
    {
        T sum = 0.0f; // use this to mix all samples with householder matrix
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
//==============================================================================
//==============================================================================
//==============================================================================

template< typename T > // fold input within given range
T fFold( T input, const T outMin, const T outMax )
{
    if ( input > outMin && input < outMax ){ return input; }
    T range = outMax - outMin;
    T twoRange = range * 2.0f;
//    bool neg = input < 0 ? true : false;
//    if ( neg ) { input *= -1.0f; }
    input = abs(input);
    fastMod3< T >( input, twoRange );
    if ( input > range ) { input = twoRange - input; }
    input += outMin;
    return input;
}

//==============================================================================
//==============================================================================
//==============================================================================

template< typename T >
T calculateLPFCoefficient( const T& frequency, const T& sampleRate )
{
    T w = ( frequency / sampleRate );
    T twoPiW = ( 2 * PI * w );
    // MAKE SURE FREQUENCY IS IN A LOGICAL RANGE
//    T coef = sin(  twoPiW );
//    DBG( "1 - sin f " << frequency << " coef " << coef );
    
//    coef = 1.0 - exp( -1.0 * twoPiW );
//    DBG( "2 - exp f " << frequency << " coef " << coef );
//
//    T y = 1 - cos( twoPiW );
//    coef = sqrt( y*y + 2*y ) - y;
//    DBG( "3 - cos f " << frequency << " coef " << coef );
    
//    coef = twoPiW  / ( 1 + twoPiW  );
//    DBG( "4 - f " << frequency << " coef " << coef );
    
//    DBG( " " );
//    return coef;
    return 1.0 - exp( -1.0*twoPiW );
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
        return (T)sjf_compileTimeRandom::get_random( count ) / (T)RAND_MAX;
    }
    //----------------------------------------
    T floatArray[ NUM_ROWS ][ NUM_COLUMNS ];
};


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

//==============================================================================
//==============================================================================

//inline bool loadAudioFile( juce::AudioBuffer< float >& buffer, juce::String& samplePath, juce::String& sampleName, double& fileSampleRate )
//{
//    juce::AudioFormatReader formatManager;
//    formatManager.registerBasicFormats();
//    auto chooser = std::make_unique<juce::FileChooser> ("Select a Wave/Aiff file to use..." , juce::File{}, "*.aif, *.wav");
//    auto chooserFlags = juce::FileBrowserComponent::openMode
//    | juce::FileBrowserComponent::canSelectFiles;
//    bool loaded = false;
//    chooser->launchAsync (chooserFlags, [this] (const juce::FileChooser& fc)
//                            {
//                                auto file = fc.getResult();
//                                if (file == juce::File{}) { return; }
//                                std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));
//                                if (reader.get() != nullptr)
//                                {
//                                    auto nSamps = (int) reader->lengthInSamples;
//                                    auto nChannels = (int) reader->numChannels;
//                                    fileSampleRate = reader->sampleRate;
//                                    buffer.setSize( nChannels, nSamps );
//                                    reader->read (&buffer, 0, nSamps, 0, true, true);
//                                    samplePath = file.getFullPathName();
//                                    sampleName = file.getFileName();
//                                    loaded = true;
//                                    return loaded;
//                                }
//                            });
//    return loaded;
//}



#endif /* sjf_audioUtilities_h */
