//
//  sjf_audioUtilities.h
//
//  Created by Simon Fay on 25/08/2022.
//

#ifndef sjf_audioUtilitiesCplusplus_h
#define sjf_audioUtilitiesCplusplus_h

#include <math.h>
#include <vector>
#include <array>
#include <string>
#include "gcem/include/gcem.hpp"

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
    return sin( M_PI* (rampUp+rampDown)/2 ); // this gives a smooth sinewave based fade
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
    
    return sin( M_PI* pan ); // this gives a smooth sinewave based fade
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
/** Use like `Hadamard<double, 8>::inPlace(data)` - size must be a power of 2 */
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
    T twoPiW = ( 2 * M_PI * w );
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
        assert( depth >= 0.0 && depth <= 1.0 );
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





//==============================================================================
//==============================================================================
//==============================================================================
template< int LEVELS >
struct sjf_divNames
{
private:
    static constexpr int NDIVTYPES = 5;
    std::array< std::string, NDIVTYPES >  m_divTypes =  { "nd", "n", "nq", "nt", "ns" };
    std::array< std::string, LEVELS * NDIVTYPES > m_table;
public:
    sjf_divNames() : m_table()
    {
//        DBG("building divNames");
        for ( auto d = 0; d < LEVELS; d++ )
        {
            int i = 1;
            for ( auto j = 0; j < d; j++ )
                i *= 2;
            auto iStr = std::to_string( i );
            for ( auto dt = 0; dt < NDIVTYPES; dt++ )
            {
//                DBG( d*NDIVTYPES + dt << " " << "/" + iStr  + m_divTypes[ dt ] );
                m_table[ d*NDIVTYPES + dt ] =  "/" + iStr  + m_divTypes[ dt ];
            }
        }
//        DBG("finished building divNames");
//        DBG( NDIVTYPES << " " << LEVELS << " " << m_table.size() );
    }
    ~sjf_divNames(){}
    
    std::array< std::string, LEVELS*NDIVTYPES > getAllNames()
    {
        return m_table;
    }
    
    std::string getName(std::size_t index)
    {
        return m_table[ index ];
    }

    std::array< int, 2 > getNumberAndDivision( std::size_t index )
    {
        std::array< int, 2 > output{ 0, 0 };
        auto type = static_cast< int >(index % NDIVTYPES);
        auto base = static_cast< int >(index / NDIVTYPES);
        switch( type )
        {
            case 0 :
                output = { 3, 2 };
                break;
            case 1 :
                output = { 1, 1 };
                break;
            case 2 :
                output = { 4, 5 };
                break;
            case 3 :
                output = { 2, 3 };
                break;
            case 4 :
                output = { 4, 7 };
                break;
        }
        output[ 1 ] *= ( std::pow( 2, base ) );
        return output;
    }

    float getMultiplier( std::size_t index )
    {
        auto numAndDiv = getNumberAndDivision( index );
        return ( static_cast<float >( numAndDiv[ 0 ] ) / static_cast< float >( numAndDiv [ 1 ] ) );
    }
    
    int getNDIVTYPES(){ return NDIVTYPES; }

};


template< typename T >
T sjf_findClosestMultiple( T value, T base )
{
    double mu = static_cast< double > ( value ) / static_cast< double > ( base );
    int mult = mu;
    mu -= mult;
    mult = mu >= 0.5 ? mult + 1 : mult;
    return mult * base;
};




//============================================================
//============================================================
//================= TIME STRETCH ALGORITHM STUFF =============

template < typename T >
T sjf_crossCorrelation( const T* val1, const T* val2, const size_t nSteps, const T norm = 1 )
{
    T sum = 0;
    for ( auto i = 0; i < nSteps; i++ )
    {
        sum += val1[ i ] * val2[ i ];
    }
    return sum * norm;
}

//==========================================================

