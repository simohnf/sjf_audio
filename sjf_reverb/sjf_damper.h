//
//  sjf_damper.h
//
//  Created by Simon Fay on 27/03/2024.
//

#ifndef sjf_rev_damper_h
#define sjf_rev_damper_h

//#include "../sjf_audioUtilitiesC++.h"
//#include "../sjf_interpolators.h"
//#include "../gcem/include/gcem.hpp"

#include "../sjf_rev.h"

namespace sjf::rev
{
    /**
     basic one pole lowpass filter, but set so that the higher the coefficient the lower the cut off frequency, this just makes it useful for setting damping in a reverb loop
     */
    template < typename  T >
    class damper
    {
    private:
        T m_lastOut = 0;
    public:
        damper(){}
        ~damper(){}
        
        /**
         this should be called every sample in the block,
         The input is:
            the sample to process
            the damping coefficient ( must be >=0 and <=1 )
         */
        T process( T x, T coef )
        {
            m_lastOut = x + coef*( m_lastOut - x );
            return m_lastOut;
        }
    };
}


#endif /* sjf_rev_damper_h */
