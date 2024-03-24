//
//  sjf_apLoop.h
//
//  Created by Simon Fay on 18/03/2024.
//



#ifndef sjf_rev_h
#define sjf_rev_h

#include "sjf_audioUtilitiesC++.h"
#include "sjf_interpolators.h"
#include "gcem/include/gcem.hpp"
namespace sjf_rev
{


    //=======//=======//=======//=======//=======//=======//=======//=======//=======
    //=======//=======//=======//=======//=======//=======//=======//=======//=======
    //=======//=======//=======//=======//=======//=======//=======//=======//=======


    template < typename  T >
    class delay
    {
    private:
        std::vector< T > m_buffer;
        int m_writePos = 0, m_wrapMask;
    public:
        delay(){}
        ~delay(){}
        
        void initialise( int sizeInSamps_pow2 )
        {
            m_buffer.resize( sizeInSamps_pow2, 0 );
            m_wrapMask = sizeInSamps_pow2 - 1;
        }
        
        T getSample( T delay, int interpType = 1 )
        {
            auto rp = getPosition( ( m_writePos - delay ) );
            switch( interpType )
            {
                case 0:
                    return m_buffer[ rp ];
                case sjf_interpolators::interpolatorTypes::linear:
                    return linInterp( rp );
                case sjf_interpolators::interpolatorTypes::cubic:
                    return polyInterp( rp, sjf_interpolators::interpolatorTypes::cubic );
                case sjf_interpolators::interpolatorTypes::pureData:
                    return polyInterp( rp, sjf_interpolators::interpolatorTypes::pureData );
                case sjf_interpolators::interpolatorTypes::fourthOrder:
                    return polyInterp( rp, sjf_interpolators::interpolatorTypes::fourthOrder );
                case sjf_interpolators::interpolatorTypes::godot:
                    return polyInterp( rp, sjf_interpolators::interpolatorTypes::godot );
                case sjf_interpolators::interpolatorTypes::hermite:
                    return polyInterp( rp, sjf_interpolators::interpolatorTypes::hermite );
            }
            return m_buffer[ rp ];
        }
        
        void setSample( T x )
        {
            m_buffer[ m_writePos ] = x;
            m_writePos += 1;
            m_writePos &= m_wrapMask;
        }
        
    private:
        inline T linInterp( T findex )
        {
            T x1, x2, mu;
            auto ind1 = static_cast< long >( findex );
            mu = findex - ind1;
            x1 = m_buffer[ ind1 ];
            x2 = m_buffer[ ( (ind1+1) & m_wrapMask ) ];
            
            return sjf_interpolators::linearInterpolate( mu, x1, x2 );
        }
        
        inline T polyInterp( T findex, int interpType )
        {
            T x0, x1, x2, x3, mu;
            auto ind1 = static_cast< long >( findex );
            mu = findex - ind1;
            x0 = m_buffer[ ( (ind1-1) & m_wrapMask ) ];
            x1 = m_buffer[ ind1 ];
            x2 = m_buffer[ ( (ind1+1) & m_wrapMask ) ];
            x3 = m_buffer[ ( (ind1+2) & m_wrapMask ) ];
            switch ( interpType ) {
                case sjf_interpolators::interpolatorTypes::cubic :
                    return sjf_interpolators::cubicInterpolate( mu, x0, x1, x2, x3 );
                    break;
                case sjf_interpolators::interpolatorTypes::pureData :
                    return sjf_interpolators::fourPointInterpolatePD( mu, x0, x1, x2, x3 );
                    break;
                case sjf_interpolators::interpolatorTypes::fourthOrder :
                    return sjf_interpolators::fourPointFourthOrderOptimal( mu, x0, x1, x2, x3 );
                    break;
                case sjf_interpolators::interpolatorTypes::godot :
                    return sjf_interpolators::cubicInterpolateGodot( mu, x0, x1, x2, x3 );
                    break;
                case sjf_interpolators::interpolatorTypes::hermite :
                    return sjf_interpolators::cubicInterpolateHermite( mu, x0, x1, x2, x3 );
                    break;
                default:
                    return sjf_interpolators::linearInterpolate( mu, x1, x2 );
                    break;
            }
        }
        
        T getPosition( T pos )
        {
            int p = pos;
            T mu = pos - p;
            p &= m_wrapMask;
            return ( static_cast< T >( p ) + mu );
        }
    };

