//
//  sjf_verb.h
//
//  Created by Simon Fay on 17/10/2022.
//

#ifndef sjf_verb_h
#define sjf_verb_h

#include "sjf_comb.h"
#include "sjf_audioUtilitiesC++.h"
#include "sjf_circularBuffer.h"


// add different early reflection types
// add modulation
// stereo processing

//=================================================
//=================================================
//=================================================

template < typename T >
class sjf_nestedAP
{
private:
    sjf_multiVoiceCircularBuffer< T, 2 > m_circBuff;
    std::array< sjf_lpf2< T >, 2 > m_lpfs;
    std::array< T, 2 > m_delayTimesSamps, m_delayedSamples, m_xhns, m_gains;
    std::array< size_t, 2 > m_delayTimesSampsRounded;
    T m_gain = 0.5, m_damp = 0.1;
    int m_nStages = 0; // this determines the number nested steps
    bool m_shouldFilter = false;
public:
    sjf_nestedAP()
    {
        initialise( 44100 );
        setAllCoefficients( 0.7, true );
    }
    ~sjf_nestedAP(){}
    
    void initialise( T maxLengthPerVoiceInSamps )
    {
        m_circBuff.initialise( maxLengthPerVoiceInSamps );
    }
    
    T process( T input, bool delayTimeIsFractional = false )
    {
        std::array< T, 2 > delTaps;
        for ( auto i = 0; i < 2; i++ )
        {
            auto dt = delayTimeIsFractional ? m_delayTimesSamps[ i ] : m_delayTimesSampsRounded[ i ];
            delTaps[ i ] = m_shouldFilter ? m_lpfs[ i ].filterInput( m_circBuff.getSample( dt, i )  ) : m_circBuff.getSample( dt, i ) ;
        }

        sjf_oneMultiplyScatterJuntion< T >( input, delTaps[ 0 ], m_gains[ 0 ] );
        sjf_oneMultiplyScatterJuntion< T >( input, delTaps[ 1 ], m_gains[ 1 ] );
        
        m_circBuff.setSample( input, 1 );
        m_circBuff.setSample( delTaps[ 1 ], 0 );
        
        m_circBuff.updateWritePosition();
        return delTaps[ 0 ];
        
    }
    
    
    void setDelayTimeSamps( T delayTimeSamps, int voiceNum )
    {
#ifndef NDEBUG
        assert( voiceNum >= 0 );
        assert( voiceNum <= 1 );
#endif
        m_delayTimesSamps[ voiceNum ] = delayTimeSamps;
        m_delayTimesSampsRounded[ voiceNum ] = delayTimeSamps;
    }
    
    void setInterpolationType( int interpType )
    {
        m_circBuff.setInterpolationType( interpType );
    }
    
    
    void setAllCoefficients( T gain, bool shouldAlternatePolarity )
    {
        m_gain = gain;
        if ( !shouldAlternatePolarity )
        {
            std::fill( std::begin( m_gains ), std::end( m_gains ),  m_gain );
        }
        else
        {
            m_gains[ 0 ] = m_gain;
            m_gains[ 1 ] = -1*m_gain;
        }

        setAllLPFCoefficients( m_damp );
    }
    
    void setShouldFilter( bool trueIfFilterIsOn )
    {
        m_shouldFilter = trueIfFilterIsOn;
    }
    
    
    void setAllLPFCoefficients( T lpfCoef )
    {
        m_damp = lpfCoef;
        for ( auto v = 0; v < m_lpfs.size(); v++ )
            m_lpfs[ v ].setCoefficient( m_damp );
    }
};

//=================================================
//=================================================
//=================================================

// diffusion a la geraint luff https://signalsmith-audio.co.uk/writing/2021/lets-write-a-reverb/
template < typename T, int NVOICES, int NSTAGES >
class sjf_glDiffuser
{
private:
    sjf_multiVoiceCircularBuffer< T, NVOICES * NSTAGES > m_circBuff;
    std::array< std::array< T, NVOICES >, NSTAGES > m_delayTimesSamps;
    std::array< std::array< size_t, NVOICES >, NSTAGES > m_delayTimesSampsRounded;
    std::array< std::array< sjf_lpf2< T >, NVOICES >, NSTAGES > m_lpfs;
    std::array< T, NVOICES > m_samps;
    T m_damp = 0;
    bool m_shouldFilter = false;
public:
    sjf_glDiffuser(){}
    ~sjf_glDiffuser(){}
    
