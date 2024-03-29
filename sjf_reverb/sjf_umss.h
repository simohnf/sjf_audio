//
//  sjf_umss.h
//
//  Created by Simon Fay on 27/03/2024.
//


#ifndef sjf_rev_umss_h
#define sjf_rev_umss_h

#include "../sjf_audioUtilitiesC++.h"
#include "../sjf_interpolators.h"
#include "../gcem/include/gcem.hpp"
#include "sjf_delay.h"
#include "sjf_damper.h"

namespace sjf::rev
{
    /**
     umss a rough implementation of Christopher Moore’s ursa major space station style multitap delay/reverb effect
     */
    template< typename T, int NFBLOOPS, int NOUTTAPS >
    class umss
    {
    public:
        umss(){}
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
            interpolation type see @sjf_interpolators
         output is the processed sample
         */
        T process( T x, int interpType = 1 )
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
        
    private:
        delay< T > m_delayLine;
        damper< T > m_damper;
        std::array < T, NFBLOOPS > m_fbDelayTimesSamps;
        std::array < T, NOUTTAPS > m_outTapDelayTimesSamps, m_outTapGains;
        T m_feedback = 0.5, m_SR = 44100, m_damping = 0.4;
    };
}




#endif /* sjf_rev_umss_h */

