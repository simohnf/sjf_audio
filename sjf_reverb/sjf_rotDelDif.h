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
    template< typename T >
    class rotDelDif
    {
//        std::vector< std::vector< delay< T >, NCHANNELS >, NSTAGES > m_delays;
//        std::array< std::array< damper< T >, NCHANNELS >, NSTAGES > m_dampers;
//        std::array< std::array< T, NCHANNELS >, NSTAGES > m_delayTimesSamps, m_damping;
//
//        std::array< std::array< bool, NCHANNELS >, NSTAGES > m_polFlip;
//        std::array< std::array< int,  NCHANNELS >, NSTAGES > m_rotationMatrix;
    public:
        rotDelDif( int nChannels, int nStages) : NCHANNELS( nChannels), NSTAGES( nStages )
        {
            m_delays.resize( NSTAGES );
            m_dampers.resize( NSTAGES );
            m_delayTimesSamps.resize( NSTAGES );
            m_damping.resize( NSTAGES );
            m_polFlip.resize( NSTAGES );
            m_rotationMatrix.resize( NSTAGES );
            for ( auto s = 0; s < NSTAGES; s++ )
            {
                m_delays[ s ].resize( NCHANNELS );
                m_dampers[ s ].resize( NCHANNELS );
                m_delayTimesSamps[ s ].resize( NCHANNELS, 0 );
                m_damping[ s ].resize( NCHANNELS, 0 );
                m_rotationMatrix[ s ].resize( NCHANNELS, 0 );
                m_polFlip[ s ].resize( NCHANNELS, false );
            }
            
//            for ( auto s = 0; s < NSTAGES; s++ )
//            {
//                for ( auto d = 0; d < NCHANNELS; d++ )
//                    {
//                        m_delayTimesSamps[ s ][ d ] = ( rand01() * 4410 ) + 2205; // random delay times just so something is set
//                        m_polFlip[ s ][ d ] = ( rand01() > 0.5 );
//                        m_rotationMatrix[ s ][ d ] = ( d - s ) & NCHANNELS;
//                    }
//            }
        }
        ~rotDelDif(){}
        
        /**
         This must be called before first use in order to set basic information such as maximum delay lengths
         Size should be a power of 2!!!
         */
        void initialise( T sampleRate, T maxDelayTimeSamps )
        {
            m_SR = sampleRate;
            for ( auto & s : m_delays )
                for ( auto & d : s )
                    d.initialise( maxDelayTimeSamps );
        }
        
        
        /**
         Set all of the delayTimes
         */
        void setDelayTimes( const std::vector< std::vector< T > >& dt )
        {
            assert( dt.size() == NSTAGES );
            for ( auto s = 0; s < NSTAGES; s++ )
            {
                assert( dt[ s ].size() == NCHANNELS );
                for ( auto d = 0; d < NCHANNELS; d++ )
                    m_delayTimesSamps[ s ][ d ] = dt[ s ][ d ];
            }
        }
        
        /**
         Set one of the delayTimes
         */
        void setDelayTime( const T dt, size_t stage, size_t channel )
        {
            assert( stage < NSTAGES );
            assert( channel < NCHANNELS );
            m_delayTimesSamps[ stage ][ channel ] = dt;
        }
//
        /**
         Set all of the damping coefficients
         */
        void setDamping( const std::vector< std::vector< T > >& damp )
        {
            assert( damp.size() == NSTAGES );
            for ( auto s = 0; s < NSTAGES; s++ )
            {
                assert( damp[ s ].size() == NCHANNELS );
                for ( auto d = 0; d < NCHANNELS; d++ )
                    m_damping[ s ][ d ] = damp[ s ][ d ];
            }
        }

        /**
         Set all of the damping coefficients
         */
        void setDamping( T damp )
        {
            for ( auto s = 0; s < NSTAGES; s++ )
                for ( auto d = 0; d < NCHANNELS; d++ )
                    m_damping[ s ][ d ] = damp;
        }
        
        /**
         Set one of the damping coefficients
         */
        void setDamping( T damp, size_t stage, size_t channel )
        {
            assert( stage < NSTAGES );
            assert( channel < NCHANNELS );
            m_damping[ stage ][ channel ] = damp;
        }
        
        
        
        /**
         Set how the channels will be shuffled/rotated between each stage
         No test will be made to ensure that all of the channels are read so this is your responsibility
         */
        void setRotationMatrix( std::vector < std::vector < size_t > >& matrix )
        {
            assert( matrix.size() == NSTAGES );
            for ( auto s = 0; s < NSTAGES; s++ )
            {
                assert( matrix[ s ].size() == NCHANNELS );
                for ( auto c = 0; c < NCHANNELS; c++ )
                {
                    assert( matrix[ s ][ c ] < NCHANNELS );
                    m_rotationMatrix[ s ][ c ] = matrix[ s ][ c ];
                }
            }
        }
        
        
        /**
         Set polarity flips
         */
        void setPolarityFlips( std::vector < std::vector < bool > >& matrix )
        {
            assert( matrix.size() == NSTAGES );
            for ( auto s = 0; s < NSTAGES; s++ )
            {
                assert( matrix[ s ].size() == NCHANNELS );
                for ( auto c = 0; c < NCHANNELS; c++ )
                {
                    assert( matrix[ s ][ c ] < NCHANNELS );
                    m_polFlip[ s ][ c ] = matrix[ s ][ c ];
                }
            }
        }
        
        /**
         This should be called once for every sample in the block
         The Input is:
            Samples to be processed ( for ease of use any up mixing to the number of channels is left to the user )
         Output:
            None - Samples are processed in place and any down mixing is left to the user
         */
        void processInPlace( std::vector< T >& samps, int interpType = 0 )
        {
            int rCh = 0;
            assert( samps.size() == NCHANNELS );
            // we need to go through each stage
            for ( auto s = 0; s < NSTAGES; s++ )
            {
                for ( auto c = 0; c < NCHANNELS; c++ )
                {
                    // first set samples in delay line
                    m_delays[ s ][ c ].setSample( samps[ c ] );
                    // then read samples from delay, but rotate channels
                    rCh = m_rotationMatrix[ s ][ c ];
                    samps[ c ] = m_delays[ s ][ c ].getSample( m_delayTimesSamps[ s ][ c ], interpType );
//                    samps[ c ] = m_polFlip[ s ][ c ] ?  : -m_delays[ s ][ rCh ];
                    samps[ c ] = m_dampers[ s ][ c ].process( samps[ c ], m_damping[ s ][ c ] );
                    sjf::mixers::Hadamard< T >::inPlace( samps.data(), NCHANNELS );
                }
            }
        }
        
    private:
        const int NCHANNELS;
        const int NSTAGES;
        
        std::vector< std::vector< delay< T > > > m_delays;
        std::vector< std::vector< damper< T > > > m_dampers;
        std::vector< std::vector< T > > m_delayTimesSamps, m_damping;
        
        std::vector< std::vector< bool > > m_polFlip;
        std::vector< std::vector< int > > m_rotationMatrix;
        
        T m_SR = 44100;
    };
}


#endif /* sjf_rev_sjf_rotDelDif_h */
