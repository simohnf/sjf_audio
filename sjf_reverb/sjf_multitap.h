//
//  sjf_multitap.h
//
//  Created by Simon Fay on 27/03/2024.
//


#ifndef sjf_rev_multitap_h
#define sjf_rev_multitap_h

#include "../sjf_rev.h"

namespace sjf::rev
{
    /**
     Basic multitap delayLine for use primarily as part of an early reflection generator
     No filtering or feedback is applied
     */
    template < typename Sample, typename INTERPOLATION_FUNCTOR = interpolation::noneInterpolate< Sample > >
    class multiTap
    {

    public:
        multiTap( const size_t maxNTaps = 64 ) noexcept : MAXNTAPS( maxNTaps ), m_nTaps( maxNTaps )
        {
            m_delayTimesSamps.resize( MAXNTAPS, 0 );
            m_gains.resize( MAXNTAPS, 0 );
        }
        ~multiTap(){}
        
        
        /**
         This must be called before first use in order to set basic information such as maximum delay length and sample rate
         */
        void initialise( const Sample sampleRate )
        {
            if( sampleRate<=0 ){ return; }
            m_delay.initialise( (sampleRate / 2) );
        }
        
        /**
         This must be called before first use in order to set the maximum delay length
         */
        void initialise( const long maxDelayInSamps )
        {
            assert( maxDelayInSamps > 0 );
            m_delay.initialise( sjf_isPowerOf( maxDelayInSamps, 2 ) ? maxDelayInSamps : sjf_nearestPowerAbove( maxDelayInSamps, 2l ) );
        }
        
        
        /**
         Set all of the delayTimes in samples
         */
        void setDelayTimesSamps( const vect< Sample >& dt )
        {
            assert( dt.size() == MAXNTAPS );
            for ( auto i = 0; i < MAXNTAPS; ++i )
                m_delayTimesSamps[ i ] = dt[ i ];
        }
        
        /**
         Set the delayTime in samples for a single tap ( must be greater than 0!!!)
         */
        void setDelayTimeSamps( const Sample dt, const size_t tapNum )
        {
            assert( dt > 0 );
            m_delayTimesSamps[ tapNum ] = dt;
        }
        
        /**
         Set all of the gains
         */
        void setGains( const vect< Sample >& gains )
        {
            assert( gains.size() == MAXNTAPS );
            for ( auto  i = 0; i < MAXNTAPS; ++i )
                m_gains[ i ] = gains[ i ];
        }
        
        /**
         Set the gain for an individual tap
         */
        void setGain( const Sample gain, const size_t tapNum )
        {
            assert( tapNum < MAXNTAPS );
            m_gains[ tapNum ] = gain;
        }
        
        /**
         Using this you can change the number of active taps
         */
        void setNTaps( const size_t nTaps )
        {
            assert( nTaps > 0 && nTaps <= MAXNTAPS );
            m_nTaps = nTaps;
        }
        
        /**
         Return the maximum number of taps possible
         */
        size_t getMaxNTaps() const { return MAXNTAPS; }
        
        /**
         This processes one sample
         Input:
            sample to be inserted into the delayLine
         Output:
            combination of all of the taps
         */
        inline Sample process( Sample x )
        {
            Sample output = 0.0;
            for ( auto i = 0; i < m_nTaps; ++i )
                output += getSample( i );
            setSample( x );
            return output;
        }
        
        /**
         Using this you can get the output of a single tap at a time
         Input:
            The tap number
         output:
            Output of delay tap
         */
        inline Sample getSample( const size_t tapNum ) const
        {
            return m_delay.getSample( m_delayTimesSamps[ tapNum ] ) * m_gains[ tapNum ];
        }
        
        /**
         Push a sample value into the delay line
         Input:
            value to store in delay line
         */
        inline void setSample( const Sample x )
        {
            m_delay.setSample( x );
        }
        
    private:
        const size_t MAXNTAPS;
        vect< Sample > m_delayTimesSamps, m_gains;
        size_t m_nTaps = MAXNTAPS;
        delayLine::delay< Sample, INTERPOLATION_FUNCTOR > m_delay;
    };
}


#endif /* sjf_rev_multitap_h */