    void initialise( size_t maxDelayTimeInSamples )
    {
        m_circBuff.initialise( maxDelayTimeInSamples );
    }
    
    void processInPlace( T* input, bool delayTimeIsFractional = false )
    {
        for ( auto i = 0; i < NSTAGES; i++ )
        {
            for ( auto j = 0; j < NVOICES; j++ )
            {
                auto buffVoice = j + ( i * NSTAGES );
                m_circBuff.setSample( input[ j ], buffVoice );
                auto dt = delayTimeIsFractional ? m_delayTimesSamps[ i ][ j ] : m_delayTimesSampsRounded[ i ][ j ];
                input[ j ] = m_shouldFilter ? m_lpfs[ i ][ j ].filterInput( m_circBuff.getSample( dt, buffVoice ) ) : m_circBuff.getSample( dt, buffVoice );
            }
            Hadamard< T, NVOICES >::inPlace( input );
        }
        m_circBuff.updateWritePosition();
    }
    
    T process( T input, bool delayTimeIsFractional = false )
    {
        std::fill( m_samps.begin(), m_samps.end(), input );
        for ( auto i = 0; i < NSTAGES; i++ )
        {
            for ( auto j = 0; j < NVOICES; j++ )
            {
                auto buffVoice = j + ( i * NSTAGES );
                m_circBuff.setSample( m_samps[ j ], buffVoice );
                auto dt = delayTimeIsFractional ? m_delayTimesSamps[ i ][ j ] : m_delayTimesSampsRounded[ i ][ j ];
                m_samps[ j ] = m_shouldFilter ? m_lpfs[ i ][ j ].filterInput( m_circBuff.getSample( dt, buffVoice ) ) : m_circBuff.getSample( dt, buffVoice );
            }
            Hadamard< T, NVOICES >::inPlace( m_samps.data() );
        }
        m_circBuff.updateWritePosition();
        return m_samps[ 0 ];
    }
    
    void setDelayTimeSamps( T delayTimeSamps, int voiceNum, int stage )
    {
        m_delayTimesSamps[ stage ][ voiceNum ] = delayTimeSamps;
        m_delayTimesSampsRounded[ stage ][ voiceNum ] = static_cast< size_t >( delayTimeSamps );
    }
    
    void setAllLPFCoefficients( T lpfCoef )
    {
        m_damp = lpfCoef;
        for ( auto & lpfStage : m_lpfs )
            for ( auto & lpf : lpfStage )
                lpf.setCoefficient( m_damp );
    }
    
    void setShouldFilter( bool trueIfFilterShouldBeOn )
    {
        m_shouldFilter = trueIfFilterShouldBeOn;
    }
    
private:
};

//=================================================
//=================================================
//=================================================
template < typename T, int NVOICES >
class sjf_FDN
{
private:
    sjf_multiVoiceCircularBuffer< T, NVOICES > m_circBuff;
    std::array< sjf_lpf2< T >, NVOICES > m_lpfs;
    T m_damp = 0.1;
    std::array< T, NVOICES > m_delayTimesSamps, m_gains;
    std::array< size_t, NVOICES > m_delayTimesSampsRounded;
    bool m_shouldFilter = false;
    bool m_shouldMix = false;
    static constexpr T m_eps = std::numeric_limits<T>::epsilon();
    static constexpr T m_outScale = 1.0 / gcem::pow( 2.0, (gcem::log(static_cast<T>(NVOICES) ) / gcem::log(2.0)) / 2.0 );
public:
    sjf_FDN()
    {
        initialise( 44100 );
    }
    ~sjf_FDN(){}
    
    void initialise( size_t maxDelayInSamps )
    {
        m_circBuff.initialise( maxDelayInSamps );
    }
    