template < typename T >
auto sjf_crossCorrelationMaxTimeLag( const T* val1, const T* val2, const size_t nVals )
{
    T norm = 1.0 / static_cast< T > (nVals);
    auto maxIndex = 0;
    auto maxVal = crossCorrelation( val1, val2, nVals ) * norm;
    for ( auto i = 1; i < nVals; i++ )
    {
        auto newVal = crossCorrelation( val1, &val2[ i ], nVals-i ) * norm;
        if ( newVal > maxVal )
        {
            maxIndex = i;
            maxVal = newVal;
        }
    }
    return maxIndex;
}

//==========================================================
/** check if an integer is a prime number */
inline constexpr bool sjf_isPrime( int number )
{
    auto max = gcem::floor( gcem::sqrt( number ) );
    if ( number < 2 )
        return false;
    if ( number == 2 )
        return true;
    for ( auto i = 2; i < ( max + 1 ); i ++ )
    {
        auto f = static_cast< float >( number ) / static_cast< float >( i );
        if ( ( f - static_cast< int >( f ) ) == 0 )
            return false;
    }
    return true;
}
//==========================================================
/** check whether a number is a power of another number */
inline bool sjf_isPowerOf( const unsigned long val, const unsigned long baseToCheck )
{
    assert( val >= baseToCheck  );
    auto b = baseToCheck;
    while ( b < val )
        b*= baseToCheck;
    return ( b == val  );
}

//==========================================================
/** find the nearest power of a base above value */
template< typename T >
inline T sjf_nearestPowerAbove( T val, T base )
{
    return std::pow( 2, std::ceil( std::log( val )/ std::log( base ) ) );
}
//==========================================================
/** find the nearest power of a base below value */
inline unsigned long sjf_nearestPowerBelow( unsigned long val, unsigned long base )
{
    return std::pow( 2, std::floor( std::log( val )/ std::log( base ) ) );
}
//==========================================================
/** calculate room mode */
template< typename T >
T sjf_calculateRoomMode( T length_meters, T width_meters, T height_meters, int P, int Q, int R , T speedOfSound = 344 )
{
    T PoL = static_cast<T>( P ) / length_meters;
    T QoW = static_cast<T>( Q ) / width_meters;
    T RoH = static_cast<T>( R ) / height_meters;
    return ( speedOfSound*0.5 ) * std::sqrt( PoL*PoL + QoW*QoW + RoH*RoH );
}
//==========================================================
/** calculate greatest common denominator */
inline int GCD( int* values, int nValues )
{
    int max = 0;
    for ( auto i = 0; i < nValues; i++ )
        max = values[ i ] > max ? values[ i ] : max;
    for ( auto i = max; i > 1; i-- )
    {
        auto sum = 0;
        for ( auto j = 0; j < nValues; j++ )
            if ( values[ j ] % i != 0 )
                sum += 1;
        if ( sum == 0 )
            return i;
    }
    return 1;
}

