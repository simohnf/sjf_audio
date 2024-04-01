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

namespace sjf::rev
{
    /**
     umss a basic implementation of Christopher Moore’s ursa major space station style multitap delay/reverb effect
     */
    template< typename T, int NFBLOOPS, int NOUTTAPS >
    class umss
    {
    private:
        delay< T > m_delayLine;
        damper< T > m_damper;
        std::array < T, NFBLOOPS > m_fbDelayTimesSamps;
        std::array < int, NOUTTAPS > m_outTapDelayTimesSamps;
        std::array < T, NOUTTAPS > m_outTapGains;
        T m_feedback = 0.5, m_SR = 44100, m_damping = 0.4;
        
    public:
        umss()
        {
            initialise( 44100, 44100 );
            for ( auto f = 0; f < NFBLOOPS; f++ )
                m_fbDelayTimesSamps[ f ] = ( rand01()*22050.0 ) + 4410;
            auto oLevel = 1.0 / NOUTTAPS;
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
            m_delayLine.initialise( sjf_nearestPowerAbove( maxDelayInSamps, 2 ) );
        }
        
        /**
        This should be called once for every sample in the block
         input is:
            the sample to be processed
            interpolation type ( optional, defaults to linear,  see @sjf_interpolators )
         output is the processed sample
         */
        T process( T x, int interpType = DEFAULT_INTERP )
        {
            T fbLoop = 0.0f, outTaps = 0.0f;
            // we should add modulation to fb delayTaps
            for ( auto i = 0; i < NFBLOOPS; i++ )
                fbLoop += m_delayLine.getSample( m_fbDelayTimesSamps[ i ], interpType );
            // no modulation for output taps
            for ( auto i = 0; i < NOUTTAPS; i++ )
                outTaps += m_delayLine.getSample( m_outTapDelayTimesSamps[ i ], 0 ) * m_outTapGains[ i ];
            x = m_damper.process( x + ( fbLoop * m_feedback ), m_damping );
            m_delayLine.setSample( x );
            return outTaps;
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
        
        
        /**
         Set the delay time for all of the taps that feedback into the delay loop
         */
        void setFeedbackDelayTimes( const std::array < T, NFBLOOPS > fbDelayTimesSamps )
        {
            m_fbDelayTimesSamps = fbDelayTimesSamps;
        }
            
        
        /**
         Set the delay time for all of the taps that feedback into the delay loop
         */
        void setOutTapDelayTimes( const std::array < int, NOUTTAPS > outTapDelayTimesSamps )
        {
            m_outTapDelayTimesSamps = outTapDelayTimesSamps;
        }
        
        /**
         Set the output levels for all of the output taps from the delayline
         */
        void setOutTapGains( const std::array < T, NOUTTAPS > outTapGains )
        {
            m_outTapGains = outTapGains;
        }
        
        

    };
}




#endif /* sjf_rev_umss_h */