    T process ( T input, bool delayTimeIsFractional = false )
    {
        std::array< T, NVOICES > delayedSamps;
        for ( auto v = 0; v < NVOICES; v++ )
        {
            auto dt = delayTimeIsFractional ? m_delayTimesSamps[ v ] : m_delayTimesSampsRounded[ v ];
            delayedSamps[ v ] = m_shouldFilter ? m_lpfs[ v ].filterInput( m_circBuff.getSample( dt, v ) ) : m_circBuff.getSample( dt, v );
        }
        auto output = std::accumulate( delayedSamps.begin(), delayedSamps.end(), 0.0 );
        output *= m_outScale;
        
        if( m_shouldMix )
            Householder< T, NVOICES >::mixInPlace( delayedSamps );
        
        for ( auto v = 0; v < NVOICES; v++ )
            m_circBuff.setSample( input + delayedSamps[ v ] * m_gains[ v ], v );
        m_circBuff.updateWritePosition();
        return output;
    }
    
    void setDelayTimeSamps( T delayTimeSamps, int voiceNum )
    {
        m_delayTimesSamps[ voiceNum ] = delayTimeSamps;
        m_delayTimesSampsRounded[ voiceNum ] = static_cast< size_t >( delayTimeSamps );
    }
    
    T getDelayTimeSamps( int voiceNum )
    {
        return m_delayTimesSamps[ voiceNum ];
    }
    
    void setFeedbackGain( T fb, int voiceNum )
    {
        m_gains[ voiceNum ] = fb;
    }
    
    void setInterpolationType( int interpType )
    {
        m_circBuff.setInterpolationType( interpType );
    }
    
    void setLPFCoefficient( T lpfCoef )
    {
        m_damp = lpfCoef;
        for ( auto v = 0; v < NVOICES; v++ )
            m_lpfs[ v ].setCoefficient( m_damp );
    }
    
    void setShouldFilter( bool trueIfFilterShouldBeOn )
    {
        m_shouldFilter = trueIfFilterShouldBeOn;
    }
    
    void setShouldMix( bool trueIfShouldMix )
    {
        m_shouldMix = trueIfShouldMix;
    }
};

//=================================================
//=================================================
//=================================================

class sjf_verb
{
private:
    static constexpr int N_LATE_REFLECT = 16;
    static constexpr int N_EARLY_VOICES = 8;
    static constexpr int N_GL_STAGES = 4;
    static constexpr int PRIME_MAX = 5000;
    
    sjf_FDN< float, N_LATE_REFLECT > m_fdn;
    
    
    int m_earlyType = earlyTypes::nestedAP;
    std::array< sjf_nestedAP<float>, (N_EARLY_VOICES / 2) > m_early;
    sjf_glDiffuser< float, N_EARLY_VOICES, N_GL_STAGES > m_early2;
    std::vector< int > m_primeNumbers;
    size_t m_roomType = 0;
    float m_volume = 100000; // m^3
    float m_SR = 44100;
    float m_LRDecaySeconds = 2;
    bool m_erParallel = false;
    float m_erToL = 1, m_dryToL = 0, m_erOutLevel = 1, m_lateOutLevel = 1;
    // ratios from here: https://radiobombfm.wordpress.com/2012/09/15/sound-101-the-golden-acoustic-ratio/
    static constexpr std::array< std::array< float, 7 >, 2 > m_roomRatios =
     { {
         { 1.39, 1.59, 1.54, 1.9, 1.9, 2.5, 2.33 },
         { 1.14, 1.26, 1.28, 1.3, 1.4, 1.5, 1.6 }
     } } ;
    
    sjf_PQR< 10 > m_pqrList;
    std::array< std::vector< int >, 7 > m_pqrIndices;
    static constexpr std::array< std::array< float, 3 >, N_LATE_REFLECT > m_roomModePQR =
    { {
        { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 }, // axial modes
        { 1, 1, 0 }, { 1, 0, 1 }, { 0, 1, 1 }, { 2, 1, 0 }, // tangential modes
        { 1, 1, 1 } // oblique mode
    } };
    
