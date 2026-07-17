//
//  sjf_waveguide.h
//  sjf_waveguide
//
//  Created by Simon Fay on 19/06/2023.
//  Copyright © 2023 sjf. All rights reserved.
//

#ifndef sjf_waveguide_h
#define sjf_waveguide_h
#include "sjf_delayLine.h"
#include "sjf_audioUtilitiesC++.h"
#include "sjf_lpf.h"
#include "sjf_waveshapers.h"

#include "gcem/include/gcem.hpp"

#include <limits>

#define MINIMUM_FREQUENCY 5.0

//==============================================
//==============================================
//==============================================
//==============================================

template < class T >
class sjf_twoZeroLP
{
public:
    sjf_twoZeroLP(){}
    ~sjf_twoZeroLP(){}
    
    void setCoefficient( T c )
    {
        m_a1 = (1+c)/2;
        m_a2 = (1-c)/4;
    }
    
    T process( T x )
    {
        auto output = (m_a1 * m_x1) + m_a2*( x + m_x2 ) ;
        m_x2 = m_x1;
        m_x1 = x;
        return output;
    }
private:
    T m_x1 = 0, m_x2 = 0, m_a1, m_a2;
};

//==============================================
//==============================================
//==============================================
//==============================================

template < class T >
class sjf_waveguideAllpass
{
public:
    sjf_waveguideAllpass()
    {
        m_delayLine.initialise( 44100 / MINIMUM_FREQUENCY );
        m_delayLine.setInterpolationType( sjf_interpolators::interpolatorTypes::allpass );
    }
    ~sjf_waveguideAllpass(){}
    
    void prepare( T sampleRate )
    {
        m_delayLine.initialise( sampleRate / MINIMUM_FREQUENCY );
        clear();
    }
    
    
    void clear()
    {
        m_delayLine.clearDelayline();
    }
    
    T process( T input )
    {
        auto dOut = m_delayLine.getSample2();
        input += ( dOut * m_g );
        m_delayLine.setSample2( input );
        dOut -= ( input * m_g );
        return dOut;
    }
    
    T sensorOutput()
    {
        auto sensorOut = m_delayLine.tapDelayLine( m_tapNum );
        return sensorOut;
    }
    
    void setDelayInSamps( T totalDelayInSamps )
    {
        m_delayInSamps = totalDelayInSamps;
        m_delayLine.setDelayTimeSamps( m_delayInSamps );
        setSensorPosition( m_sensorPosition );
    }
    
    void setSensorPosition( const T sensorPos )
    {
#ifndef NDEBUG
        assert( sensorPos > 0 && sensorPos < 1 );
#endif
        m_sensorPosition = sensorPos;
        m_sensorDelay = m_delayInSamps * m_sensorPosition;
        m_delayLine.setTapTime( m_tapNum, m_sensorDelay );
    }
    
    void setG( const T g )
    {
#ifndef NDEBUG
        assert( g > -1 && g < 1 );
#endif
        m_g = g;
    }
    
private:
    sjf_delayLine< double > m_delayLine;
    T m_delayInSamps = 4410, m_sensorPosition = 0.5 , /* m_pickPosition = 0.25, */ m_g = 0;
    T m_sensorDelay = m_delayInSamps * m_sensorPosition;
    
    static constexpr int m_tapNum = 0;
};

//==============================================
//==============================================
//==============================================
//==============================================
template < class T >
class sjf_pmExcitation
{
public:
    sjf_pmExcitation()
    {
        m_exciteTable.resize( m_periodSamples );
        m_envelopeTable.resize( 520 );
        for ( auto& e : m_envelopeTable )
            e = 0;
        setEnvelope( 0.5, 1 );
        m_lpf.setCoefficient( calculateLPFCoefficient( 1000, 44100 ) );
    }
    
    ~sjf_pmExcitation(){}
    
    void setLPFCoef( T c )
    {
        m_lpf.setCoefficient( c );
    }
    