//==========================================================
// calculate room mode
template< int STEPS >
struct sjf_PQR
{

private:
    static constexpr int NAXIAL = 3;
    static constexpr int NTANGENTIAL = STEPS*3;
    static constexpr int NOBLIQUE = STEPS*3 + 1;
    static constexpr int SIZE = NAXIAL + NTANGENTIAL + NOBLIQUE;
    int m_finalSize = SIZE;
    int m_table[ SIZE ][ 3 ];

public:
    constexpr sjf_PQR() : m_table()
    {
        int tempTable[ SIZE ][ 3 ];
        for ( int i = 0; i < 3; i++ )
        {
            tempTable[ i ][ 0 ] = tempTable[ i ][ 1 ] = tempTable[ i ][ 2 ] = 0;
            tempTable[ i ][ i ] += 1;
        }
        for ( int i = 0; i < STEPS*3; i++ )
        {
            int pqr[ 3 ] = { tempTable[ i ][ 0 ], tempTable[ i ][ 1 ], tempTable[ i ][ 2 ] };
            if ( pqr[ i % 3 ] > pqr[ (i + 1) % 3 ] )
                pqr[ (i + 1) % 3 ] +=1;
            else
                pqr[ i % 3 ] +=1;
            for ( auto j = 0; j < 3; j++ )
                tempTable[ i + 3 ][ j ] = pqr[ j ];
        }
        auto count = STEPS*3 + 3;
        tempTable[ count ][ 0 ] = tempTable[ count ][ 1 ] = tempTable[ count ][ 2 ] = 1;
        auto stop = count + STEPS;
        for ( int i = count; i < stop; i++ )
            for ( auto j = 0; j < 3; j++ )
            {
                int pqr[ 3 ] = { tempTable[ i ][ 0 ], tempTable[ i ][ 1 ], tempTable[ i ][ 2 ] };
                pqr[ j ] += 1;
                count += 1;
                for ( auto k = 0; k < 3; k++ )
                    tempTable[ count ][ k ] = pqr[ k ];
            }
        count = 0;
        for ( auto i = 0; i < SIZE; i++ )
        {
            int pqr[ 3 ] = { tempTable[ i ][ 0 ], tempTable[ i ][ 1 ], tempTable[ i ][ 2 ] };
            auto isUnique = true;
            for ( auto j = 0; j < i; j++ )
            {
                if ( (pqr[0]==tempTable[j][0]) && (pqr[1]==tempTable[j][1]) && (pqr[2]==tempTable[j][2]) )
                {
                    isUnique = false;
                    break;
                }
            }
            if ( isUnique && GCD( pqr, 3 ) == 1 )
            {
                m_table[count][0] = pqr[0];
                m_table[count][1] = pqr[1];
                m_table[count][2] = pqr[2];
                count ++;
            }
        }
        m_finalSize = count;
    }

    ~sjf_PQR(){}

    const size_t getSize(){ return m_finalSize; }
    const int getValue(std::size_t index, std::size_t index2) const { return m_table[ index ][ index2 ]; }
    const int* operator[](std::size_t index) const { return m_table[ index ]; }
};


//========//========//========//========//========//========//========
//========//========//========//========//========//========//========
//========//========//========//========//========//========//========
//** Calculate the feedback gain necessary to achieve a desired decay time given a specific delay time */
template< typename T >
T sjf_calculateFeedbackGain( T delayTime, T desiredDecayTime )
{
    return gcem::pow( 10.0, -3.0 * delayTime / desiredDecayTime );
}

//m_gain[ s ] = std::pow( 10.0, -3.0 * del / m_decayInMS );

/** Generate a series of random values. The maximum value is divided into n bands, and each random value will be placed within one of these bands */
template< typename T >
void genVelvetNoise( T max, std::vector< T >& storage  )
{
    auto nVals = storage.size();
    auto bw = max / static_cast< T >( nVals );
    for ( auto v = 0; v < nVals; v++ )
    {
        auto low = bw * v;
        storage[ v ] = low + bw * rand01();
    }
}


namespace sjf::utilities
{
    /** class to hold and call member functions of other classes ( e.g. instead of having big switch/if statements etc */
template< typename classType, typename returnType, typename... arguments >
    class classMemberFunctionPointer
    {
        typedef returnType ( classType::*memberPtr )( arguments... args );
    public:
        classMemberFunctionPointer( classType* parent ) noexcept : PARENT( parent ) {}
        classMemberFunctionPointer( classType* parent, memberPtr memFunc ) : funcPtr( memFunc ), PARENT( parent ) {}

        classMemberFunctionPointer( const classMemberFunctionPointer& ) noexcept = delete;
        classMemberFunctionPointer& operator=( const classMemberFunctionPointer& ) noexcept = delete;
        
        classMemberFunctionPointer( classMemberFunctionPointer&&) noexcept = default;
        classMemberFunctionPointer& operator=( classMemberFunctionPointer&&) noexcept = default;
        
        
        ~classMemberFunctionPointer(){}
        
        returnType operator() ( arguments... args ){ return ( PARENT->*funcPtr )( args... ); }

        void operator=( memberPtr func ){ funcPtr = func; }

