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


template < typename T, int NVOICES >
class sjf_nestedAllpass
{
private:
    sjf_multiVoiceCircularBuffer< T, NVOICES > m_circBuff;
    std::array< sjf_lpf2< T >, NVOICES > m_lpfs;
    std::array< T, NVOICES > m_delayTimesSamps, m_delayedSamples, m_xhns, m_gains;
    std::array< size_t, NVOICES > m_roundedDelays;
    T m_gain = 0.5, m_parallelInputScale, m_damp = 0.1;
    int m_nStages = 0; // this determines the number nested steps
    std::vector< int > m_stages;
    bool m_shouldFilter = false;
public:
    sjf_nestedAllpass()
    {
        initialise( 44100 );
        setNumNestedStages( NVOICES / 2 );
        setAllCoefficients( 0.7, true );
    }
    ~sjf_nestedAllpass(){}
    
    void initialise( T maxLengthPerVoiceInSamps )
    {
        m_circBuff.initialise( maxLengthPerVoiceInSamps );
    }
      
    T process( T input, bool delayTimeIsFractional = false )
    {
        size_t voiceCount = 0;
        for ( auto j = 0; j < m_nStages; j++ )
        {
            for ( auto i = 0; i < m_stages[ j ] - 1; i++ )
            {
                // get delayed signal at this stage
                m_delayedSamples[ voiceCount ] = delayTimeIsFractional ? m_circBuff.getSample( m_delayTimesSamps[ voiceCount ], voiceCount ) : m_circBuff.getSample( m_roundedDelays[ voiceCount ], voiceCount );
                // to filter or not to filter
                m_delayedSamples[ voiceCount ] = m_shouldFilter ? m_lpfs[ voiceCount ].filterInput( m_delayedSamples[ voiceCount ] ) : m_delayedSamples[ voiceCount ];
                // one multiply value
                m_xhns[ voiceCount ] = ( input - m_delayedSamples[ voiceCount ] ) * m_gains[ voiceCount ];
                input += m_xhns[ voiceCount ];
                voiceCount += 1;
            }
            m_delayedSamples[ voiceCount ] = delayTimeIsFractional ? m_circBuff.getSample( m_delayTimesSamps[ voiceCount ], voiceCount ) : m_circBuff.getSample( m_roundedDelays[ voiceCount ], voiceCount );
            m_delayedSamples[ voiceCount ] = m_shouldFilter ? m_lpfs[ voiceCount ].filterInput( m_delayedSamples[ voiceCount ] ) : m_delayedSamples[ voiceCount ];
            m_xhns[ voiceCount ] = ( input - m_delayedSamples[ voiceCount ] ) * m_gains[ voiceCount ];
            m_circBuff.setSample( input + m_xhns[ voiceCount ], voiceCount );
            input = m_delayedSamples[ voiceCount ] + m_xhns[ voiceCount ];
            // then go backwards
            for ( auto i = 0; i < m_stages[ j ] - 1; i++ )
            {
                voiceCount -= 1;
                // set value in circular buffer
                m_circBuff.setSample( input + m_xhns[ voiceCount ], voiceCount );
                // calculate input to previous stage
                input = m_delayedSamples[ voiceCount  ] + m_xhns[ voiceCount ];
            }
            voiceCount += m_stages[ j ];
        }
        m_circBuff.updateWritePosition();
        return input;
    }
    