    void triggerNewExcitation( const T periodSamples, const T amplitude, const T noiseBlend )
    {
//        m_lpf.reset();
        m_periodSamples = static_cast< int > ( periodSamples );
        m_exciteTable.resize( m_periodSamples );
        std::vector< T > randomBits;
        randomBits.resize( m_periodSamples );
        // fill random table with 1 or minus 1
        static constexpr auto longsize = sizeof( long ) * CHAR_BIT;
        auto pos = 0;
        std::bitset<  longsize  > randNum;
        while ( pos < m_periodSamples )
        {
            randNum = std::rand();
            for( auto i = 0; i < randNum.size(); i++ )
            {
                auto val = randNum[ i ] ? noiseBlend : -noiseBlend;
                randomBits[ pos ] = val;
                pos += 1;
                if ( pos == m_periodSamples )
                    break;
            }
        }
        for ( auto i = 0; i < m_periodSamples; i++ )
        {
            auto ampfIndex = ( static_cast<T>( i ) / static_cast<T>( m_periodSamples ) ) * static_cast<T>( m_envelopeTable.size() - 1 );
            auto ampIndex = static_cast<int>( ampfIndex );
            auto ampMu = ampfIndex - ampIndex;
            auto ampX1 = m_envelopeTable[ ampIndex ];
            auto ampX2 = m_envelopeTable[ ampIndex + 1];
            auto amp = sjf_interpolators::linearInterpolate( ampMu, ampX1, ampX2 ) * amplitude;
            auto val = amp * ( (1 - noiseBlend) + randomBits[ i ] );
            m_exciteTable[ i ] = m_lpf.filterInput( val ) ;
        }
        m_readPointer = 0;
        m_attackFlag = true;
    }
    
    T outputExcitation()
    {
        if ( !m_attackFlag )
            return 0;
        auto outSamp = m_exciteTable[ m_readPointer ];
        m_readPointer += 1;
        m_attackFlag = m_readPointer >= m_periodSamples ? false : true;
        return outSamp;
    }
    
    void setEnvelope( const T centrePoint, const T power )
    {
        // first sample in envelope and last samples in envelope are always zero so it always returns to zero
        for ( auto i = 0; i < STEPS_IN_ENVELOPE; i++)
        {
            auto normalisedPos = static_cast<T>( i ) / static_cast<T>( STEPS_IN_ENVELOPE ) ;
            m_envelopeTable[ i+1 ] =  ( normalisedPos < centrePoint ) ? ( normalisedPos / centrePoint ) : ( 1 - normalisedPos ) / ( 1 - centrePoint );
            m_envelopeTable[ i+1 ] = std::pow( m_envelopeTable[ i+1 ], power );
        }
    }
    
    std::vector< T >& getEnvelope()
    {
        return m_envelopeTable;
    }
    
private:
    
    
    const size_t STEPS_IN_ENVELOPE = 512;
    sjf_lpf< T > m_lpf;
    T m_periodSamples = 100;
    std::vector< T > m_exciteTable, m_envelopeTable;
    bool m_attackFlag = false;
    size_t m_readPointer = 0;
};


//==============================================
//==============================================
//==============================================
//==============================================
template < class T >
class sjf_decayTable
{
public:
    sjf_decayTable(){}
    ~sjf_decayTable(){}
    void prepare( T sampleRate )
    {
        createDecayTable( sampleRate );
    }
    
    void reset()
    {
        m_decayEnvCount = 0;
        m_finishedFlag = false;
        m_isRunning = false;
    }
    
    std::optional<T> outputEnvelope()
    {
        if( m_finishedFlag )
            return {};
        auto output = m_decayTable[ m_decayEnvCount ];
        m_decayEnvCount += 1;
        if ( m_decayEnvCount >= m_decayTable.size() )
            m_finishedFlag = true;
        return output;
    }
    
    void startRunningEnvelope()
    {
        m_isRunning = true;
    }
    
    bool getIsRunning()
    {
        return m_isRunning;
    }
    
private:
    void createDecayTable( T sampleRate )
    {
        auto decayLength = sampleRate / 10.0;
        m_decayTable.resize( decayLength );
        
        for ( auto i = 0; i < m_decayTable.size(); i++ )
            m_decayTable[ i ] = ( decayLength - i ) / decayLength;
    }
    
    bool m_finishedFlag = false, m_isRunning = false;
    size_t m_decayEnvCount = 0;
    std::vector< T > m_decayTable;
};

//==============================================
//==============================================
//==============================================
//==============================================
template < class T >
class sjf_threePointHighPass
{
public:
    sjf_threePointHighPass(){}
    ~sjf_threePointHighPass(){}
    