    static constexpr std::array< float, N_LATE_REFLECT > m_lateRScale =
    { 1, 1, 1, 0.5, 0.5, 0.5, 0.5, 0.25 };
    
    
public:
    sjf_verb()
    {
        initialisePQRIndices();
        fillPrimeNumberVector( PRIME_MAX );
        setRoomVolume( m_volume );
        DBG( "INITIALISE SJF_VERB");
        
        for ( auto i = 0; i < (N_EARLY_VOICES / 2); i++ )
        {
            m_early[ i ].setAllCoefficients( 0.7, false );
            m_early[ i ].setShouldFilter( true );
        }
        
        m_fdn.setShouldFilter( true );
        setLRLPFCoef( 0.186 );
    }
    ~sjf_verb(){}
    
    void initialise( float sampleRate )
    {
        if ( sampleRate <= 0 )
            return;
        m_SR = sampleRate;
        
        
        for ( auto & er : m_early )
            er.initialise( m_SR * 0.25 );
        m_early2.initialise( m_SR * 0.25 );
        m_fdn.initialise( m_SR * 0.5 );
    }
    
    float process( float input )
    {
        auto er = input;
        switch ( m_earlyType )
        {
            case earlyTypes::nestedAP :
                for ( auto & early : m_early )
                    er = early.process( er );
                break;
            case earlyTypes::geraintLuff :
                er = m_early2.process( er );
                break;
            default:
                for ( auto & early : m_early )
                    er = early.process( er );
                break;
        }

        auto toLate = ( input * m_dryToL ) + ( er * m_erToL );
        float lr = m_fdn.process( toLate, true );
        return ( lr * m_lateOutLevel ) + ( er * m_erOutLevel );
    }
    
    void setRoomVolume( float volInMetersCubed )
    {
        if ( volInMetersCubed == m_volume )
            return;
        m_volume = volInMetersCubed;
        calculateDelayTimes();
    }
    
    void setRoomType( size_t roomType )
    {
        if ( roomType == m_roomType )
            return;
        m_roomType = roomType;
        calculateDelayTimes();
    }
    
    void setLRDecay( float decayInSeconds )
    {
        if ( decayInSeconds == m_LRDecaySeconds )
            return;
        m_LRDecaySeconds = decayInSeconds;
        setLateFBCoefficients();
    }
    
    void setLRLPFCoef( float LRLPFCoef )
    {
        m_fdn.setLPFCoefficient( LRLPFCoef );
    }

    void setERLPFCoef( float ERLPFCoef )
    {
        for ( auto & early : m_early )
            early.setAllLPFCoefficients( ERLPFCoef );
    }
    
    
    int getNumRoomTypes()
    {
        return m_roomRatios[ 0 ].size();
    }
    
    void setEarlyToLate( float earlyToLatePercentage )
    {
#ifndef NDEBUGl
        assert( earlyToLatePercentage >= 0 );
        assert( earlyToLatePercentage <= 100 );
#endif
        auto e2L = earlyToLatePercentage * 0.01;
        m_erToL = std::sqrt( e2L );
        m_dryToL = std::sqrt( 1.0 - e2L );
    }

    void setEarlyOutputLevel( float erOutPercentage )
    {
#ifndef NDEBUG
        assert( erOutPercentage >= 0 );
        assert( erOutPercentage <= 100 );
#endif
        m_erOutLevel = std::sqrt( erOutPercentage * 0.01 );
    }
    
    void setLateOutputLevel( float lateOutPercentage )
    {
#ifndef NDEBUG
        assert( lateOutPercentage >= 0 );
        assert( lateOutPercentage <= 100 );
#endif
        m_lateOutLevel = std::sqrt( lateOutPercentage * 0.01 );
    }
    
    void setERParallel( bool trueIfParallelFalseIfseries )
    {
        m_erParallel = trueIfParallelFalseIfseries;
    }
    
    void shouldMixLate( bool trueIfShouldixLate )
    {
        m_fdn.setShouldMix( trueIfShouldixLate );
    }
    
