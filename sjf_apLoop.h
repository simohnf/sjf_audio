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
                setDelayTimeSamples(rand01() * 4410 + 4410, s, d );
        std::fill( m_lpfs.begin(), m_lpfs.end(), 0 );
        setDecay( 5000 );
        setDiffusion( 0.6 );
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
        setDecay( m_decayInMS );
    }
    
    void setDiffusion( T diff )
    {
        for ( auto s = 0; s < NSTAGES; s++ )
            for ( auto d = 0; d < AP_PERSTAGE; d++ )
                m_diffusions[ s ][ d ] = diff;
    }
    
    void setDelayTimeSamples( T dt, int stage, int delayNumber )
    {
        m_delayTimes[ stage ][ delayNumber ] = dt;
        setDecay( m_decayInMS );
    }
    

    T process( T input )
    {
        // #define DoAllPass(N) { int j=(i+N)&MASK;float delayed=buf[j];buf[i]=x-=delayed*mu;x=x*mu+delayed; i=j; }
        // input signal into loop
//        auto samp = m_buffer[ (m_writePos - m_totalDelayTimeSamps) & m_wrapMask ]; // start of loop
        auto samp = m_lastSamp;
        auto output = 0.0, readPos = 0.0;
        T writePos = m_writePos;
        for ( auto s = 0; s < NSTAGES; s ++ )
        {
            samp +=  input;
            for ( auto a = 0; a < AP_PERSTAGE; a++ )
            {
                // do all pass stuff
                readPos = getPosition( writePos - m_delayTimes[ s ][ a ] );
                samp = doAllpass( samp, m_diffusions[ s ][ a ], readPos, writePos );
                writePos = readPos;
            }
            
            // read last delay
            writePos = getPosition( writePos - m_delayTimes[ s ][ AP_PERSTAGE ] );
            // apply lpf
            m_lpfs[ s ] = doDamping( m_buffer[ writePos ], m_lpfs[ s ], m_damping );
            samp = m_lpfs[ s ];
//            samp = m_dampers[ s ].process( m_buffer[ writePos ], m_damping );
            output += samp; // add to rev output
            samp *= m_gain[ s ]; // decay factor
            m_buffer[ writePos ] = samp;
        }
        m_lastSamp = samp;
        m_writePos += 1;
        m_writePos &= m_wrapMask;
        return output;
    }
    




    void setDecay( T decayInMS )
    {
        m_decayInMS = decayInMS;
        for ( auto s = 0; s < NSTAGES; s++ )
        {
            auto del = 0.0;
            for ( auto d = 0; d < AP_PERSTAGE + 1; d++ )
                del += m_delayTimes[ s ][ d ];
            del  /= ( m_SR * 0.001 );
//            auto res = -3.0 * m_decayInMS / del ;
            m_gain[ s ] = std::pow( 10.0, -3.0 * del / m_decayInMS );
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
    
    void fillPrimes()
    {
        static constexpr auto MAXNPRIMES = 10000;
        m_primes.reserve( MAXNPRIMES );
        for ( auto i = 2; i < MAXNPRIMES; i++ )
            if( sjf_isPrime( i ) )
                m_primes.emplace_back( i );
        m_primes.shrink_to_fit();
    }
    
    
    inline T getPosition( T pos )
    {
        int p = pos;
        T mu = pos - p;
        p &= m_wrapMask;
        return ( static_cast< T >( p ) + mu );
    }
    
    
    inline T doAllpass( T input, T coef, T readPos, T writePos )
    {
        // #define DoAllPass(N) { int j=(i+N)&MASK;float delayed=buf[j];buf[i]=x-=delayed*mu;x=x*mu+delayed; i=j; }
        // one multiply as per moorer
        auto delayed = m_buffer[ readPos ];
//        input -= delayed * coef;
//        m_buffer[ writePos ] = input;
//        return input*coef + delayed;
        auto xhn = ( input - delayed ) * coef;
        m_buffer[ writePos ] = input + xhn;
        return delayed + xhn;
    }
    
    inline T doDamping( T x, T lastOut, T coef )
    {
        x = x + coef*( lastOut - x );
        return x;
    }
    
    std::array< T, NSTAGES > m_gain, m_lpfs;
    std::array< std::array< T, AP_PERSTAGE + 1 >, NSTAGES > m_delayTimes;
    std::array< std::array< T, AP_PERSTAGE >, NSTAGES > m_diffusions;
//    std::array< T, NSTAGES > m_dtPerStage;
//    std::array< sjf_damping, NSTAGES > m_damp;
    std::vector< int > m_primes;
    
    T m_lastSamp = 0;
    std::vector< T > m_buffer;
    int m_writePos = 0;
    int m_size = 2048;
    int m_wrapMask = m_size - 1;
    T m_SR = 44100, m_decayInMS = 100;
    T m_damping = 0.999, m_totalDelayTimeSamps;
};


#endif /* sjf_ap_loop */