    T process( T x )
    {
        m_y1 = m_a0*x + m_a1*m_x1 + m_b1*m_y1;
        m_x1 = x;
        return m_y1;
    }
    
    void setCutoff( T f, T sr )
    {
        auto w2 = sin (2. * M_PI * f / sr ) / 2;
        m_a0 = 1.0 / ( 1 + w2 );
        m_a1 = -1.0 * m_a0;
        m_b1 = m_a0 * ( 1  - w2 );
    }
    
    void clear()
    {
        m_x1 = 0;
        m_y1 = 0;
    }
    
private:
    T m_x1 = 0, m_y1 = 0;
    T m_a0 = 0.5, m_a1 = 0.5, m_b1 = 0.5;
};

//==============================================
//==============================================
//==============================================
//==============================================


class sjf_waveguide
{
public:
    sjf_waveguide()
    {
        // min freq == 20hz; max period in seconds = 1/20; max period in samples = SR/20;
        for ( auto& wg : m_waveguide )
        {
            wg.prepare( m_SR );
        }
        m_emb.initialise( m_SR / MINIMUM_FREQUENCY );
        m_emb.setInterpolationType( sjf_interpolators::interpolatorTypes::allpass );
        
        m_excitation.setLPFCoef( calculateLPFCoefficient( 1000.0, m_SR ) );
        
        m_decayTable.prepare( m_SR );
        
    }
    ~sjf_waveguide(){}
    
    void prepare( double sampleRate )
    {
        m_SR = sampleRate;
        for ( auto& wg : m_waveguide )
        {
            wg.prepare( m_SR );
            wg.clear();
        }
        m_emb.initialise( m_SR / MINIMUM_FREQUENCY );
        m_emb.clearDelayline();
        
        m_excitation.setLPFCoef( calculateLPFCoefficient( 1000.0, m_SR ) );
        
        m_decayTable.prepare( m_SR );
    }
    
    void triggerNewNote( double midiPitch, double midiVelocity )
    {
        newNote( midiPitch, midiVelocity, m_split, m_stiff, m_sensorPos, m_pickPos, m_decaySeconds, m_mediumBrightness, m_exciteNoiseLevel, m_exciteEnvCentre, m_exciteEnvExponent, m_exciteExtension, m_exciteNoiseBrightness, m_apnonLinA, m_harmLevel, m_harmNumber );
    }
    
    void triggerNewNote( double midiPitch, double midiVelocity, double split, double stiff, double sensor, double pickPos, double decay, double medBright, double exciteNoiseLevel, double exciteEnvCentre, double exciteEnvExponent, double exciteEnvExtension, double exciteNoiseBright, double nonLinA, double harmonicLevel, double harmonicNumber )
    {
        newNote( midiPitch, midiVelocity, split, stiff, sensor, pickPos, decay, medBright, exciteNoiseLevel, exciteEnvCentre, exciteEnvExponent, exciteEnvExtension, exciteNoiseBright, nonLinA, harmonicLevel, harmonicNumber );
    }
    
    double processSample( int indexThroughCurrentBuffer )
    {
        if ( !m_isPlayingFlag )
            return 0;
        else
        {
            m_decayCount += 1;
            if ( m_decayCount >= m_decayToSilenceSamps )
                m_decayTable.startRunningEnvelope();
        }
        
        auto outputSamp = m_excitation.outputExcitation();
        if ( m_harmBalance > 0 )
        {
            m_emb.setSample2( m_lastOutSamp * m_harmBalance );
            outputSamp = sjf_cubic< double >( outputSamp + ( m_emb.getSample2() ) );
        }
        // apply nonlinearity
        m_lastOutSamp = applyNonlinearities( m_lastOutSamp*( 1.0 - m_harmBalance) + outputSamp );
        if( m_decayTable.getIsRunning() )
        {
            auto amp = m_decayTable.outputEnvelope();
            if( amp.has_value() )
                m_lastOutSamp *= amp.value();
            else
            {
                m_isPlayingFlag = false;
                for ( auto& lpf : m_mediumDampers )
                    lpf.reset();
                for ( auto& wg : m_waveguide )
                    wg.clear();
                m_emb.clearDelayline();
                m_apNonLin.clear();
                return 0;
            }
        }
        if ( m_onlyOneDirection )
        {
            m_lastOutSamp = m_mediumDampers[ 1 ].filterInput( ( m_mediumDampers[ 0 ].filterInput( m_waveguide[ 0 ].process( m_lastOutSamp ) ) ) ) * m_fb;
            outputSamp += m_lastOutSamp;
            return outputSamp;
        }
        for ( auto i = 0; i < m_waveguide.size(); i++ )
        {
            m_lastOutSamp = ( m_mediumDampers[ i ].filterInput( m_waveguide[ i ].process( m_lastOutSamp ) ) ) * m_fb;
            outputSamp += m_waveguide[ i ].sensorOutput();
        }
        m_lastOutSamp = m_oddHarmonicsFlag ? -m_lastOutSamp : m_lastOutSamp;
        return outputSamp;
    }
    