    //=======//=======//=======//=======//=======//=======//=======//=======//=======
    //=======//=======//=======//=======//=======//=======//=======//=======//=======
    //=======//=======//=======//=======//=======//=======//=======//=======//=======
    template < typename  T >
    class oneMultAP
    {
    private:
        delay< T > m_del;
    public:
        oneMultAP(){}
        ~oneMultAP(){}
        
        void initialise( int sizeInSamps_pow2 )
        {
            m_del.initialise( sizeInSamps_pow2 );
        }
        
        T process( T x, T delay, T coef )
        {
            auto delayed = m_del.getSample( delay );
            auto xhn = ( x - delayed ) * coef;
            m_del.setSample( x + xhn );
            return delayed + xhn;
        }
    };

    //=======//=======//=======//=======//=======//=======//=======//=======//=======
    //=======//=======//=======//=======//=======//=======//=======//=======//=======
    //=======//=======//=======//=======//=======//=======//=======//=======//=======
    template < typename  T >
    class damper
    {
    private:
        T m_lastOut = 0;
    public:
        damper(){}
        ~damper(){}
        
        T process( T x, T coef )
        {
            m_lastOut = x + coef*( m_lastOut - x );
            return m_lastOut;
        }
    };
//=======//=======//=======//=======//=======//=======//=======//=======//=======
//=======//=======//=======//=======//=======//=======//=======//=======//=======
//=======//=======//=======//=======//=======//=======//=======//=======//=======

    template < typename T, int MAXNTAPS >
    class multiTap
    {
    public:
        multiTap(){}
        ~multiTap(){}
        
        void initialise( T sampleRate )
        {
            auto delSize = sjf_nearestPowerAbove( sampleRate / 2, 2 );
            m_delay.initialise( delSize );
        }
        
        void setDelayTimes( const std::array< T, MAXNTAPS >& dt )
        {
            for ( auto d = 0; d < MAXNTAPS; d++ )
                m_delayTimes[ d ] = dt[ d ];
        }
        
        void setGains( const std::array< T, MAXNTAPS >& gains )
        {
            for ( auto  g = 0; g < MAXNTAPS; g++ )
                m_gains[ g ] = gains[ g ];
        }
        
        void setNTaps( int nTaps )
        {
            m_nTaps = nTaps;
        }
        
        T process( T x )
        {
            T output = 0.0;
            for ( auto t = 0; t < m_nTaps; t++ )
                output += m_delay.getSample( m_delayTimes[ t ], 0 ) * m_gains[ t ];
            m_delay.setSample( x );
            return output;
        }
    private:
        std::array< T, MAXNTAPS > m_delayTimes, m_gains;
        int m_nTaps = MAXNTAPS;
        delay< T > m_delay;
    };
    //=======//=======//=======//=======//=======//=======//=======//=======//=======
    //=======//=======//=======//=======//=======//=======//=======//=======//=======
    //=======//=======//=======//=======//=======//=======//=======//=======//=======

    template < typename T, int NSTAGES >
    class seriesAllpass
    {
    public:
        seriesAllpass(){}
        ~seriesAllpass(){}
        
        void initialise( T sampleRate )
        {
            auto size = sjf_nearestPowerAbove( sampleRate * 0.1, 2 );
            for ( auto & a : m_aps )
                a.initialise( size );
        }
        
        void setCoefs( T coef )
        {
            for ( auto a = 0; a < NSTAGES; a++ )
                m_coefs[ a ] = coef;
        }
        
        void setDelayTimes( const std::array< T, NSTAGES >& dt )
        {
            for ( auto d = 0; d < NSTAGES; d++ )
                m_delayTimes[ d ] = dt[ d ];
        }
        
        T process( T x )
        {
            for ( auto a = 0; a < NSTAGES; a++ )
            {
                x = m_aps[ a ].process( x, m_delayTimes[ a ], m_coefs[ a ] );
            }
            return x;
        }
    private:
        std::array< oneMultAP< T >, NSTAGES > m_aps;
        std::array< T, NSTAGES > m_coefs, m_delayTimes;
    };

    //=======//=======//=======//=======//=======//=======//=======//=======//=======
    //=======//=======//=======//=======//=======//=======//=======//=======//=======
    //=======//=======//=======//=======//=======//=======//=======//=======//=======