    private:

        memberPtr funcPtr{ nullptr };
        classType* PARENT;
    };

//========//========//========//========//========//========//========
//========//========//========//========//========//========//========
//========//========//========//========//========//========//========

    /** Simple class for ending to and decoding from MS. Use like : sjf::utilities::MidSide<float>::encode( lSamp, rSamp )*/
    template < typename Sample >
    struct MidSide
    {
        struct MS{ Sample mid, side; };
        struct LR{ Sample left, right; };
    
        /** encode from LR to MS */
        static inline MS encode( Sample left, Sample right ) { return { left + right, left - right }; }
        /** encode from LR to MS */
        static inline MS encode( LR lr ) { return { lr.left + lr.right, lr.left - lr.right }; }
        
        /** decode from MS to LR  */
        static inline LR decode( MS ms ) { return { (ms.mid + ms.side)*outScale,  (ms.mid - ms.side)*outScale, }; }
        /** decode from MS to LR  */
        static inline LR decode( Sample mid, Sample side ) { return { (mid + side)*outScale,  (mid - side)*outScale, }; }
        
    private:
        static constexpr Sample outScale{0.5};
    };

    template< typename Sample >
    Sample clip( Sample value, const Sample min, const Sample max ) { return value < min ? min : value > max ? max : value; }



    template < typename Sample >
    Sample phaseEnvelope( const Sample phase, const Sample nRampSegments )
    {
        Sample up, down;
        
        up = down = phase * nRampSegments;
        up = clip< Sample >( up, 0, 1 );
        
        down -= ( nRampSegments - 1 );
        down *= -1;
        down = clip< Sample > ( down, -1, 0 );
        
        return up + down;
    }

//    template< typename T >
//    void twoDVectorResize( std::vector< std::vector< T > >& vect, size_t newSize1, size_t newSize2 )
//    {
//        vect.resize( newSize1, std::vector< T >( newSize2 ) );
//    }
//
//    template< typename T, typename T2 >
//    void twoDVectorResize( std::vector< std::vector< T > >& vect, size_t newSize1, size_t newSize2, T2 initialValue )
//    {
//        vect.resize( newSize1, std::vector< T >( newSize2, initialValue ) );
//    }

    /** resize multidimensional std::vectors */
    template< typename T, typename... ARGS >
    void vectorResize( std::vector<T>& vect, size_t newSize ) { vect.resize( newSize ); }

    /** resize multidimensional std::vectors */
    template< typename T, typename... ARGS >
    void vectorResize( std::vector<T>& vect, size_t newSize, T initialValue ) { vect.resize( newSize, initialValue ); }
    
    /** resize multidimensional std::vectors */
    template< typename T, typename... ARGS >
    void vectorResize( std::vector<T>& vect, size_t newSize, ARGS... args ) { vect.resize( newSize, {args...} ); }
    
    /** resize multidimensional std::vectors
     Call like vectorResize( vectorName, d1 size, d2 size, etc )
     For vectors of floats/ints etc, an additional final value can be included to initialise all values.
     ==> e.g. vectorResize( myVector, 2, 3, 0.2  ) --> std::vector<std::vector<double>> { {0.2, 0.2, 0.2}, {0.2, 0.2, 0.2} }
     If the final dimension is another container this can be initialised by providing the required number of initial values.
     ==> e.g. vectorResize( myVectorOfArrays, 2, 3, 0.2, 0.3  ) --> std::vector<std::vector<std::array<double, 2>>> { { {0.2, 0.3}, {0.2, 0.3}, {0.2, 0.3}  },  { {0.2, 0.3}, {0.2, 0.3}, {0.2, 0.3}  } };
     Alternatively, the final dimension can be initialised using a braced initialiser list
     ==> e.g. e.g. vectorResize( myVectorOfArrays, 2, 3, {0.2, 0.3, 0.5}  ) --> std::vector<std::vector<std::vector<double>>>{ { {0.2, 0.3, 0.5} , {0.2, 0.3, 0.5} , {0.2, 0.3, 0.5} },  { {0.2, 0.3, 0.5} , {0.2, 0.3, 0.5} , {0.2, 0.3, 0.5} } };
     */
    template< typename T, typename... ARGS >
    void vectorResize( std::vector<std::vector<T>>& vect, size_t newSize, ARGS... args )
    {
        vect.resize( newSize );
        for ( auto & v: vect )
            vectorResize( v, args... );
    }


