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
        rotDelDif( const size_t nChannels = 8, const size_t nStages = 5 ) : NCHANNELS(nChannels), NSTAGES(nStages), m_hadMixer(nChannels)
        {
            utilities::vectorResize( m_delays, NSTAGES, NCHANNELS );
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
            for ( auto & s : m_delays )
                for ( auto & d : s )
                    d.initialise( maxDelayTimeSamps );
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
        
        
        
//        /**
//         Set how the channels will be shuffled/rotated between each stage
//         No test will be made to ensure that all of the channels are read so this is your responsibility
//         */
//        void setRotationMatrix( const twoDVect< size_t >& matrix )
//        {
//            assert( matrix.size() == NSTAGES );
//            for ( auto i = 0; i < NSTAGES; ++i )
//            {
//                assert( matrix[ i ].size() == NCHANNELS );
//                for ( auto j = 0; j < NSTAGES; ++j )
//                {
//                    assert( matrix[ i ][ j ] < NCHANNELS );
//                    m_rotationMatrix[ i ][ j ] = matrix[ i ][ j ];
//                }
//            }
//        }
//
//        /**
//         Set how the channels will be shuffled/rotated between each stage
//         No test will be made to ensure that all of the channels are read so this is your responsibility
//         */
//        void setRotationMatrix( const vect< size_t >& matrix, const size_t stage )
//        {
//            assert( matrix.size() == NCHANNELS );
//            for ( auto i = 0; i < NCHANNELS; ++i )
//            {
//                assert( matrix[ i ] < NCHANNELS );
//                m_rotationMatrix[ stage ][ i ] = matrix[ i ];
//            }
//        }
        
        
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
                for ( auto j = 0; j < NCHANNELS; ++j )
                {
                    // first set samples in delay line
                    m_delays[ i ][ j ].setSample( samps[ j ] );
                    // then read samples from delay, but rotate channels and flip polarity of some
                    samps[ j ] = m_delays[ i ][ j ].getSample( m_delayTimesSamps[ i ][ j ] ) * m_polFlip[ i ][ j ];
//                    samps[ j ] = m_delays[ i ][ j ].getSample( m_delayTimesSamps[ i ][ m_rotationMatrix[ i ][ j ] ] ) * m_polFlip[ i ][ j ];
                    samps[ j ] = m_dampers[ i ][ j ].process( samps[ j ], m_damping[ i ][ j ] );
                    m_hadMixer.inPlace( samps.data(), NCHANNELS );
                }
            }
        }
    private:
        const size_t NCHANNELS;
        const size_t NSTAGES;
        
        twoDVect< delayLine::delay< Sample, INTERPOLATION_FUNCTOR > > m_delays;
        twoDVect< filters::damper< Sample > > m_dampers;
        twoDVect< Sample > m_delayTimesSamps, m_damping;
        
        twoDVect< Sample > m_polFlip; // multiply by one or minus one
//        twoDVect< size_t > m_rotationMatrix;
        
        Sample m_SR = 44100;
        sjf::mixers::Hadamard< Sample > m_hadMixer;
    };
}


#endif /* sjf_rev_sjf_rotDelDif_h */
