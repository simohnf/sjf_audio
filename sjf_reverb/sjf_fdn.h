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
     The different possible mixers that can be used within the loop
     */
    enum class mixers
    {
        none, hadamard, householder
    };
    /**
     A feedback delay network with low pass filtering and allpass based diffusion in the loop
     */
    
    // NCHANNELS should be a power of 2!!!
    template< typename T >
    class fdn /* : sjf::multiChannelEffect< T > */
    {
    public:
        fdn( int nchannels ) noexcept : NCHANNELS( nchannels )
        {
            m_delays.resize( NCHANNELS );
            m_dampers.resize( NCHANNELS );
            m_lowDampers.resize( NCHANNELS );
            m_diffusers.resize( NCHANNELS );
            m_delayTimesSamps.resize( NCHANNELS, 0 );
            m_apDelayTimesSamps.resize( NCHANNELS, 0 );
            m_fbGains.resize( NCHANNELS, 0 );
            
            
            for ( auto & d : m_delays )
                d.initialise( m_SR );
            for ( auto & ap : m_diffusers )
                ap.initialise( m_SR * 0.25 );
            setDecayInMS( m_decayInMS );
        }
        ~fdn(){}
        
        /**
         This must be called before first use in order to set basic information such as maximum delay lengths and sample rate
         */
        void initialise( int maxSizePerChannelSamps, int maxSizePerAPChannelSamps, T sampleRate )
        {
            m_SR = sampleRate;
            for ( auto & d : m_delays )
                d.initialise( maxSizePerChannelSamps );
            for ( auto & ap : m_diffusers )
                ap.initialise( maxSizePerAPChannelSamps );
            
            setDecayInMS( m_decayInMS );
        }
        
        

        
        
        /**
         This allows you to set all of the delay times
         */
        void setDelayTimes( const std::vector< T >& dt )
        {
            assert( dt.size() == NCHANNELS );
            for ( auto c = 0; c < NCHANNELS; c ++ )
                m_delayTimesSamps[ c ] = dt[ c ];
            setDecayInMS( m_decayInMS );
        }
        
        /**
         This allows you to set one of the delay times, this does not reset the decay time calculation so you will need to reset that using setDecayInMS()!!!
         */
        void setDelayTime( const T dt, const int delayNumber )
        {
            assert( delayNumber < NCHANNELS );
            m_delayTimesSamps[ delayNumber ] = dt;
        }
        
        /**
         This allows you to set all of the delay times used by the allpass filters for diffusion.
         */
        void setAPTimes( const std::vector< T >& dt )
        {
            assert( dt.size() == NCHANNELS );
            for ( auto c = 0; c < NCHANNELS; c ++ )
                m_apDelayTimesSamps[ c ] = dt[ c ];
            setDecayInMS( m_decayInMS );
        }
        
        /**
         This allows you to set one of the delay times used by the allpass filters for diffusion,  this does not reset the decay time calculation so you will need to reset that using setDecayInMS()!!! 
         */
        void setAPTime( const T dt, const int delayNumber )
        {
            assert( delayNumber < NCHANNELS );
            m_apDelayTimesSamps[ delayNumber ] = dt;
        }
        
        
        /**
         This sets the amount of diffusion ( must be greater than -1 and less than 1
         0 sets no diffusion
         */
        void setDiffusion( T diff ) { m_diffusion = diff; }
        
        
        /**
         This sets the amount of high frequency damping applied in the loop ( must be >= 0 and <= 1 )
         */
        void setDamping( T dampCoef ) { m_damping = dampCoef < 1 ? (dampCoef > 0 ? dampCoef : 00001) : 0.9999; }
        
        /**
         This sets the amount of low frequency damping applied in the loop ( must be >= 0 and <= 1 )
         */
        void setDampingLow( T dampCoef ) { m_lowDamping = dampCoef < 1 ? (dampCoef > 0 ? dampCoef : 00001) : 0.9999; }
        
        /**
         This sets the desired decay time in milliseconds
         */
        void setDecayInMS( T decayInMS )
        {
            if ( m_decayInMS == decayInMS ){ return; }
            m_decayInMS = decayInMS;
            auto dt = 0.0;
            for ( auto c = 0; c < NCHANNELS; c++ )
            {
                dt = ( m_delayTimesSamps[ c ] +  m_apDelayTimesSamps[ c ] ) * 1000.0 / m_SR;
                m_fbGains[ c ] = sjf_calculateFeedbackGain< T >( dt, m_decayInMS );
            }
        }
        
        
        /**
         This should be called for every sample in the block
         The input is:
            array of samples, one for each channel in the delay network (up & downmixing must be done outside the loop
         */
        void processInPlace( std::vector< T >& samples )
        {
            assert( samples.size() == NCHANNELS );
            std::vector< T > delayed( NCHANNELS, 0 );
            for ( auto c = 0; c < NCHANNELS; c++ )
            {
                delayed[ c ] = m_delays[ c ].getSample( m_delayTimesSamps[ c ] );
                delayed[ c ] = m_dampers[ c ].process( delayed[ c ], m_damping ); // lp filter
                delayed[ c ] = m_lowDampers[ c ].processHP( delayed[ c ], m_lowDamping ); // hp filter
            }
            
            mixer( delayed );
        
            
            if ( m_diffusion == 0.0 )
                for ( auto c = 0; c < NCHANNELS; c++ )
                    m_delays[ c ].setSample(
                                            m_fbControl?
                                            nonlinearities::tanhSimple( samples[ c ] + delayed[ c ]*m_fbGains[ c ] ) :
                                            samples[ c ] + delayed[ c ]*m_fbGains[ c ]
                                            );
            else
                for ( auto c = 0; c < NCHANNELS; c++ )
                    m_delays[ c ].setSample( m_diffusers[ c ].process( samples[c]+delayed[c]*m_fbGains[c], m_apDelayTimesSamps[c], m_diffusion ) );
            samples = delayed;
            return;
        }
        
        /** Set the interpolation Type to be used, the interpolation type see @sjf_interpolators */
        void setInterpolationType( sjf_interpolators::interpolatorTypes type )
        {
            for ( auto & d : m_delays )
                d.setInterpolationType( type );
            for ( auto & a : m_diffusers )
                a.setInterpolationType( type );
        }
        
        /** determines which type of mixing should be used */
        void setMixType( mixers type )
        {
            if (m_mixType == type){ return; }
            m_mixType = type;
            
        }
        
        /** sets whether feedback should be limited. This adds a nonlinearity within the loop, increasing cpu load slightly, but preventing overloads( hopefully ) */
        void setControlFB( bool shouldLimitFeedback ){ m_fbControl = shouldLimitFeedback; }
        
    private:
        inline void noMix( std::vector< T > delayed ) { return; }
        inline void hadamardMix( std::vector< T > delayed ){ sjf::mixers::Hadamard< T >::inPlace( delayed.data(), NCHANNELS ); }
        inline void householderMixMix( std::vector< T > delayed ) { sjf::mixers::Householder< T >::mixInPlace( delayed.data(), NCHANNELS ); }
        
                
        
        const int NCHANNELS;
        
        std::vector< delayLine::delay< T > > m_delays;
        std::vector< filters::damper< T > > m_dampers, m_lowDampers;
        std::vector< filters::oneMultAP< T > > m_diffusers;
        std::vector< T > m_delayTimesSamps, m_apDelayTimesSamps, m_fbGains;
        T m_decayInMS{1000}, m_SR{44100}, m_damping{0.2}, m_lowDamping{0.95}, m_diffusion{0.5};
        
        mixers m_mixType = mixers::hadamard;
        int m_interpType = DEFAULT_INTERP;
        
        sjf::utilities::classMemberFunctionPointer< fdn, void, std::vector< T > > mixer{ this, &fdn::hadamardMix };
        
        bool m_fbControl{false};
    };
}

#endif /* sjf_rev_fdn_h */
