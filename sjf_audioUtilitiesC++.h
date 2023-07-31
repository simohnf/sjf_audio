//
//  sjf_audioUtilities.h
//
//  Created by Simon Fay on 25/08/2022.
//

#ifndef sjf_audioUtilitiesCplusplus_h
#define sjf_audioUtilitiesCplusplus_h


#ifndef PI
#define PI 3.14159265
#endif

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
    static constexpr T m_householderWeight = ( -2.0f / static_cast<T>(size) );
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

//==============================================================================
//==============================================================================
//==============================================================================
template< typename T >
T midiToFrequency( T midiNote, T tuning = 440 )
{
    auto distanceFromA4 = midiNote - 69;
    return tuning * std::pow( 2, distanceFromA4 / 12 );
}

//==============================================================================
//==============================================================================
//==============================================================================
template< typename T >
T sjf_clip( T input, T outMin, T outMax )
{
    input = (input < outMin) ? ( outMin ) : ( (input > outMax) ? outMax : input );
    return input;
}

//==============================================================================
//==============================================================================
//==============================================================================
template< typename T >
T sjf_fold( T input, T outMin, T outMax )
{
    while ( input < outMin || input > outMax )
    {
        input = (input < outMin) ? outMin + (outMin - input) : outMax - (input - outMax);
    }
    return input;
}
//==============================================================================
//==============================================================================
//==============================================================================
template< typename T >
T sjf_wrap( T input, T outMin, T outMax )
{
    while ( input < outMin || input > outMax )
    {
        input = (input < outMin) ? outMax - (outMin - input) : outMin + (input - outMax);
    }
    return input;
}
//==============================================================================
//==============================================================================
//==============================================================================
template< typename T >
class sjf_jitter
{
public:
    static T addJitter( T input, T depth, T outMin, T outMax, int type )
    {
#ifndef NDEBUG
        assert( depth >= 0 && depth <= 1 );
        assert( type >= clip && type <= fold );
#endif
        auto r = abs( outMax - outMin ) * depth;
        r *= ( ( ( rand01() ) * 2.0 ) - 1.0 );
        input += r;
        switch (type)
        {
            case clip:
                input = (input < outMin) ? ( outMin ) : ( (input > outMax) ? outMax : input );
                return input;
            case wrap:
                return sjf_wrap< T >( input, outMin, outMax );
            case fold:
                return sjf_fold< T >( input, outMin, outMax );
            default:
                input = (input < outMin) ? ( outMin ) : ( (input > outMax) ? outMax : input );
                return input;
        }
        input = (input < outMin) ? ( outMin ) : ( (input > outMax) ? outMax : input );
        return input;
    }
    
//    static T addJitter( T input, T outMin, T outMax )
//    {
//        auto range = abs( outMax - outMin );
//        auto r = ( ( ( rand01() ) * 2.0 ) - 1.0 ) * range;
//        input += r;
//    }
    enum limitType {
        clip = 0, wrap, fold
    };
};

//==============================================================================
//==============================================================================
//==============================================================================


//==============================================================================
//==============================================================================
//==============================================================================
template < class T >
class sjf_filterResponse
{
public:
    static std::array< T, 3 > calculateFilterResponse( T f0, T sampleRate, const std::vector< T >& xCoefficients, const std::vector< T >& yCoefficients, T tolerance = M_PI )
    {
        std::array< T, 3 > apd; // amplitude, phase, delay(samples)
        auto fNormal = f0 / sampleRate;
        static constexpr float M_TWO_PI = 2.0 * M_PI;
        auto w = M_TWO_PI * fNormal;
        auto xComp = calculateRealAndImaginary( w, xCoefficients );
        auto yComp = calculateRealAndImaginary( w, yCoefficients );
        apd[ 0 ] = std::sqrt( xComp[ 0 ]*xComp[ 0 ] + xComp[ 1 ]*xComp[ 1 ] ) / std::sqrt( yComp[ 0 ]*yComp[ 0 ] + yComp[ 1 ]*yComp[ 1 ] );
        apd[ 1 ] = std::atan2( xComp[ 1 ], xComp[ 0 ] ) - std::atan2( yComp[ 1 ], yComp[ 0 ] );
        apd[ 2 ] = ( apd[ 1 ] / fNormal  ) / M_TWO_PI;
        return apd;
    }
private:
    static std::array< T, 2 > calculateRealAndImaginary( T w, const std::vector< T >& coefficients)
    {
        std::array< T, 2 > rAndI;
        rAndI[ 0 ] = 0.0;
        rAndI[ 1 ] = 0.0;
        for ( auto i = 0; i < coefficients.size(); i++ )
        {
            auto a = -1 * i * w;
            rAndI[ 0 ] += coefficients[ i ]*std::cos( a );
            rAndI[ 1 ] += coefficients[ i ]*std::sin( a );
        }
        return rAndI;
    }
};
#endif /* sjf_audioUtilitiesC++ */
