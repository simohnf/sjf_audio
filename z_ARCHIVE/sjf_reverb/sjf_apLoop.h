//
//  sjf_apLoop.h
//
//  Created by Simon Fay on 27/03/2024.
//


#ifndef sjf_rev_apLoop_h
#define sjf_rev_apLoop_h

#include "../sjf_rev.h"

namespace sjf::rev
{
    /**
     A basic allpass loop in the style of Keith Barrhttp://www.spinsemi.com/knowledge_base/effects.html#Reverberation
     The architecture can have any number of stages and any number of allpass loops per stage
     Each stage is separated by a further delay and then a lowpass filter
     This version does not use a single loop
     */

    template < typename Sample, rev::fbLimiters::fbLimiterTypes limitType  = rev::fbLimiters::fbLimiterTypes::none, interpolation::interpolatorTypes interpType = interpolation::interpolatorTypes::pureData >
    class allpassLoop 
    {
    public:
        allpassLoop( const size_t stages = 6, const size_t apPerStage = 2 ): NSTAGES( stages ), NAP_PERSTAGE( apPerStage )
        {
            utilities::vectorResize( m_aps, NSTAGES, NAP_PERSTAGE );
            m_delays.resize( NSTAGES );
            m_dampers.resize( NSTAGES );
            m_lowDampers.resize( NSTAGES );
            m_gains.resize( NSTAGES );
            
            utilities::vectorResize( m_delayTimesSamps, NSTAGES, NAP_PERSTAGE + 1, static_cast<Sample>(0) );
            utilities::vectorResize( m_diffusions, NSTAGES, NAP_PERSTAGE, static_cast<Sample>(0.5) );
            // just ensure that delaytimes are set to begin with
            for ( auto s = 0; s < NSTAGES; ++s )
                for ( auto d = 0; d < NAP_PERSTAGE+1; ++d )
                    setDelayTimeSamples( std::round(rand01() * 4410 + 4410), s, d );
            
            setDecay( m_decayInMS );
        }
        ~allpassLoop(){}
        
        /**
         This must be called before first use in order to set basic information such as maximum delay lengths and sample rate
         */
        void initialise( const Sample sampleRate )
        {
            m_SR = sampleRate > 0 ? sampleRate : m_SR;
            auto delSize = m_SR / 2;
            for ( auto s = 0; s < NSTAGES; ++s )
            {
                for ( auto a = 0; a < NAP_PERSTAGE; ++a )
                    m_aps[ s ][ a ].initialise( delSize );
                m_delays[ s ].initialise( delSize );
            }
            m_lastSamp = 0;
        }
        
        /**
         This allows you to set all of the delay times
         */
        void setDelayTimesSamples( const twoDVect< Sample >& delayTimesSamps)
        {
            assert( delayTimesSamps.size() == NSTAGES );
            assert( delayTimesSamps[ 0 ].size() == NAP_PERSTAGE+1 );
            for ( auto s = 0; s < NSTAGES; ++s )
                for ( auto d = 0; d < NAP_PERSTAGE+1; ++d )
                    m_delayTimesSamps[ s ][ d ] = delayTimesSamps[ d ][ s ];
        }
        
        /**
         This sets the amount of diffusion
         i.e. the allpass coefficient ( keep -0.9 < diff < 0.9 for safety )
         */
        void setDiffusion( const Sample diff )
        {
            assert ( diff > -0.9 && diff < 0.9 );
            for ( auto s = 0; s < NSTAGES; ++s )
                for ( auto a = 0; a < NAP_PERSTAGE; ++a )
                    m_diffusions[ s ][ a ] = diff;
        }
        
        /**
         This allows you to set a single the delay time
         */
        void setDelayTimeSamples( const Sample dt, const size_t stage, const size_t delayNumber )
        {
            m_delayTimesSamps[ stage ][ delayNumber ] = dt;
        }
        
        /**
         This sets the desired decay time in milliseconds
         */
        void setDecay( const Sample decayInMS )
        {
            if( m_decayInMS == decayInMS ){ return; }
            m_decayInMS = decayInMS;
            for ( auto s = 0; s < NSTAGES; ++s )
            {
                auto del = 0.0;
                for ( auto d = 0; d < NAP_PERSTAGE + 1; ++d )
                    del += m_delayTimesSamps[ s ][ d ];
                del  /= ( m_SR * 0.001 );
                m_gains[ s ] = sjf_calculateFeedbackGain< Sample >( del, m_decayInMS );
            }
        }
        
        /**
         Get the current decay time in ms
         */
        auto getDecayInMs() const { return m_decayInMS; }
        
        /**
         This sets the amount of damping applied between each section of the loop ( must be >= 0 and <= 1 )
         */
        void setDamping( const Sample dampCoef )
        {
            assert ( dampCoef > 0 && dampCoef < 1 );
            m_damping = dampCoef;
        }
        
        /**
         This sets the amount of low frequency damping applied between each section of the loop ( must be >= 0 and <= 1 )
         */
        void setDampingLow( const Sample dampCoef )
        {
            assert ( dampCoef > 0 && dampCoef < 1 );
            m_lowDamping = dampCoef;
        }
        
        /**
         return the number of stages in the loop
         */
        auto getNStages( ) const { return NSTAGES; }
        
        /**
         return the number of allpass filter in each stage of the loop
         */
        auto getNApPerStage( ) const { return NAP_PERSTAGE; }
        
        /**
         This should be called for every sample in the block
         The input is:
            array of samples, one for each channel in the delay network (up & downmixing must be done outside the loop
         */
        void processInPlace( vect< Sample >& samples )
        {
            auto nChannels = samples.size();
            auto output = vect< Sample >( nChannels, 0 );
            auto chanCount = 0;
            auto samp = m_lastSamp;
            for ( auto s = 0; s < NSTAGES; ++s )
            {
                samp += samples[ chanCount ];

                for ( auto a = 0; a < NAP_PERSTAGE; ++a )
                    samp = m_aps[ s ][ a ].process( samp, m_delayTimesSamps[ s ][ a ], m_diffusions[ s ][ a ] );
                samp = m_dampers[ s ].process( samp, m_damping );
                samp = m_lowDampers[ s ].processHP( samp, m_lowDamping );
                output[ chanCount ] += samp;
                m_delays[ s ].setSample( samp * m_gains[ s ] );
                samp = m_delays[ s ].getSample( m_delayTimesSamps[ s ][ NAP_PERSTAGE ] );
                chanCount = ( ++chanCount >= nChannels ) ? 0 : chanCount;
            }
            m_lastSamp = m_limiter( samp );
            samples = output;
            return;
        }
        
        
        /** sets whether feedback should be limited. This adds a nonlinearity within the loop, increasing cpu load slightly, but preventing overloads( hopefully ) */
        void setControlFB( const bool shouldLimitFeedback ) { m_fbControl = shouldLimitFeedback; }
        
    private:
        const size_t NSTAGES, NAP_PERSTAGE;
        twoDVect< filters::oneMultAP < Sample, interpType > > m_aps;
        vect< delayLine::delay < Sample, interpType > > m_delays;
        vect< filters::damper < Sample > > m_dampers, m_lowDampers;
        
        vect< Sample > m_gains;
        twoDVect< Sample > m_delayTimesSamps;
        twoDVect< Sample > m_diffusions;
        
        Sample m_lastSamp{0};
        Sample m_SR{44100}, m_decayInMS{100};
        Sample m_damping{0.2}, m_lowDamping{0.95};
        
        bool m_fbControl{false};
        fbLimiters::limiter< Sample, limitType > m_limiter;
    };
}


#endif /* sjf_rev_apLoop_h */