    // the split of total delay time between forward and backward travelling waves
    void setSplit( double split )
    {
#ifndef NDEBUG
        assert( split > 0 && split < 1 );
#endif
        if ( m_split == split )
            return;
        m_split = split;
    }
    
    double getSplit( )
    {
        return m_split;
    }
    
    // the stiffness of the medium
    void setStiffness( double stiff )
    {
#ifndef NDEBUG
        assert( stiff > -1 && stiff < 1 );
#endif
        if ( m_stiff == stiff )
            return;
        m_stiff = stiff;
    }
    
    double getStiffness( )
    {
        return m_stiff;
    }
    
    // the position of the pickup
    void setSensorPosition( double sensorPos )
    {
#ifndef NDEBUG
        assert( sensorPos > 0 && sensorPos < 1 );
#endif
        if ( m_sensorPos == sensorPos )
            return;
        m_sensorPos = sensorPos;
    }
    
    double getSensorPos( )
    {
        return m_sensorPos;
    }
    
    // the decay by 60dB in seconds
    void setDecay( double decay )
    {
#ifndef NDEBUG
        assert( decay > 0 );
#endif
        if ( m_decaySeconds == decay )
            return;
        m_decaySeconds = decay;
    }
    
    double getDecay( )
    {
        return m_decaySeconds;
    }
    
    // the brighness of the medium
    void setMediumBrightness( double bright )
    {
#ifndef NDEBUG
        assert( bright >= 0 && bright <= 1 );
#endif
        if ( m_mediumBrightness == bright )
            return;
        m_mediumBrightness = bright;
    }
    
    double getMediumBrightness( )
    {
        return m_mediumBrightness;
    }
    
    // the pick position along the string
    void setPickPosition( double pickPos )
    {
#ifndef NDEBUG
        assert( pickPos > 0 && pickPos < 1 );
#endif
        if ( m_pickPos == pickPos )
            return;
        m_pickPos = pickPos;
    }
    
    double getPickPosition( )
    {
        return m_pickPos;
    }
    
    void setExciteNoiseLevel( double noiseLevel )
    {
#ifndef NDEBUG
        assert( noiseLevel >= 0 && noiseLevel <= 1 );
#endif
        m_exciteNoiseLevel = noiseLevel;
    }
    
    double getExciteNoiseLevel( )
    {
        return m_exciteNoiseLevel;
    }
    
    void setExciteEnvelopeParams( double centrePoint, double power )
    {
#ifndef NDEBUG
        assert( centrePoint >= 0 && centrePoint <= 1 );
        assert( power > 0 );
#endif
        m_exciteEnvCentre = centrePoint;
        m_exciteEnvExponent = power;
        m_excitation.setEnvelope( m_exciteEnvCentre, m_exciteEnvExponent );
    }
    
    double getExciteEnvelopeCentre( )
    {
        return m_exciteEnvCentre;
    }
    
    double getExciteEnvelopeExponent( )
    {
        return m_exciteEnvExponent;
    }
    
    
    std::vector< double >& getExciteEnvelope()
    {
        return m_excitation.getEnvelope();
    }
    
    bool isBusy()
    {
        return m_isPlayingFlag;
    }
    
    // extend excitation as factor of period
    // usefule for "bowed" sounds
    void setExcitationLengthFactor( double lengthFactor )
    {
#ifndef NDEBUG
        assert( lengthFactor >= 1 );
#endif
        m_exciteExtension = lengthFactor;
    }
    
    double getExcitationLengthFactor( )
    {
        return m_exciteExtension;
    }
    
