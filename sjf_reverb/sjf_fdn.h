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
    template< typename Sample >
    class fdn
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
            setSetValVariant();
        }
        ~fdn(){}
        
        /**
         This must be called before first use in order to set basic information such as maximum delay lengths and sample rate
         */
        void initialise( int maxSizePerChannelSamps, int maxSizePerAPChannelSamps, Sample sampleRate )
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
        void setDelayTimes( const std::vector< Sample >& dt )
        {
            assert( dt.size() == NCHANNELS );
            for ( auto c = 0; c < NCHANNELS; c ++ )
                m_delayTimesSamps[ c ] = dt[ c ];
            setDecayInMS( m_decayInMS );
        }
        
        /**
         This allows you to set one of the delay times, this does not reset the decay time calculation so you will need to reset that using setDecayInMS()!!!
         */
        void setDelayTime( const Sample dt, const int delayNumber )
        {
            assert( delayNumber < NCHANNELS );
            m_delayTimesSamps[ delayNumber ] = dt;
        }
        
        /**
         This allows you to set all of the delay times used by the allpass filters for diffusion.
         */
        void setAPTimes( const std::vector< Sample >& dt )
        {
            assert( dt.size() == NCHANNELS );
            for ( auto c = 0; c < NCHANNELS; c ++ )
                m_apDelayTimesSamps[ c ] = dt[ c ];
            setDecayInMS( m_decayInMS );
        }
        
        /**
         This allows you to set one of the delay times used by the allpass filters for diffusion,  this does not reset the decay time calculation so you will need to reset that using setDecayInMS()!!! 
         */
        void setAPTime( const Sample dt, const int delayNumber )
        {
            assert( delayNumber < NCHANNELS );
            m_apDelayTimesSamps[ delayNumber ] = dt;
        }
        
        
        /**
         This sets the amount of diffusion ( must be greater than -1 and less than 1
         0 sets no diffusion
         */
        void setDiffusion( Sample diff )
        {
            if ( (m_diffusion==0.0||diff==0.0) && (m_diffusion!=diff) )
            {
                m_diffusion = diff;
                setSetValVariant();
                return;
            }
            m_diffusion = diff;
        }
        
        
        /**
         This sets the amount of high frequency damping applied in the loop ( must be >= 0 and <= 1 )
         */
        void setDamping( Sample dampCoef ) { m_damping = dampCoef < 1 ? (dampCoef > 0 ? dampCoef : 00001) : 0.9999; }
        
        /**
         This sets the amount of low frequency damping applied in the loop ( must be >= 0 and <= 1 )
         */
        void setDampingLow( Sample dampCoef ) { m_lowDamping = dampCoef < 1 ? (dampCoef > 0 ? dampCoef : 00001) : 0.9999; }
        
        /**
         This sets the desired decay time in milliseconds
         */
        void setDecayInMS( Sample decayInMS )
        {
            if ( m_decayInMS == decayInMS ){ return; }
            m_decayInMS = decayInMS;
            auto dt = 0.0;
            for ( auto c = 0; c < NCHANNELS; c++ )
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
        void processInPlace( std::vector< Sample >& samples )
        {
            assert( samples.size() == NCHANNELS );
            std::vector< Sample > delayed( NCHANNELS, 0 );
            for ( auto c = 0; c < NCHANNELS; c++ )
            {
                delayed[ c ] = m_delays[ c ].getSample( m_delayTimesSamps[ c ] );
                delayed[ c ] = m_dampers[ c ].process( delayed[ c ], m_damping ); // lp filter
                delayed[ c ] = m_lowDampers[ c ].processHP( delayed[ c ], m_lowDamping ); // hp filter
            }
            
//            mixer( delayed );
            switch ( m_mixType ) {
                case mixers::hadamard:
                    hadamardMix( delayed );
                    break;
                case mixers::householder:
                    householderMixMix( delayed );
                    break;
                default:
                    break;
            }
        
            for ( auto c = 0; c < NCHANNELS; c++ )
            {
                auto val = samples[ c ]+delayed[ c ]*m_fbGains[ c ];
                std::visit( fdnSetValVisitor{ val, c, *this }, m_setValTypes[ m_setValVisitorType ] );
            }
            samples = delayed;
            return;
        }
        
        /** Set the interpolation Type to be used, the interpolation type see @sjf_interpolators */
        void setInterpolationType( sjf::interpolation::interpolatorTypes type )
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
        void setControlFB( bool shouldLimitFeedback )
        {
            m_fbControl = shouldLimitFeedback;
            setSetValVariant();
        }
        
    private:
        void setSetValVariant()
        {
            if( !m_fbControl && m_diffusion == 0 )
                m_setValVisitorType = 0;
            else if( m_fbControl && m_diffusion == 0 )
                m_setValVisitorType = 1;
            else if( !m_fbControl && m_diffusion != 0 )
                m_setValVisitorType = 2;
            else if( m_fbControl && m_diffusion != 0 )
                m_setValVisitorType = 3;
        }
        
        
        struct noDiffNoLimit
        {
            void operator()( Sample val, size_t c, fdn& parent )
                { parent.m_delays[ c ].setSample( val ); }
        };
        struct noDiffLimit
        {
            void operator()( Sample val, size_t c, fdn& parent )
                { parent.m_delays[ c ].setSample( nonlinearities::tanhSimple( val ) ); }
        };
        struct diffNoLimit
        {
            void operator()( Sample val, size_t c, fdn& parent )
            {
                val = parent.m_diffusers[ c ].process( val, parent.m_apDelayTimesSamps[ c ], parent.m_diffusion );
                parent.m_delays[ c ].setSample( val ); }
        };
        struct diffLimit
        {
            void operator()( Sample val, size_t c, fdn& parent )
            {
                val = parent.m_diffusers[ c ].process( val, parent.m_apDelayTimesSamps[ c ], parent.m_diffusion );
                parent.m_delays[ c ].setSample( nonlinearities::tanhSimple( val ) );
            }
        };
        struct fdnSetValVisitor
        {
            Sample val;
            int channel;
            fdn& parent;
            void operator ()( noDiffNoLimit& s ){ s( val, channel, parent ); }
            void operator ()( noDiffLimit& s ){ s( val, channel, parent ); }
            void operator ()( diffNoLimit& s ){ s( val, channel, parent ); }
            void operator ()( diffLimit& s ){ s( val, channel, parent ); }
        };
        int m_setValVisitorType = 0;
        using setValVariant = std::variant< noDiffNoLimit, noDiffLimit, diffNoLimit, diffLimit >;
        std::array< setValVariant, 4 > m_setValTypes{ noDiffNoLimit(), noDiffLimit(), diffNoLimit(), diffLimit() };
        
        
        inline void noMix( std::vector< Sample > delayed ) { return; }
        inline void hadamardMix( std::vector< Sample > delayed ){ sjf::mixers::Hadamard< Sample >::inPlace( delayed.data(), NCHANNELS ); }
        inline void householderMixMix( std::vector< Sample > delayed ) { sjf::mixers::Householder< Sample >::mixInPlace( delayed.data(), NCHANNELS ); }
        
                
        
        const int NCHANNELS;
        
        std::vector< delayLine::delay< Sample > > m_delays;
        std::vector< filters::damper< Sample > > m_dampers, m_lowDampers;
        std::vector< filters::oneMultAP< Sample > > m_diffusers;
        std::vector< Sample > m_delayTimesSamps, m_apDelayTimesSamps, m_fbGains;
        Sample m_decayInMS{1000}, m_SR{44100}, m_damping{0.2}, m_lowDamping{0.95}, m_diffusion{0.5};
        
        mixers m_mixType = mixers::hadamard;

//        sjf::utilities::classMemberFunctionPointer< fdn, void, std::vector< Sample > > mixer{ this, &fdn::hadamardMix };
        
        bool m_fbControl{false};
    };
}

#endif /* sjf_rev_fdn_h */
