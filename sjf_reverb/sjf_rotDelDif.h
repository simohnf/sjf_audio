//
//  sjf_sjf_rotDelDif.h
//
//  Created by Simon Fay on 02/04/2024.
//


#ifndef sjf_rev_sjf_rotDelDif_h
#define sjf_rev_sjf_rotDelDif_h

#include "../sjf_rev.h"
namespace sjf::rev
{
    /**
     A structure of delays with rotation, mixing, and lpf between each stage
     For use as a diffuser a là Geraint Luff "Let's Write a Reverb"... also Miller Puckette
     */
    template< typename Sample, typename INTERPOLATION_FUNCTOR = interpolation::fourPointInterpolatePD< Sample > >
    class rotDelDif
    {
    public:
        rotDelDif( const size_t nChannels = 8, const size_t nStages = 5, const size_t nModChannels = 8 ) : NCHANNELS(nChannels), NSTAGES(nStages), NMODCHANNELS( nModChannels<0?0:nModChannels>NCHANNELS?NCHANNELS:nModChannels), m_hadMixer(nChannels)
        {
            utilities::vectorResize( m_modDelays, NSTAGES, NMODCHANNELS );
            utilities::vectorResize( m_delays, NSTAGES, NCHANNELS-NMODCHANNELS );
            utilities::vectorResize( m_dampers, NSTAGES, NCHANNELS );
            utilities::vectorResize( m_delayTimesSamps, NSTAGES, NCHANNELS, static_cast<Sample>(0) );
            utilities::vectorResize( m_damping, NSTAGES, NCHANNELS, static_cast<Sample>(0) );
            utilities::vectorResize( m_polFlip, NSTAGES, NCHANNELS, static_cast<Sample>(1) );
//            utilities::vectorResize( m_rotationMatrix, NSTAGES, NCHANNELS, static_cast<size_t>(0) );
        }
        ~rotDelDif(){}
        
        /**
         This must be called before first use in order to set basic information such as maximum delay lengths
         Size should be a power of 2!!!
         */
        void initialise( const Sample sampleRate, const Sample maxDelayTimeSamps )
        {
            m_SR = sampleRate;
//            for ( auto & s : m_delays )
//                for ( auto & d : s )
//                    d.initialise( maxDelayTimeSamps );
            for ( auto & s : m_modDelays )
                s.initialise( maxDelayTimeSamps );
            for ( auto & s : m_delays )
                s.initialise( maxDelayTimeSamps );
//                for ( auto & d : s )
//                    d.initialise( maxDelayTimeSamps );
        }
        
        
        /**
         Set all of the delayTimes
         */
        void setDelayTimes( const twoDVect< Sample >& dt )
        {
            assert( dt.size() == NSTAGES );
            for ( auto i = 0; i < NSTAGES; ++i )
            {
                assert( dt[ i ].size() == NCHANNELS );
                for ( auto j = 0; j < NSTAGES; ++j )
                    m_delayTimesSamps[ i ][ j ] = dt[ i ][ j ];
            }
        }
        
        /**
         Set one of the delayTimes
         */
        void setDelayTime( const Sample dt, const size_t stage, const size_t channel )
        {
            assert( stage < NSTAGES );
            assert( channel < NCHANNELS );
            m_delayTimesSamps[ stage ][ channel ] = dt;
        }
//
        /**
         Set all of the damping coefficients
         */
        void setDamping( const twoDVect< Sample >& damp )
        {
            assert( damp.size() == NSTAGES );
            for ( auto i = 0; i < NSTAGES; ++i )
            {
                assert( damp[ i ].size() == NCHANNELS );
                for ( auto j = 0; j < NSTAGES; ++j )
                    m_damping[ i ][ j ] = damp[ i ][ j ];
            }
        }

        /**
         Set all of the damping coefficients
         */
        void setDamping( const Sample damp )
        {
            for ( auto i = 0; i < NSTAGES; ++i )
                for ( auto j = 0; j < NSTAGES; ++j )
                    m_damping[ i ][ j ] = damp;
        }
        
        /**
         Set one of the damping coefficients
         */
        void setDamping( const Sample damp, const size_t stage, const size_t channel )
        {
            assert( stage < NSTAGES );
            assert( channel < NCHANNELS );
            m_damping[ stage ][ channel ] = damp;
        }
        
        
        /**
         Set polarity flips
         */
        void setPolarityFlips( const twoDVect< bool >& matrix )
        {
            assert( matrix.size() == NSTAGES );
            for ( auto i = 0; i < NSTAGES; ++i )
            {
                assert( matrix[ i ].size() == NCHANNELS );
                for ( auto j = 0; j < NSTAGES; ++j )
                {
                    assert( matrix[ i ][ j ] < NCHANNELS );
                    m_polFlip[ i ][ j ] = matrix[ i ][ j ] ? -1 : 1;
                }
            }
        }
        
        
        /**
         Set polarity flip of one channel in one stage
         */
        void setPolarityFlip( const bool shouldFlip, const size_t stage, const size_t channel )
        {
            m_polFlip[ stage ][ channel ] = shouldFlip ? -1 : 1;
        }
        
        /**
         This should be called once for every sample in the block
         The Input is:
            Samples to be processed ( for ease of use any up mixing to the number of channels is left to the user )
         Output:
            None - Samples are processed in place and any down mixing is left to the user
         */
        void processInPlace( vect< Sample >& samps )
        {
            assert( samps.size() == NCHANNELS );
            // we need to go through each stage
            for ( auto i = 0; i < NSTAGES; ++i )
            {
                for ( auto j = 0; j < NMODCHANNELS; ++j )
                {
                    // first set samples in delay line
                    m_modDelays[ i ].setSample( j, samps[ j ] );
                    
                    // then read samples from delay, but rotate channels and flip polarity of some
                    samps[ j ] = m_modDelays[ i ].getSample( j, m_delayTimesSamps[ i ][ j ] ) * m_polFlip[ i ][ j ];
                    samps[ j ] = m_dampers[ i ][ j ].process( samps[ j ], m_damping[ i ][ j ] );
                }
                for ( auto j = NMODCHANNELS; j < NCHANNELS; ++j )
                {
                    // first set samples in delay line
                    m_delays[ i ].setSample( j-NMODCHANNELS, samps[ j ] );
                    
                    // then read samples from delay, but rotate channels and flip polarity of some
                    samps[ j ] = m_delays[ i ].getSample( j-NMODCHANNELS, m_delayTimesSamps[ i ][ j ] ) * m_polFlip[ i ][ j ];
                    samps[ j ] = m_dampers[ i ][ j ].process( samps[ j ], m_damping[ i ][ j ] );
                }
                
                m_hadMixer.inPlace( samps.data(), NCHANNELS );
                m_modDelays[ i ].updateWritePos();
                m_delays[ i ].updateWritePos();
            }
        }
    private:
        const size_t NCHANNELS, NSTAGES, NMODCHANNELS;
        
        vect< delayLine::multiChannelDelay<Sample, INTERPOLATION_FUNCTOR> > m_modDelays;
        vect< delayLine::multiChannelDelay<Sample, interpolation::noneInterpolate<Sample>> > m_delays;
        
        twoDVect< filters::damper< Sample > > m_dampers;
        twoDVect< Sample > m_delayTimesSamps, m_damping;
        twoDVect< Sample > m_polFlip; // multiply by one or minus one
        
        Sample m_SR = 44100;
        sjf::mixers::Hadamard< Sample > m_hadMixer;
    };
}


#endif /* sjf_rev_sjf_rotDelDif_h */