    void setEarlyType( int earlyType )
    {
        if ( m_earlyType != earlyType )
        {
            m_earlyType = earlyType;
            calculateDelayTimes();
        }
    }
    
    enum earlyTypes { nestedAP, geraintLuff };
    
private:
    void initialisePQRIndices()
    {
        for ( auto & i : m_pqrIndices )
            i.resize( m_pqrList.getSize() );
        std::vector< float > modes, mSorted;
        modes.resize( m_pqrList.getSize() );
        mSorted.resize( m_pqrList.getSize() );
        for ( auto room = 0; room < m_roomRatios[0].size(); room ++ )
        {
            for ( auto i = 0; i < m_pqrList.getSize(); i++ )
            {
                auto pqr = m_pqrList[ i ];
                auto L = m_roomRatios[ 0 ][ room ];
                auto W = m_roomRatios[ 1 ][ room ];
                auto f = sjf_calculateRoomMode<float>( L, W, 1.0, pqr[ 0 ], pqr[ 1 ], pqr[ 2 ] );
                mSorted[i] = modes[i] = f;
            }
            std::sort(mSorted.begin(), mSorted.end());

            for ( auto i = 0; i < m_pqrList.getSize(); i++ )
            {
                auto it = find(modes.begin(), modes.end(), mSorted[ i ] );
                m_pqrIndices[room][i] = static_cast<int>( it - modes.begin() );
            }
        }
    }
    
    void fillPrimeNumberVector( int primeMax )
    {
        m_primeNumbers.reserve( primeMax );
        for ( auto i = 0; i < primeMax; i++ )
        {
            if( sjf_isPrime( i ) )
                m_primeNumbers.emplace_back( i );
        }
        m_primeNumbers.shrink_to_fit();
    }
    
    void calculateDelayTimes()
    {
        auto nPrimes = m_primeNumbers.size();
        std::vector< bool > primeList; // use this to keep track of which primeNumbers have been used
        primeList.resize( nPrimes, true ); // keep a rtack of which primeNumbers are available
        std::array< float, N_LATE_REFLECT > roomModesInSamps;
        float L, W, H;
        float ratioX, ratioY;
        ratioX = m_roomRatios[ 0 ][ m_roomType ];
        ratioY = m_roomRatios[ 1 ][ m_roomType ];
        
        H = std::pow( m_volume / ( ratioX * ratioY ), ( 1.0 / 3.0 ) );
        W = H * ratioY;
        L = H * ratioX;
        for ( auto i = 0; i < N_LATE_REFLECT; i++ )
        {
            auto pqrIndex = m_pqrIndices[m_roomType][ i ];
            auto P = m_pqrList[ pqrIndex ][ 0 ];
            auto Q = m_pqrList[ pqrIndex ][ 1 ];
            auto R = m_pqrList[ pqrIndex ][ 2 ];
            auto f = sjf_calculateRoomMode( L, W, H, P, Q, R );
            auto T = 1.0 / f;
            roomModesInSamps[ i ] = T * m_SR;
        }
        
        switch ( m_earlyType ) {
            case earlyTypes::nestedAP :
                calculateNestedAPDelays( roomModesInSamps[ N_LATE_REFLECT - 1 ], primeList );
                break;
            case earlyTypes::geraintLuff :
                calculateGLDelays( roomModesInSamps[ N_LATE_REFLECT - 1 ], primeList );
            default:
                calculateNestedAPDelays( roomModesInSamps[ N_LATE_REFLECT - 1 ], primeList );
                break;
        }
        
        
        for ( auto i = N_LATE_REFLECT-1 ; i >= 0; i-- )
        {
            auto dtSamps = findNearestPrimeDelay( roomModesInSamps[ i ], primeList );
            m_fdn.setDelayTimeSamps( dtSamps,  i );
        }
        setLateFBCoefficients();

    }
    