    /** Calculate the sqrt of a number using Newton's method ( recursive ) */
    template< typename T >
    constexpr T sqrtR( T x, T tol = 0.00001, T guess = 1 )
    {
        auto nG = 0.5 * ( guess + x/guess );
        auto diff = nG - guess;
        diff = diff<0 ? -diff : diff;
        if ( diff <= tol )
            return nG;
        else
            return sqrtR( x, tol, nG );
    }

    /** Calculate the sqrt of a number using Newton's method ( recursive ) */
    template< typename T >
    constexpr T sqrt( T x, T tol = 0.00001 )
    {
        auto guess = 1.;
        auto nG = 0.5 * ( guess + x/guess );
        auto diff = nG - guess;
        diff = diff<0 ? -diff : diff;
        while( diff > tol )
        {
            guess = nG;
            nG = 0.5 * ( guess + x/guess );
            diff = nG - guess;
            diff = diff<0 ? -diff : diff;
        }
        return nG;
    }

    /** check if an integer is a prime number */
    inline constexpr bool isPrime( size_t number )
    {
        if ( number < 2 )
            return false;
        if ( number == 2 )
            return true;
        auto max = sqrt<double>(number);
        auto count = 2.;
        while( count <= max )
        {
            auto res = number/count;
            if( res - static_cast<size_t>(res) == 0 )
                return false;
            ++count;
        }
        return true;
    }

    /** Simple struct holding all prime numbers upto(and including) the maximum */
    template< size_t MAX >
    struct primes
    {
        constexpr primes() : m_table(), m_table2()
        {
            for ( auto i = 0; i < MAX+1; ++i )
            {
                if( isPrime( i, MAX2 ) )
                {
                    m_table[ i ] = true;
                    m_table2[ MAX2 ] = i;
                    ++MAX2;
                }
                else
                    m_table[ i ] = false;
            }
        }
        
        /** Is the given number prime? */
        const bool operator[] ( size_t number ) const
        {
            assert( number < MAX+1 );
            return m_table[ number ];
        }
        
        /** What is the nth prime number? (note the first prime is index 0!!!) */
        const size_t nthPrime( size_t n ) const
        {
            assert( n < MAX2 );
            return m_table2[ n ];
        }
        
        /** How many primes are less than or equal to the given maximum */
        const size_t getNPrimes() const { return MAX2; }
    private:
        constexpr bool isPrime( size_t number, size_t countToDate ) const
        {
            if ( number < 2 ){ return false; }
            if ( number == 2 ){ return true; }
            auto sq = sqrt<double>( number );
            auto max = sq;
            max = max < countToDate ? max : countToDate;
            for ( auto i = 0; i < max; ++i )
            {
                auto test = static_cast<double>( m_table2[ i ] );
                if ( test > sq )
                    return true;
                auto res = number / static_cast<double>( m_table2[ i ] );
                if( res - static_cast<size_t>(res) == 0 )
                    return false;
            }
            return true;
        }
        
        
        
        bool m_table[ MAX+1 ];
        size_t MAX2{0};
        size_t m_table2[ MAX+1 ];
    };

    /** simple function to wrap input when it goes below a given minimum or above given maximum */
    template < typename T, T min = 0, T max = 1 >
    T wrap ( T x )
    {
        static constexpr T range = max - min;
        while ( x < min )
            x += range;
        while ( x > max )
            x -= range;
        return x > min ? ( (x < max) ? x : x - range ) : x + range;
    };
}
#endif /* sjf_audioUtilitiesC++ */
