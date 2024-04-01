//
//  sjf_oneMultAP.h
//
//  Created by Simon Fay on 27/03/2024.
//


#ifndef sjf_rev_oneMultAP_h
#define sjf_rev_oneMultAP_h

//#include "../sjf_audioUtilitiesC++.h"
//#include "../sjf_interpolators.h"
//#include "../gcem/include/gcem.hpp"
//#include "sjf_delay.h"
//
//#include "sjf_rev_consts.h"

#include "../sjf_rev.h"

namespace sjf::rev
{
    /**
     One multiply all pass as per Moorer - "about this reverberation business"
     This is a very bare bones implementation that essentially just serves as a wrapper for a delayline with an optional damper which can be activated
     */
    template < typename  T >
    class oneMultAP
    {
    private:
        delay< T > m_del;
        damper< T > m_damper;
        
    public:
        oneMultAP(){}
        ~oneMultAP(){}
        
        /**
         This must be called before first use in order to set basic information such as maximum delay lengths
         Size should be a power of 2!!!
         */
        void initialise( int sizeInSamps )
        {
            if (!sjf_isPowerOf( sizeInSamps, 2 ) )
                sizeInSamps = sjf_nearestPowerAbove( sizeInSamps, 2 );
            m_del.initialise( sizeInSamps );
        }
        
        /**
         This processes a single delay, it should be called once for every sample in the block
         Input:
            sample to process
            delay time in samples ( must be < max size set during initialisation )
            coefficient ( should be >-1 and <1 )
            damping ( optional, defaults to 0 i.e. no damping, must be >=0 < 1 )
            interpolation type ( optional, defaults to linear, see @sjf_interpolators )
         output:
            Processed sample
         */
        T process( T x, T delay, T coef, int interpType = DEFAULT_INTERP, T damping = 0.0 )
        {
            auto delayed = m_del.getSample( delay, interpType );
            auto xhn = ( x - delayed ) * coef;
            auto fb = damping > 0.0 && damping < 1.0 ? m_damper.process( ( x + xhn ), damping ) : ( x + xhn );
            m_del.setSample( fb );
            return delayed + xhn;
        }
        
    };
}

#endif /* sjf_rev_oneMultAP_h */