    template < typename T, int NSTAGES, int AP_PERSTAGE >
    class allpassLoop
    {
    public:
        allpassLoop()
        {
//            fillPrimes();
            // just ensure that delaytimes are set to begin with
            for ( auto s = 0; s < NSTAGES; s++ )
                for ( auto d = 0; d < AP_PERSTAGE+1; d++ )
                    setDelayTimeSamples( std::round(rand01() * 4410 + 4410), s, d );
            setDecay( 5000 );
            setDiffusion( 0.5 );
        }
        ~allpassLoop(){}

        void initialise( T sampleRate )
        {
            m_SR = sampleRate;
            auto delSize = sjf_nearestPowerAbove( m_SR / 2, 2 );
            for ( auto s = 0; s < NSTAGES; s++ )
            {
                for ( auto a = 0; a < AP_PERSTAGE; a++ )
                    m_aps[ s ][ a ].initialise( delSize );
                m_delays[ s ].initialise( delSize );
            }
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
            diff = diff <= 0.9 ? (diff >= -0.9 ? diff : -0.9 ) : 0.9;
            for ( auto s = 0; s < NSTAGES; s++ )
                for ( auto d = 0; d < AP_PERSTAGE; d++ )
                    m_diffusions[ s ][ d ] = diff;
        }
        
        void setDelayTimeSamples( T dt, int stage, int delayNumber )
        {
            m_delayTimes[ stage ][ delayNumber ] = dt;
            setDecay( m_decayInMS );
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
                m_gain[ s ] = std::pow( 10.0, -3.0 * del / m_decayInMS );
            }
        }
        
        void setDamping( T dampCoef )
        {
            m_damping = dampCoef < 1 ? (dampCoef > 0 ? dampCoef : 00001) : 0.9999;
        }
        
        
        T process( T input )
        {
            auto samp = m_lastSamp;
            auto output = 0.0;
            for ( auto s = 0; s < NSTAGES; s ++ )
            {
                samp +=  input;
                for ( auto a = 0; a < AP_PERSTAGE; a++ )
                {
                    samp = m_aps[ s ][ a ].process( samp, m_delayTimes[ s ][ a ], m_diffusions[ s ][ a ] );
                }
                samp = m_dampers[ s ].process( samp, m_damping );
                output += samp;
                m_delays[ s ].setSample( samp * m_gain[ s ] );
                samp = m_delays[ s ].getSample( m_delayTimes[ s ][ AP_PERSTAGE ] );
            }
            m_lastSamp = samp;
            return output;
        }
        
    private:
//
//        void fillPrimes()
//        {
//            static constexpr auto MAXNPRIMES = 10000;
//            m_primes.reserve( MAXNPRIMES );
//            for ( auto i = 2; i < MAXNPRIMES; i++ )
//                if( sjf_isPrime( i ) )
//                    m_primes.emplace_back( i );
//            m_primes.shrink_to_fit();
//        }
        
        std::array< T, NSTAGES > m_gain;
        std::array< std::array< T, AP_PERSTAGE + 1 >, NSTAGES > m_delayTimes;
        std::array< std::array< T, AP_PERSTAGE >, NSTAGES > m_diffusions;
        
        
        std::array< std::array< oneMultAP < T >, AP_PERSTAGE >, NSTAGES > m_aps;
        std::array< delay < T >, NSTAGES > m_delays;
        std::array< damper < T >, NSTAGES > m_dampers;
        
        std::vector< int > m_primes;
        
        T m_lastSamp = 0;
        T m_SR = 44100, m_decayInMS = 100;
        T m_damping = 0.999;
    };


    template< typename T, int NSTAGES >
    class seriesAP
    {
    public:
        seriesAP()
        {
            
        }
        ~seriesAP(){}
        
        void initialise( T sampleRate )
        {
            m_SR = sampleRate;
            auto size = sjf_nearestPowerAbove( sampleRate, 2 );
            m_buffer.resize( size );
            m_wrapMask = size - 1;
        }
        
    private:
        std::vector< T > m_buffer;
        int m_wrapMask = 0;
        T coef = 0.7, m_SR = 44100;
        std::array< T, NSTAGES > m_delayTimes;
    };
}

#endif /* sjf_rev_h */



