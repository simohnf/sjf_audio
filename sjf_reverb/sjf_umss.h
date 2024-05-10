//
//  sjf_umss.h
//
//  Created by Simon Fay on 27/03/2024.
//


#ifndef sjf_rev_umss_h
#define sjf_rev_umss_h

//#include "../sjf_audioUtilitiesC++.h"
//#include "../sjf_interpolators.h"
//#include "../gcem/include/gcem.hpp"
//#include "sjf_delay.h"
//#include "sjf_damper.h"
//
//#include "sjf_rev_consts.h"

#include "../sjf_rev.h"
#include "../sjf_dcBlock.h"
namespace sjf::rev
{
    /**
     umss a basic implementation of Christopher Moore’s ursa major space station style multitap delay/reverb effect
     */
    template< typename T >
    class umss
    {
    private:
        const int NFBLOOPS, NOUTTAPS;
        const T FBSCALE;
        delayLine::delay< T > m_delayLine;
        damper< T > m_damper;
        sjf::filters::dcBlock< T > m_dcBlock;
        std::vector < T > m_fbDelayTimesSamps;
        std::vector < int > m_outTapDelayTimesSamps;
        std::vector < T > m_outTapGains;
        T m_feedback = 1.0, m_SR = 44100, m_damping = 0.4;
        
    public:
        umss( int nFbLoops, int nOutTaps ) : NFBLOOPS( nFbLoops ), NOUTTAPS( nOutTaps ),  FBSCALE( 1.0 / static_cast<T>( nFbLoops ) )
        {
            m_fbDelayTimesSamps.resize( NFBLOOPS, 0 );
            m_outTapDelayTimesSamps.resize( NOUTTAPS, 0 );
            m_outTapGains.resize( NOUTTAPS, 0 );
            initialise( 44100, 44100 );
            for ( auto f = 0; f < NFBLOOPS; f++ )
                m_fbDelayTimesSamps[ f ] = ( rand01()*22050.0 ) + 4410;
            auto oLevel = 1.0 / std::sqrt(NOUTTAPS);
            for ( auto o = 0; o < NOUTTAPS; o++ )
            {
                m_outTapDelayTimesSamps[ o ] = ( rand01()*22050.0 ) + 4410;
                m_outTapGains[ o ] = oLevel;
            }
        }
        ~umss(){}
        
        /**
        This must be called before first use to set basic information like delayline length etc
         */
        void initialise( T maxDelayInSamps, T sampleRate )
        {
            m_SR = sampleRate;
            if ( !sjf_isPowerOf( maxDelayInSamps, 2 ) )
                maxDelayInSamps = sjf_nearestPowerAbove( maxDelayInSamps, 2 );
            m_delayLine.initialise( sjf_nearestPowerAbove( maxDelayInSamps, 2 ) );
        }
        
        /**
        This should be called once for every sample in the block
         input is:
            the sample to be processed
            interpolation type ( optional, defaults to linear,  see @sjf_interpolators )
         output is the processed sample
         */
        void process( T x, T* outputSamples, int numOutChannels, int interpType = DEFAULT_INTERP )
        {
            T fbLoop = 0.0f;
            // we should add modulation to fb delayTaps
            for ( auto i = 0; i < NFBLOOPS; i++ )
                fbLoop += m_delayLine.getSample( m_fbDelayTimesSamps[ i ], interpType );
            fbLoop *= m_feedback * FBSCALE;
            fbLoop = m_dcBlock.process( fbLoop );
            auto outChan = 0;
            // no modulation for output taps
            for ( auto i = 0; i < NOUTTAPS; i++ )
            {
                assert( outChan <= numOutChannels );
                outputSamples[ outChan ] = m_delayLine.getSample( m_outTapDelayTimesSamps[ i ], 0 ) * m_outTapGains[ i ];
//                outChan += 1;
                outChan  = ++outChan < numOutChannels ? outChan : 0;
            }
            m_delayLine.setSample( m_damper.process( x + ( fbLoop ), m_damping ) );
        }
        
        /**
         Set amount of lowpass filtering added to feedback loop, should be >=0 <=1
         */
        void setDamping( T damping )
        {
            m_damping = damping;
        }
        
        /**
         Set the amount of feedback applied to the loop, should be >=0 <=1
         */
        void setFeedback( T feedback )
        {
            m_feedback = feedback;
        }
//
        
//        /**
//         Set the desired decay time in milliseconds
//         */
//        void setDecay( T decayInMs )
//        {
//            auto fbDT
//        }
        
        /**
         Set the delay time for all of the taps that feedback into the delay loop
         */
        void setFeedbackDelayTimes( const std::vector < T >& fbDelayTimesSamps )
        {
            assert( fbDelayTimesSamps.size() == NFBLOOPS );
            for ( auto i = 0; i < NFBLOOPS; i++ )
                m_fbDelayTimesSamps[ i ] = fbDelayTimesSamps[ i ];
        }
        
        /**
         Set the delay time for one of the taps that feedback into the delay loop
         */
        void setFeedbackDelayTime( T fbDelayTimesSamps, size_t tapNumber )
        {
            m_fbDelayTimesSamps[ tapNumber ] = fbDelayTimesSamps;
        }
            
        
        /**
         Set the delay time for all of the taps that is output
         */
        void setOutTapDelayTimes( const std::vector < int >& outTapDelayTimesSamps )
        {
            assert( outTapDelayTimesSamps.size() == NOUTTAPS );
            for ( auto i = 0; i < NOUTTAPS; i++ )
                m_outTapDelayTimesSamps[ i ] = outTapDelayTimesSamps[ i ];
        }
        
        /**
         Set the delay time for one of the taps that is output
         */
        void setOutTapDelayTime( int outTapDelayTimesSamps, size_t tapNumber )
        {
            m_outTapDelayTimesSamps[ tapNumber ] = outTapDelayTimesSamps;
        }
        
        /**
         Set the output levels for all of the output taps from the delayline
         */
        void setOutTapGains( const std::vector < T >& outTapGains )
        {
            assert( outTapGains.size() == NOUTTAPS );
            for ( auto i = 0; i < NOUTTAPS; i++ )
                m_outTapGains[ i ] = outTapGains[ i ];
        }
        
        /**
         Set the output levels for one of the output taps from the delayline
         */
        void setOutTapGain( T outTapGain, size_t tapNumber )
        {
            m_outTapGains[ tapNumber ] = outTapGain;
        }
        
    };
}




#endif /* sjf_rev_umss_h */

