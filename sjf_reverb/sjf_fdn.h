//
//  sjf_fdn.h
//
//  Created by Simon Fay on 27/03/2024.
//

#ifndef sjf_rev_fdn_h
#define sjf_rev_fdn_h

#include "../sjf_audioUtilitiesC++.h"
#include "../sjf_interpolators.h"
#include "../gcem/include/gcem.hpp"

#include "sjf_delay.h"
#include "sjf_damper.h"
#include "sjf_oneMultAP.h"

namespace sjf::rev
{
    /**
     A feedback delay network with low pass filtering and allpass based diffusion in the loop
     */
    
    // NCHANNELS should be a power of 2!!!
    template< typename T, int NCHANNELS >
    class fdn
    {
    public:
        fdn()
        {
            for ( auto & d : m_delays )
                d.initialise( sjf_nearestPowerAbove( m_SR, 2 ) );
            for ( auto & ap : m_diffusers )
                ap.initialise( sjf_nearestPowerAbove( m_SR * 0.25, 2 ) );
            
            // ensure delaytimes are initialised
            std::array< T, NCHANNELS > rDT;
            for ( auto & d : rDT )
                d = m_SR * ( 0.1 + ( rand01() * 0.4 ) );
            setDelayTimes( rDT );
            
            for ( auto & d : rDT )
                d = m_SR * ( 0.01 + ( rand01() * 0.14 ) );
            setAPTimes( rDT );
            
        }
        ~fdn(){}
        
        /**
         This must be called in prepare to play in order to set basic information such as maximum delay lengths and sample rate
         */
        void initialise( int maxSizePerChannelSamps, T sampleRate )
        {
            m_SR = sampleRate;
            for ( auto & d : m_delays )
                d.initialise( sjf_nearestPowerAbove( maxSizePerChannelSamps, 2 ) );
            for ( auto & ap : m_diffusers )
                ap.initialise( sjf_nearestPowerAbove( maxSizePerChannelSamps * 0.25, 2 ) );
        }
        
        /**
         This allows you to set all of the delay times
         */
        void setDelayTimes( const std::array< T, NCHANNELS >& dt )
        {
            for ( auto c = 0; c < NCHANNELS; c ++ )
                m_delayTimesSamps[ c ] = dt[ c ];
            setDecayInMS( m_decayInMS );
        }
        
        /**
         This allows you to set all of the delay times used by the allpass filters for diffusion
         */
        void setAPTimes( const std::array< T, NCHANNELS >& dt )
        {
            for ( auto c = 0; c < NCHANNELS; c ++ )
                m_apDelayTimesSamps[ c ] = dt[ c ];
            setDecayInMS( m_decayInMS );
        }
        
        
        /**
         This sets the amount of diffusion ( must be greater than -1 and less than 1
         0 sets no diffusion
         */
        void setDiffusion( T diff )
        {
            m_diffusion = diff;
        }
        
        
        /**
         This sets the amount of damping applied in the loop ( must be >= 0 and <= 1 )
         */
        void setDamping( T damp )
        {
            m_damping = damp;
        }
        
        /**
         This sets the desired decay time in milliseconds
         */
        void setDecayInMS( T decayInMS )
        {
            m_decayInMS = decayInMS;
            auto dt = 0.0;
            for ( auto c = 0; c < NCHANNELS; c++ )
            {
                dt = ( m_delayTimesSamps[ c ] +  m_apDelayTimesSamps[ c ] ) * m_SR * 0.001;
                m_fbGains[ c ] = sjf_calculateFeedbackGain< T >( dt, m_decayInMS );
            }
        }
        
        
        /**
         This should be called for every sample in the block
         The input is:
            array of samples, one for each channel in the delay network (up & downmixing must be done outside the loop
            the desired mixing type within the loop ( see @mixers )
            the interpolation type
                0 = None
                1 = linear
                2 = cubic
                3 = four point (pure data)
                4 = fourth order
                5 = godot
                6 = hermite
         */
        void processInPlace( std::array< T, NCHANNELS  >& samples, int mixType = mixers::hadamard, int interpType = 1 )
        {
            std::array< T, NCHANNELS  > delayed;
            for ( auto c = 0; c < NCHANNELS; c++ )
            {
                delayed[ c ] = m_delays[ c ].getSample( m_delayTimesSamps[ c ], interpType );
                delayed[ c ] = m_dampers[ c ].process( delayed[ c ], m_damping ); // lp filter
            }
            
            switch( mixType )
            {
                case mixers::none :
                    break;
                case mixers::hadamard :
                    Hadamard< T, NCHANNELS >::inPlace( delayed );
                case mixers::householder :
                    Householder< T, NCHANNELS >::mixInPlace( delayed );
            }
        
            
            if ( m_diffusion == 0.0 )
                for ( auto c = 0; c < NCHANNELS; c++ )
                    m_delays[ c ].setSample( samples[ c ] + delayed[ c ] );
            else
                for ( auto c = 0; c < NCHANNELS; c++ )
                    m_delays[ c ].setSample(
                                            m_diffusers[ c ].process(
                                                                     samples[ c ] + delayed[ c ], m_apDelayTimesSamps[ c ], m_diffusion
                                                                     )
                                            );
            samples = delayed;
            return;
        }
        
        /**
         The different possible mixers that can be used within the loop
         */
        enum mixers
        {
            none, hadamard, householder
        };
    private:
        std::array< delay< T >, NCHANNELS > m_delays;
        std::array< damper< T >, NCHANNELS > m_dampers;
        std::array< oneMultAP< T >, NCHANNELS > m_diffusers;
        std::array< T, NCHANNELS > m_delayTimesSamps, m_apDelayTimesSamps, m_fbGains;
        T m_decayInMS = 1000, m_SR = 44100, m_damping = 0.2, m_diffusion = 0.5;
    };
}

#endif /* sjf_rev_fdn_h */
