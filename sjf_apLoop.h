//
//  sjf_apLoop.h
//
//  Created by Simon Fay on 18/03/2024.
//



#ifndef sjf_apLoop_h
#define sjf_apLoop_h

#include "sjf_audioUtilitiesC++.h"
#include "gcem/include/gcem.hpp"
#
template < typename T, int NSTAGES, int AP_PERSTAGE >
class sjf_apLoop
{
public:
    sjf_apLoop()
    {
        fillPrimes();
        // just ensure that delaytimes are set to begin with
        for ( auto s = 0; s < NSTAGES; s++ )
            for ( auto d = 0; d < AP_PERSTAGE+1; d++ )
                setDelayTimeSamples(rand01() * 4410, s, d );
        setDecay( 500 );
    }
    ~sjf_apLoop(){}

    void initialise( T sampleRate )
    {
        m_SR = sampleRate;
        m_size = sjf_nearestPowerAbove( m_SR * 2, 2 );
        m_wrapMask = m_size - 1;
        m_buffer.resize( m_size );
    }
    
    void setDelayTimesSamples( std::array< std::array < T, AP_PERSTAGE + 1 >, NSTAGES > delayTimes)
    {
        for ( auto s = 0; s < NSTAGES; s++ )
            for ( auto d = 0; d < AP_PERSTAGE+1; d++ )
                m_delayTimes[ s ][ d ] = delayTimes[ d ][ s ];
        updateStageStarts( );
        setDecay( m_decayInMS );
    }
    
    void setDelayTimeSamples( T dt, int stage, int delayNumber )
    {
        m_delayTimes[ stage ][ delayNumber ] = dt;
        updateStageStarts();
        setDecay( m_decayInMS );
    }
    
    std::array< T, 2 > process( const T inputL, const T inputR )
    {
        // #define DoAllPass(N) { int j=(i+N)&MASK;float delayed=buf[j];buf[i]=x-=delayed*mu;x=x*mu+delayed; i=j; }
        // input signal into loop
        auto samp = m_buffer[ (m_writePos - m_totalDelayTimeSamps) & m_wrapMask ]; // start of loop
        auto outL = 0.0, outR = 0.0, delayed = 0.0, xhn = 0.0, writePos = 0.0, readPos = 0.0;
        for ( auto s = 0; s < NSTAGES; s ++ )
        {
            samp +=  ( (s & 1) == 0 ) ? inputL : inputR;
            writePos = m_writePos - m_stageStarts[ s ];
            // read pos  == writepos + offset for allpass
            for ( auto a = 0; a < AP_PERSTAGE; a++ )
            {
                // do all pass stuff
                // one multiply as per moorer
                readPos = writePos - m_delayTimes[ s ][ a ];
                readPos &= m_wrapMask;
                delayed = m_buffer[ readPos ];
                xhn = ( samp - delayed ) * m_diffusions[ s ][ a ];
                m_buffer[ writePos ] = samp + xhn;
                samp = delayed + xhn;
                writePos = readPos;
            }
            // do low pass filter
            // read last delay
            delayed = m_buffer[ ( readPos - 1 ) & m_wrapMask ];
            samp = samp + ( m_damping * ( delayed - samp ) );
            m_buffer[ writePos ] = samp;
            if ( (s&1) == 0 )
                outL += samp;
            else
                outR += samp;
            samp *= m_gain[ s ];
        }
//        m_lastSamp = samp;
        m_writePos += 1;
        m_writePos &= m_wrapMask;
        return { outL, outR };
    }
    
    void setDecay( T decayInMS )
    {
        m_decayInMS = decayInMS;
        for ( auto s = 0; s < NSTAGES; s++ )
        {
            auto del = 0.0;
            for ( auto d = 0; d < AP_PERSTAGE + 1; d++ )
                del += m_delayTimes[ s ][ d ];
            m_gain[ s ] = std::pow( -3.0 * m_decayInMS / del );
        }
    }
    
    void setDamping( T dampCoef )
    {
        m_damping = dampCoef;
//        for ( auto & d : m_damp )
//            d.setCoefficient( dampCoef );
    }
    
private:
    
    //=======//=======//=======//=======//=======//=======//=======//=======//=======
    //=======//=======//=======//=======//=======//=======//=======//=======//=======
    //=======//=======//=======//=======//=======//=======//=======//=======//=======
//    class sjf_damping
//    {
//    private:
//        T m_y1 = 0, m_g = 0;
//        static constexpr T m_maxG = 1.0 - std::numeric_limits< T >::epsilon();
//    public:
//        sjf_damping(){}
//        ~sjf_damping(){}
//
//        void setCoefficient( T g )
//        {
//            m_g = g<0 ? 0 : g >= 1 ? m_maxG : g;
//        }
//
//        T filterInput( T input )
//        {
//            m_y1 = input + m_g * ( m_y1 - input );
//            return m_y1;
//        }
//
//        void clear()
//        {
//            m_y1 = 0;
//        }
//
//    };
    //=======//=======//=======//=======//=======//=======//=======//=======//=======
    //=======//=======//=======//=======//=======//=======//=======//=======//=======
    //=======//=======//=======//=======//=======//=======//=======//=======//=======
    
    void updateStageStarts()
    {
        m_stageStarts[ 0 ] = 0;
        for ( auto s = 0; s < NSTAGES - 1; s++ )
        {
            auto del = 0.0;
            for ( auto d = 0; d < AP_PERSTAGE + 1; d++ )
            {
                del += m_delayTimes[ s ][ d ];
            }
            m_stageStarts[ s + 1 ] = del + m_stageStarts[ s ];
            m_totalDelayTimeSamps = m_stageStarts[ s + 1 ];
        }
        for ( auto d = 0; d < AP_PERSTAGE + 1; d++ )
        {
            m_totalDelayTimeSamps += m_delayTimes[ NSTAGES - 1 ][ d ];
        }
    }
    
    void fillPrimes()
    {
        static constexpr auto MAXNPRIMES = 10000;
        m_primes.reserve( MAXNPRIMES );
        for ( auto i = 2; i < MAXNPRIMES; i++ )
            if( sjf_isPrime( i ) )
                m_primes.emplace_back( i );
        m_primes.shrink_to_fit();
    }
    
    std::array< T, NSTAGES > m_stageStarts, m_gain;
    std::array< std::array< T, AP_PERSTAGE + 1 >, NSTAGES > m_delayTimes;
    std::array< std::array< T, AP_PERSTAGE >, NSTAGES > m_diffusions;
//    std::array< T, NSTAGES > m_dtPerStage;
//    std::array< sjf_damping, NSTAGES > m_damp;
    std::vector< int > m_primes;
    
//    T m_lastSamp = 0;
    std::vector< T > m_buffer;
    int m_writePos = 0;
    int m_size = 2048;
    int m_wrapMask = m_size - 1;
    T m_SR = 44100, m_decayInMS = 100;
    T m_damping = 0.2, m_totalDelayTimeSamps;
};


#endif /* sjf_ap_loop */

