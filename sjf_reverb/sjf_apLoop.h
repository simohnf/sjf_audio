//
//  sjf_apLoop.h
//
//  Created by Simon Fay on 27/03/2024.
//


#ifndef sjf_rev_apLoop_h
#define sjf_rev_apLoop_h

//#include "../sjf_audioUtilitiesC++.h"
//#include "../sjf_interpolators.h"
//#include "../gcem/include/gcem.hpp"
//#include "sjf_oneMultAP.h"
//#include "sjf_delay.h"
//#include "sjf_damper.h"
//#include "sjf_rev_consts.h"

#include "../sjf_rev.h"
namespace sjf::rev
{
    /**
     A basic allpass loop in the style of Keith Barrhttp://www.spinsemi.com/knowledge_base/effects.html#Reverberation
     The architecture can have any number of stages and any number of allpass loops per stage
     Each stage is separated by a further delay and then a lowpass filter
     This version does not use a single loop
     */
    template < typename T, int NSTAGES, int AP_PERSTAGE >
    class allpassLoop
    {
    private:
        std::array< std::array< oneMultAP < T >, AP_PERSTAGE >, NSTAGES > m_aps;
        std::array< delay < T >, NSTAGES > m_delays;
        std::array< damper < T >, NSTAGES > m_dampers;
        
        std::array< T, NSTAGES > m_gains;
        std::array< std::array< T, AP_PERSTAGE + 1 >, NSTAGES > m_delayTimes;
        std::array< std::array< T, AP_PERSTAGE >, NSTAGES > m_diffusions;
        
        T m_lastSamp = 0;
        T m_SR = 44100, m_decayInMS = 100;
        T m_damping = 0.999;
    public:
        allpassLoop()
        {
            // just ensure that delaytimes are set to begin with
            for ( auto s = 0; s < NSTAGES; s++ )
                for ( auto d = 0; d < AP_PERSTAGE+1; d++ )
                    setDelayTimeSamples( std::round(rand01() * 4410 + 4410), s, d );
            
            setDecay( 5000 );
            setDiffusion( 0.5 );
        }
        ~allpassLoop(){}
        
        /**
         This must be called before first use in order to set basic information such as maximum delay lengths and sample rate
         */
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
        
        /**
         This allows you to set all of the delay times
         */
        void setDelayTimesSamples( std::array< std::array < T, AP_PERSTAGE + 1 >, NSTAGES > delayTimes)
        {
            for ( auto s = 0; s < NSTAGES; s++ )
                for ( auto d = 0; d < AP_PERSTAGE+1; d++ )
                    m_delayTimes[ s ][ d ] = delayTimes[ d ][ s ];
            setDecay( m_decayInMS );
        }
        
        /**
         This sets the amount of diffusion
         i.e. the allpass coefficient ( keep -1 < diff < 1 for safety )
         */
        void setDiffusion( T diff )
        {
            diff = diff <= 0.9 ? (diff >= -0.9 ? diff : -0.9 ) : 0.9;
            for ( auto s = 0; s < NSTAGES; s++ )
                for ( auto d = 0; d < AP_PERSTAGE; d++ )
                    m_diffusions[ s ][ d ] = diff;
        }
        
        /**
         This allows you to set a single the delay time
         */
        void setDelayTimeSamples( T dt, int stage, int delayNumber )
        {
            m_delayTimes[ stage ][ delayNumber ] = dt;
            setDecay( m_decayInMS );
        }
        
        /**
         This sets the desired decay time in milliseconds
         */
        void setDecay( T decayInMS )
        {
            m_decayInMS = decayInMS;
            for ( auto s = 0; s < NSTAGES; s++ )
            {
                auto del = 0.0;
                for ( auto d = 0; d < AP_PERSTAGE + 1; d++ )
                    del += m_delayTimes[ s ][ d ];
                del  /= ( m_SR * 0.001 );
                m_gains[ s ] = sjf_calculateFeedbackGain<T>( del, m_decayInMS );
//                m_gains[ s ] = std::pow( 10.0, -3.0 * del / m_decayInMS );
            }
        }
        
        /**
         This sets the amount of damping applied between each section of the loop ( must be >= 0 and <= 1 )
         */
        void setDamping( T dampCoef )
        {
            m_damping = dampCoef < 1 ? (dampCoef > 0 ? dampCoef : 00001) : 0.9999;
        }
        
        /**
         This should be called for every sample in the block
         The input is:
         the sample to input into the loop
         the interpolation type (optional, defaults to linear, see @sjf_interpolators)
         */
        T process( T input, int interpType = DEFAULT_INTERP )
        {
            auto samp = m_lastSamp;
            auto output = 0.0;
            for ( auto s = 0; s < NSTAGES; s ++ )
            {
                samp +=  input;
                for ( auto a = 0; a < AP_PERSTAGE; a++ )
                {
                    samp = m_aps[ s ][ a ].process( samp, m_delayTimes[ s ][ a ], m_diffusions[ s ][ a ], interpType );
                }
                samp = m_dampers[ s ].process( samp, m_damping );
                output += samp;
                m_delays[ s ].setSample( samp * m_gains[ s ] );
                samp = m_delays[ s ].getSample( m_delayTimes[ s ][ AP_PERSTAGE ] );
            }
            m_lastSamp = samp;
            return output;
        }
        
    };
}


#endif /* sjf_rev_apLoop_h */