    void setLateFBCoefficients()
    {
        for ( auto i = 0; i < N_LATE_REFLECT; i++ )
        {
            auto fb = std::pow( 10, -3.0 * (static_cast<float>( m_fdn.getDelayTimeSamps( i ) ) / m_SR) / m_LRDecaySeconds );
            m_fdn.setFeedbackGain( fb, i ); // what about scaling by mode type?!?!
        }
    }
    
    void calculateNestedAPDelays( float modePersiodSamples, std::vector< bool >& primeList )
    {
        DBG("modePersiodSamples " << modePersiodSamples );
        auto maxERAllowed = (m_SR * 0.1) * 1.2;
//        auto minER = std::round(m_SR * 0.001);
        while ( modePersiodSamples > maxERAllowed )
            modePersiodSamples = std::pow( modePersiodSamples, 0.95 );
        //    calculate nested allpass delays
//        auto baseDelay = modePersiodSamples / static_cast< float >( N_EARLY_VOICES );
//        for ( int i = N_EARLY_VOICES - 1; i >= 0; i-- )
//        {
//            int desiredDelay = std::round( modePersiodSamples / static_cast< float >( std::pow( 3.0, i ) ) );
//            int delay = findNearestPrimeDelay( desiredDelay, primeList );
//            auto v = static_cast<int>(i / 2);
//            auto a = 1 - (i%2);
//            DBG( v << " " << a << " " << delay << " " << desiredDelay );
//            m_early[ v ].setDelayTimeSamps( delay, a );
//        }

        for ( int i = 0; i < N_EARLY_VOICES; i++ )
        {
//            int desiredDelay = (baseDelay * i) + (baseDelay * static_cast< float >( (N_EARLY_VOICES-i) / static_cast< float >( N_EARLY_VOICES ) ) );
            int desiredDelay = 2 + std::pow( modePersiodSamples, static_cast< float >(i+1) / static_cast< float >( N_EARLY_VOICES ) );
            int delay = findNearestPrimeDelay( desiredDelay, primeList );
            auto v = static_cast<int>(i / 2);
            auto a = i % 2;
            DBG( v << " " << a << " " << delay << " " << desiredDelay );
            m_early[ v ].setDelayTimeSamps( delay, a );
        }
    }
    
    void calculateGLDelays( float modePersiodSamples, std::vector< bool >& primeList )
    {
        // each stage should be twice the length of the last
        // ==> need to divide the total length by 2^NVOICES - 1
        static constexpr float divisor = 1.0 / ( gcem::pow( 2.0, N_EARLY_VOICES ) - 1 );
        auto stageLength = modePersiodSamples * divisor;
        for ( int s = 0; s < N_GL_STAGES; s++ )
        {
            stageLength *= s == 0 ? 1 : 2;
            for ( int i = 0; i < N_EARLY_VOICES; i++ )
            {
                int desiredDelay = 2 + std::pow( stageLength, static_cast< float >( i+1 ) / static_cast< float >( N_EARLY_VOICES ) );
                int delay = findNearestPrimeDelay( desiredDelay, primeList );
                
                auto voice = i + ( N_GL_STAGES - s);
                voice %= N_EARLY_VOICES;
                DBG( "gl " << stageLength<< " " << desiredDelay << " " << delay << " " << voice << " " << s );
                m_early2.setDelayTimeSamps( delay, i, s );
            }
        }
        
    }
    
    int findNearestPrimeDelay( int desiredDelay, std::vector< bool >& primeList )
    {
        auto delayDifference = desiredDelay; // placeholder starting point
        auto primeDelay = desiredDelay;
        auto primeIndex = 0;
        for ( auto p = 0; p < primeList.size(); p++ )
        {
            if ( primeList[ p ] )
            {
                auto prime = m_primeNumbers[ p ];
                auto mi = std::round( std::log( desiredDelay ) / std::log( prime ) );
                auto pDel = std::pow( prime, mi );
                auto dif = std::abs(  pDel - desiredDelay );
                if ( dif <= delayDifference )
                {
                    primeDelay = pDel;
                    primeIndex = p;
                    delayDifference = dif;
                }
            }
        }
        primeList[ primeIndex ] = false;
        return primeDelay;
    }
    
};
#endif /* sjf_verb_h */