    T processParallel( T input, bool delayTimeIsFractional = false )
    {
        size_t voiceCount = 0;
        T value = 0;
        input *= m_parallelInputScale;
        for ( auto j = 0; j < m_nStages; j++ )
        {
            T inSamp = input;
            for ( auto i = 0; i < m_stages[ j ] - 1; i++ )
            {
                // get delayed signal at this stage
                m_delayedSamples[ voiceCount ] = delayTimeIsFractional ? m_circBuff.getSample( m_delayTimesSamps[ voiceCount ], voiceCount ) : m_circBuff.getSample( m_roundedDelays[ voiceCount ], voiceCount );
                // to filter or not to filter
                m_delayedSamples[ voiceCount ] = m_shouldFilter ? m_lpfs[ voiceCount ].filterInput( m_delayedSamples[ voiceCount ] ) : m_delayedSamples[ voiceCount ];
                // one multiply value
                m_xhns[ voiceCount ] = ( inSamp - m_delayedSamples[ voiceCount ] ) * m_gains[ voiceCount ];
                inSamp += m_xhns[ voiceCount ];
                voiceCount += 1;
            }
            m_delayedSamples[ voiceCount ] = delayTimeIsFractional ? m_circBuff.getSample( m_delayTimesSamps[ voiceCount ], voiceCount ) : m_circBuff.getSample( m_roundedDelays[ voiceCount ], voiceCount );
            m_delayedSamples[ voiceCount ] = m_shouldFilter ? m_lpfs[ voiceCount ].filterInput( m_delayedSamples[ voiceCount ] ) : m_delayedSamples[ voiceCount ];
            m_xhns[ voiceCount ] = ( inSamp - m_delayedSamples[ voiceCount ] ) * m_gains[ voiceCount ];
            m_circBuff.setSample( inSamp + m_xhns[ voiceCount ], voiceCount );
            inSamp = m_delayedSamples[ voiceCount ] + m_xhns[ voiceCount ];
            // then go backwards
            for ( auto i = 0; i < m_stages[ j ] - 1; i++ )
            {
                voiceCount -= 1;
                m_circBuff.setSample( inSamp, voiceCount ); // set value in circular buffer
                inSamp = m_delayedSamples[ voiceCount  ] + m_xhns[ voiceCount ]; // calculate input to previous stage
            }
            value += inSamp;
            voiceCount += m_stages[ j ];
        }
        m_circBuff.updateWritePosition();
        return value;
    }
    
    void setDelayTimeSamps( T delayTimeSamps, int voiceNum )
    {
#ifndef NDEBUG
        assert( voiceNum >= 0 );
        assert( voiceNum < NVOICES );
#endif
        m_delayTimesSamps[ voiceNum ] = delayTimeSamps;
        m_roundedDelays[ voiceNum ] = delayTimeSamps;
    }
    
    void setNumNestedStages( int nStages )
    {
#ifndef NDEBUG
        assert( nStages > 0 );
        assert( nStages <= NVOICES );
#endif
        if ( m_nStages == nStages )
            return;
        m_nStages = nStages < 2 ? 2 : nStages > NVOICES ? NVOICES : nStages;
        m_stages.resize( m_nStages );
        int stageSize = NVOICES  / m_nStages;
        auto dif = NVOICES - ( stageSize * m_nStages );
        std::fill( std::begin( m_stages ), std::end( m_stages ) - dif,  stageSize );
        std::fill( std::end( m_stages ) - dif, std::end( m_stages ),  stageSize + 1 );
        m_parallelInputScale = 1.0 / std::pow( 2, ( log( m_nStages )/log( 2 ) ) / 2.0 );
    }
    
    void setInterpolationType( int interpType )
    {
        m_circBuff.setInterpolationType( interpType );
    }
    
    
    void setAllCoefficients( T gain, bool shouldAlternatePolarity )
    {
        m_gain = gain;
        
        auto voice = 0;
        if ( !shouldAlternatePolarity )
            std::fill( std::begin( m_gains ), std::end( m_gains ),  m_gain ); return;
        for ( auto s = 0; s < m_nStages; s++ )
        {
            auto g = m_gain;
            for ( auto i = 0; i < m_stages[ s ]; i++ )
            {
                m_gains[ voice ] = g;
                g *= -1.0;
                voice += 1;
            }
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
        for ( auto v = 0; v < NVOICES; v++ )
                m_lpfs[ v ].setCoefficient( lpfCoef * ( 1.0 - m_gains[ v ]) );
//        for ( auto & f : m_lpfs )
//            f.setCoefficient( LPFCoef );
    }
};

// diffusion a la geraint luff https://signalsmith-audio.co.uk/writing/2021/lets-write-a-reverb/
template < typename T, int NVOICES, int NSTAGES >
class sjf_glDiffuser
{
private:
    std::array< std::array< sjf_circularBuffer< T >, NVOICES >, NSTAGES > m_buffers;
    
public:
    sjf_glDiffuser(){}
    ~sjf_glDiffuser(){}
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
    std::array< size_t, NVOICES > m_roundedDelays;
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
            delayedSamps[ v ] = delayTimeIsFractional ? m_circBuff.getSample( m_roundedDelays[ v ], v ) : m_circBuff.getSample( m_delayTimesSamps[ v ], v );
            delayedSamps[ v ] = m_shouldFilter ? m_lpfs[ v ].filterInput( delayedSamps[ v ] ) : delayedSamps[ v ];
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
        m_roundedDelays[ voiceNum ] = static_cast< size_t >( delayTimeSamps );
    }
    
