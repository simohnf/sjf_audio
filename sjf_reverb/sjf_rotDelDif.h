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
    template< typename T, int NCHANNELS, int NSTAGES >
    class rotDelDif
    {
    private:
        std::array< std::array< delay< T >, NCHANNELS >, NSTAGES > m_delays;
        std::array< std::array< damper< T >, NCHANNELS >, NSTAGES > m_dampers;
        std::array< std::array< T, NCHANNELS >, NSTAGES > m_delayTimesSamps, m_damping;
        
        std::array< std::array< bool, NCHANNELS >, NSTAGES > m_polFlip;
        std::array< std::array< int,  NCHANNELS >, NSTAGES > m_rotationMatrix;
        
        T m_SR = 44100;
        
    public:
        rotDelDif()
        {
            for ( auto s = 0; s < NSTAGES; s++ )
            {
                for ( auto d = 0; d < NCHANNELS; d++ )
                    {
                        m_delayTimesSamps[ s ][ d ] = ( rand01() * 4410 ) + 2205; // random delay times just so something is set
                        m_polFlip[ s ][ d ] = ( rand01() > 0.5 );
                        m_rotationMatrix[ s ][ d ] = ( d - s ) & NCHANNELS;
                    }
            }
        }
        ~rotDelDif(){}
        
        /**
         This must be called before first use in order to set basic information such as maximum delay lengths
         Size should be a power of 2!!!
         */
        void initialise( T sampleRate, T maxDelaySize )
        {
            m_SR = sampleRate;
            for ( auto & s : m_delays )
                for ( auto & d : s )
                    d.initialise( maxDelaySize );
        }
        
        
        /**
         Set all of the delayTimes
         */
        void setDelayTimes( const std::array< std::array< T, NCHANNELS >, NSTAGES >& dt )
        {
            for ( auto s = 0; s < NSTAGES; s++ )
                for ( auto d = 0; d < NCHANNELS; d++ )
                    m_delayTimesSamps[ s ][ d ] = dt[ s ][ d ];
        }
        
        /**
         Set all of the damping coefficients
         */
        void setDamping( const std::array< std::array< T, NCHANNELS >, NSTAGES >& damp )
        {
            for ( auto s = 0; s < NSTAGES; s++ )
                for ( auto d = 0; d < NCHANNELS; d++ )
                    m_damping[ s ][ d ] = damp[ s ][ d ];
        }
        
        /**
         This should be called once for every sample in the block
         The Input is:
            Samples to be processed ( for ease of use any up mixing to the number of channels is left to the user )
         Output:
            None - Samples are processed in place and any down mixing is left to the user
         */
        void processInPlace( std::array< T, NCHANNELS >& samps )
        {
            int rch = 0;
            // we need to go through each stage
            for ( auto s = 0; s < NSTAGES; s++ )
            {
                for ( auto d = 0; d < NCHANNELS; d++ )
                {
                    // first set samples in delay line
                    m_delays[ s ][ d ].setSample( samps[ d ] );
                    // then read samples from delay, but rotate channels
                }
            }
        }
        
    private:
    };
}


#endif /* sjf_rev_sjf_rotDelDif_h */
