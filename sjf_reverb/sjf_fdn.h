//
//  sjf_fdn.h
//
//  Created by Simon Fay on 27/03/2024.
//

#ifndef sjf_rev_fdn_h
#define sjf_rev_fdn_h

#include "../sjf_rev.h"

namespace sjf::rev
{
    /**
     A feedback delay network with low pass filtering and allpass based diffusion in the loop
     */
template< typename Sample, mixers::mixerTypes mixType = mixers::mixerTypes::householder, rev::fbLimiters::fbLimiterTypes limitType  = rev::fbLimiters::fbLimiterTypes::none, interpolation::interpolatorTypes interpType = interpolation::interpolatorTypes::pureData >
    class fdn
    {
    public:
        fdn( const size_t nChannels = 8 ) noexcept : NCHANNELS(nChannels), m_delays(NCHANNELS), m_mixer(NCHANNELS)
        {
            m_dampers.resize( NCHANNELS );
            m_lowDampers.resize( NCHANNELS );
            m_diffusers.resize( NCHANNELS );
            m_delayTimesSamps.resize( NCHANNELS, 0 );
            m_apDelayTimesSamps.resize( NCHANNELS, 0 );
            m_fbGains.resize( NCHANNELS, 0 );
            
            m_delays.initialise(m_SR);
            for ( auto & ap : m_diffusers )
                ap.initialise( m_SR * 0.25 );
            setDecay( m_decayInMS );
        }
        ~fdn(){}
        
        /**
         This must be called before first use in order to set basic information such as maximum delay lengths and sample rate
         */
        void initialise( const long maxSizePerChannelSamps, const long maxSizePerAPChannelSamps, const Sample sampleRate )
        {
            m_SR = sampleRate > 0 ? sampleRate : m_SR;
            m_delays.initialise( maxSizePerChannelSamps );
            for ( auto & ap : m_diffusers )
                ap.initialise( maxSizePerAPChannelSamps );
            
            setDecay( m_decayInMS );
        }
        
        

        
        
        /**
         This allows you to set all of the delay times
         */
        void setDelayTimes( const vect< Sample >& dt )
        {
            assert( dt.size() == NCHANNELS );
            for ( auto c = 0; c < NCHANNELS; ++c )
                m_delayTimesSamps[ c ] = dt[ c ];
            setDecay( m_decayInMS );
        }
        
        /**
         This allows you to set one of the delay times, this does not reset the decay time calculation so you will need to reset that using setDecay()!!!
         */
        void setDelayTime( const Sample dt, const size_t delayNumber )
        {
            assert( delayNumber < NCHANNELS );
            m_delayTimesSamps[ delayNumber ] = dt;
        }
        
        /**
         This allows you to set all of the delay times used by the allpass filters for diffusion.
         */
        void setAPTimes( const vect< Sample >& dt )
        {
            assert( dt.size() == NCHANNELS );
            for ( auto c = 0; c < NCHANNELS; ++c )
                m_apDelayTimesSamps[ c ] = dt[ c ];
            setDecay( m_decayInMS );
        }
        
        /**
         This allows you to set one of the delay times used by the allpass filters for diffusion,  this does not reset the decay time calculation so you will need to reset that using setDecay()!!! 
         */
        void setAPTime( const Sample dt, const size_t delayNumber )
        {
            assert( delayNumber < NCHANNELS );
            m_apDelayTimesSamps[ delayNumber ] = dt;
        }
        
        
        /**
         This sets the amount of diffusion ( must be greater than -1 and less than 1
         0 sets no diffusion
         */
        void setDiffusion( const Sample diff )
        {
            assert ( diff > -0.9 && diff < 0.9 );
            m_diffusion = diff;
        }
        
        
        /**
         This sets the amount of high frequency damping applied in the loop ( must be >= 0 and <= 1 )
         */
        void setDamping( const Sample dampCoef )
        {
            assert ( dampCoef > 0 && dampCoef < 1 );
            m_damping = dampCoef;
        }
        /**
         This sets the amount of low frequency damping applied in the loop ( must be >= 0 and <= 1 )
         */
        void setDampingLow( const Sample dampCoef )
        {
            assert ( dampCoef > 0 && dampCoef < 1 );
            m_lowDamping = dampCoef;
        }
        /**
         This sets the desired decay time in milliseconds
         */
        void setDecay( const Sample decayInMS )
        {
            if ( m_decayInMS == decayInMS ){ return; }
            m_decayInMS = decayInMS;
            auto dt = 0.0;
            for ( auto c = 0; c < NCHANNELS; ++c )
            {
                dt = ( m_delayTimesSamps[ c ] +  m_apDelayTimesSamps[ c ] ) * 1000.0 / m_SR;
                m_fbGains[ c ] = sjf_calculateFeedbackGain< Sample >( dt, m_decayInMS );
            }
        }
        
        
        /**
         This should be called for every sample in the block
         The input is:
            array of samples, one for each channel in the delay network (up & downmixing must be done outside the loop
         */
        void processInPlace( vect< Sample >& samples )
        {
            assert( samples.size() == NCHANNELS );
            vect< Sample > delayed( NCHANNELS, 0 );
            for ( auto c = 0; c < NCHANNELS; ++c )
            {
                delayed[ c ] = m_delays.getSample( c, m_delayTimesSamps[ c ] );
                delayed[ c ] = m_dampers[ c ].process( delayed[ c ], m_damping ); // lp filter
                delayed[ c ] = m_lowDampers[ c ].processHP( delayed[ c ], m_lowDamping ); // hp filter
            }
            m_mixer.inPlace(delayed.data(), NCHANNELS);
            
            
            for ( auto c = 0; c < NCHANNELS; ++c )
                m_delays.setSample( c, m_limiter( m_diffusers[c].process( samples[c]+delayed[c]*m_fbGains[c], m_apDelayTimesSamps[c],m_diffusion ) ) );
            m_delays.updateWritePos();
            samples = delayed;
            return;
        }
        
        
        
    private:
        const size_t NCHANNELS;
        
        delayLine::multiChannelDelay<Sample, interpType > m_delays;
        vect< filters::oneMultAP< Sample, interpType > > m_diffusers;
        vect< filters::damper< Sample > > m_dampers, m_lowDampers;
        vect< Sample > m_delayTimesSamps, m_apDelayTimesSamps, m_fbGains;
        Sample m_decayInMS{1000}, m_SR{44100}, m_damping{0.2}, m_lowDamping{0.95}, m_diffusion{0.5};
        
//        MIXER m_mixer;
        mixers::mixer< Sample, mixType > m_mixer;
//        LIMITER m_limiter;
        fbLimiters::limiter< Sample, limitType > m_limiter;
    };
}

#endif /* sjf_rev_fdn_h */