    T getDelayTimeSamps( int voiceNum )
    {
        return m_delayTimesSamps[ voiceNum ];
    }
    
    void setFeedbackGain( T fb, int voiceNum )
    {
        m_gains[ voiceNum ] = fb;
//        m_lpfs[ voiceNum ].setCoefficient( m_damp * ( 1.0 - m_gains[ voiceNum ]) );
    }
    
    void setInterpolationType( int interpType )
    {
        m_circBuff.setInterpolationType( interpType );
    }
    
    void setLPFCoefficient( T lpfCoef )
    {
        m_damp = lpfCoef;
        for ( auto v = 0; v < NVOICES; v++ )
//            m_lpfs[ v ].setCoefficient( m_damp * ( 1.0 - m_gains[ v ]) );
            m_lpfs[ v ].setCoefficient( m_damp );
//        for ( auto & filt : m_lpfs )
//            filt.setCoefficient( lpfCoef );
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
    static constexpr int N_LATE_REFLECT = 8;
    static constexpr int N_NESTED_AP = 8;
    static constexpr int PRIME_MAX = 5000;
    
    sjf_FDN< float, N_LATE_REFLECT > m_fdn;
    sjf_nestedAllpass< float, N_NESTED_AP > m_early;
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
        
        m_early.setAllCoefficients( 0.5, true );
        setERLPFCoef( 0.186 );
        m_early.setShouldFilter( true );
        
        m_fdn.setShouldFilter( true );
        setLRLPFCoef( 0.186 );
    }
    ~sjf_verb(){}
    
    void initialise( float sampleRate )
    {
        if ( sampleRate <= 0 )
            return;
        m_SR = sampleRate;
        
        m_early.initialise( m_SR * 0.25 );
        m_fdn.initialise( m_SR * 0.5 );
    }
    
    float process( float input )
    {
        auto er = m_erParallel ? m_early.processParallel( input, true ) : m_early.process( input, true );
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
        m_early.setAllLPFCoefficients( ERLPFCoef );
    }
    
    
    int getNumRoomTypes()
    {
        return m_roomRatios[ 0 ].size();
    }
    
    void setEarlyType( int nStages )
    {
        m_early.setNumNestedStages( std::max( nStages, 1 ) );
    }
    
    void setEarlyToLate( float earlyToLatePercentage )
    {
#ifndef NDEBUG
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
                auto f = sjf_calculateRoomMode<float>( m_roomRatios[0][room], m_roomRatios[1][room], 1, pqr[0], pqr[1], pqr[2] );
                mSorted[i] = modes[i] = f;
            }
            std::sort(mSorted.begin(), mSorted.end());

            for ( auto i = 0; i < m_pqrList.getSize(); i++ )
            {
                auto it = find(modes.begin(), modes.end(), mSorted[ i ] );
                m_pqrIndices[room][ i ] = static_cast<int>( it - modes.begin() );
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
        
        calculateNestedAPDelays( roomModesInSamps[ N_LATE_REFLECT - 1 ], primeList );
        
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
        auto baseAPDelay = pow( modePersiodSamples, 0.9 ) / static_cast<float>( N_NESTED_AP );
        //    calculate nested allpass delays
        for ( auto i = 0; i < N_NESTED_AP; i++ )
        {
            int desiredDelay = baseAPDelay * i;
            desiredDelay += baseAPDelay * ( N_NESTED_AP - i ) / N_NESTED_AP; // just so delays aren't evenly spread
            int delay = findNearestPrimeDelay( desiredDelay, primeList );
            m_early.setDelayTimeSamps( delay, i );
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