    void setNonLinearity( bool nonLin1ShouldBeOn )
    {
        m_nonLinFlag1 = nonLin1ShouldBeOn;
    }
    
    void setNonLin1Factor( double a )
    {
#ifndef NDEBUG
        assert( a >= 0 && a <= 0.99 );
#endif
        m_apnonLinA = -a;
    }
    
    double getNonLin1Factor()
    {
        return abs( m_apNonLinANeg );
    }
    
    void setHarmonic( double a, int harmNumber )
    {
#ifndef NDEBUG
        assert( a >= 0 && a <= 1 );
        assert( harmNumber > 1 );
#endif
        m_harmLevel = a;
        m_harmNumber = harmNumber;
    }

    double getHarmonicLevel()
    {
        return m_harmLevel;
    }
    
    void setExcitationCutoff( double f )
    {
        m_exciteNoiseBrightness = calculateLPFCoefficient( f, m_SR );
        m_excitation.setLPFCoef( m_exciteNoiseBrightness );
    }
    
    void setOddHarmonicsOnly( bool shouldBeOn )
    {
        m_oddHarmonicsFlag = shouldBeOn;
    }
    
private:
    void newNote( double midiPitch, double midiVelocity, double split, double stiff, double sensor, double pickPos, double decay, double medBright, double exciteNoiseLevel, double exciteEnvCentre, double exciteEnvExponent, double exciteEnvExtension, double exciteNoiseBright, double nonLinA, double harmonicLevel, double harmonicNumber )
    {
        
        DBG( split );
        auto amplitude = midiVelocity / 127.0;
        auto fundamental = midiToFrequency< double >( midiPitch );
        
        auto fCutOff = std::fmin(fundamental * std::pow( 2, ( medBright * 6.5 ) + 2 ), 20000 );
        medBright = calculateLPFCoefficient< double > ( fCutOff, m_SR );
        for ( auto& lpf : m_mediumDampers )
            lpf.setCoefficient( medBright );
        auto apd = sjf_filterResponse< double >::calculateFilterResponse( fundamental, m_SR, { medBright }, { 1.0, medBright - 1.0 } );
        DBG( "del " << apd[2] );
        auto delayCompensation = -2.0 * apd[ 2 ]; // tuning compensation for delays and feedback sections --> need to calculate properly
        
        nonLinA *= (1.0 - medBright)*-1.0;
        if ( m_nonLinFlag1 )
        {
            apd = sjf_filterResponse< double >::calculateFilterResponse( fundamental, m_SR, { nonLinA, 1.0 }, { 1.0, nonLinA } );
            delayCompensation -= apd[ 2 ] * 0.5;
            apd = sjf_filterResponse< double >::calculateFilterResponse( fundamental, m_SR, { -nonLinA, 1.0 }, { 1.0, -nonLinA } );
            delayCompensation -= -apd[ 2 ] * 0.5;
        }
        auto periodSeconds = 1.0 / fundamental;
        auto periodSamples = periodSeconds * m_SR;
        auto decayComp = ( 1.0 - abs( stiff ) ) * 0.001; // at 0 stiffness decay is time in seconds for drop by 60dB ( i.e. *0.001 )
        m_fb = -1.0 * std::pow( decayComp, ( periodSeconds/(decay*2)) );
        m_periodSamples = periodSamples + delayCompensation;
        auto p1 = (m_periodSamples * split);
        auto p2 = m_periodSamples - p1;
        auto sensor1 = sensor;
        auto sensor2 = 1.0 - sensor;
        auto minPeriod = (m_SR*0.0005);
        m_onlyOneDirection = false;
        // how do I compensate for allpass in delay lines???
        if ( p1 < minPeriod || p2 < minPeriod )
        {
            if (  m_periodSamples < minPeriod )
            {
                DBG("ONLY ONE");
                m_onlyOneDirection = true;
                p1 = p2 = m_periodSamples;
                sensor1 = sensor2 = 0.5;
                m_fb = std::pow( decayComp, ( periodSeconds/(decay)) );
                
//                m_waveguide[ 0 ].setDelayInSamps( m_periodSamples );
//                m_waveguide[ 0 ].setSensorPosition( 0.5 );
//                m_waveguide[ 0 ].setG( stiff );
//                m_fb = std::pow( decayComp, ( periodSeconds/(decay)) );
            }
            else if ( m_periodSamples * 0.5 < minPeriod )
            {
                DBG("Half period");
                p1 = p2 = m_periodSamples * 0.5;
                sensor1 = sensor2 = 0.5;
//                for ( auto& wg : m_waveguide )
//                {
//                    wg.setDelayInSamps( m_periodSamples * 0.5 );
//                    wg.setSensorPosition( 0.5 );
//                    wg.setG( stiff );
//                }
            }
            else if ( p1 < minPeriod )
            {
                p1 = minPeriod;
                p2 = m_periodSamples - minPeriod;
//                m_waveguide[ 0 ].setDelayInSamps( minPeriod );
//                m_waveguide[ 0 ].setSensorPosition( sensor );
//                m_waveguide[ 0 ].setG( stiff );
//                m_waveguide[ 1 ].setDelayInSamps( m_periodSamples - minPeriod );
//                m_waveguide[ 1 ].setSensorPosition( 1 - sensor );
//                m_waveguide[ 1 ].setG( stiff );
            }
            else if ( p2 < minPeriod )
            {
                p2 = minPeriod;
                p1 = m_periodSamples - minPeriod;
//                m_waveguide[ 0 ].setDelayInSamps( m_periodSamples - minPeriod );
//                m_waveguide[ 0 ].setSensorPosition( sensor );
//                m_waveguide[ 0 ].setG( stiff );
//                m_waveguide[ 1 ].setDelayInSamps( minPeriod );
//                m_waveguide[ 1 ].setSensorPosition( 1 - sensor );
//                m_waveguide[ 1 ].setG( stiff );
            }
        }
//        else
//        {
            m_waveguide[ 0 ].setDelayInSamps( p1 );
            m_waveguide[ 0 ].setSensorPosition( sensor1 );
            m_waveguide[ 0 ].setG( stiff );
            m_waveguide[ 1 ].setDelayInSamps( p2 );
            m_waveguide[ 1 ].setSensorPosition( sensor2 );
            m_waveguide[ 1 ].setG( stiff );
//        }

        m_emb.setDelayTimeSamps( m_periodSamples / harmonicNumber );
        m_harmBalance = harmonicLevel;
        
        m_excitation.setLPFCoef( exciteNoiseBright );
        m_excitation.setEnvelope( exciteEnvCentre, exciteEnvExponent );
        m_excitation.triggerNewExcitation( m_periodSamples * exciteEnvExtension, amplitude, exciteNoiseLevel );
        
        
        
        m_apNonLin.setA( nonLinA, -nonLinA );
        
        m_decayCount = 0;
        m_decayToSilenceSamps = (decay * m_SR * 2);
        m_isPlayingFlag = true;
        
        m_decayTable.reset();

    }
    
    double applyNonlinearities( double x )
    {
        if ( m_nonLinFlag1 )
        {
            return sjf_softClip< double >( ( m_apNonLin.process( x ) ) );
        }
        return x;
    }
    
    std::array< sjf_waveguideAllpass< double >, 2 > m_waveguide ;
    sjf_delayLine< double > m_emb;

    std::array< sjf_lpf< double >, 2 > m_mediumDampers;
    sjf_pmExcitation< double > m_excitation;
    sjf_asymAllpass< double > m_apNonLin;
    
    double m_SR = 44100;
    double m_split = 0.5, m_stiff = 0, m_sensorPos = 0.2, m_decaySeconds = 1, m_mediumBrightness = 0.7, m_pickPos = 0.1, m_exciteNoiseLevel = 1, m_exciteEnvCentre = 0.1, m_exciteEnvExponent = 1, m_exciteNoiseBrightness = 1, m_apnonLinA = 0, m_apNonLinANeg = 0, m_harmLevel = 0, m_harmNumber = 2, m_harmBalance = 0;
    bool m_oddHarmonicsFlag = false;
    double m_fb = 0.99, m_periodSamples = 100, m_exciteExtension = 1, m_exciteCoef = 0.5;
    double m_lastOutSamp = 0;
    
    int m_decayCount = 0, m_decayToSilenceSamps = (m_decaySeconds * m_SR * 2);
    bool m_isPlayingFlag = false, m_onlyOneDirection = false;
    bool m_nonLinFlag1 = false;
    
    sjf_decayTable< double > m_decayTable;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_waveguide )
};

#endif /* sjf_waveguide_h */
