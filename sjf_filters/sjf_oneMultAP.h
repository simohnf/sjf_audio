//
//  sjf_oneMultAP.h
//
//  Created by Simon Fay on 27/03/2024.
//


#ifndef sjf_oneMultAP_h
#define sjf_oneMultAP_h

#include "../sjf_delays.h"
#include "sjf_damper.h"

namespace sjf::filters
{
    /**
     One multiply all pass as per Moorer - "about this reverberation business"
     This is a very bare bones implementation that essentially just serves as a wrapper for a delayline with an optional damper which can be activated
     */
    template < typename Sample, typename INTERPOLATION_FUNCTOR = interpolation::fourPointInterpolatePD< Sample > >
    class oneMultAP
    {
    public:
        oneMultAP(){}
        ~oneMultAP(){}
        
        /**
         This must be called before first use in order to set basic information such as maximum delay lengths
         Size should be a power of 2!!!
         */
        void initialise( long sizeInSamps )
        {
            if (!sjf_isPowerOf( sizeInSamps, 2 ) )
                sizeInSamps = sjf_nearestPowerAbove( sizeInSamps, 2l );
            m_del.initialise( sizeInSamps );
        }
        
        /**
         This processes a single delay (with damping), it should be called once for every sample in the block
         Input:
            sample to process
            delay time in samples ( must be < max size set during initialisation )
            coefficient ( should be >-1 and <1 )
            damping coefficient ( 0<= coef < 1 )
         output:
            Processed sample
         */
        Sample process( Sample x, Sample delay, Sample coef, Sample damping )
        {
            assert ( coef > -1 && coef < 1 );
            auto delayed = m_del.getSample( delay );
            auto xhn = ( x - delayed ) * coef;
            m_del.setSample( m_damper.process( ( x + xhn ), damping ) );
            return delayed + xhn;
        }
        
        /**
         This processes a single delay (without any damping), it should be called once for every sample in the block
         Input:
            sample to process
            delay time in samples ( must be < max size set during initialisation )
            coefficient ( should be >-1 and <1 )
         output:
            Processed sample
         */
        Sample process( Sample x, Sample delay, Sample coef )
        {
            assert ( coef > -1 && coef < 1 );
            auto delayed = m_del.getSample( delay );
            auto xhn = ( x - delayed ) * coef;
            m_del.setSample( x + xhn );
            return delayed + xhn;
        }
        
    private:
        delayLine::delay< Sample, INTERPOLATION_FUNCTOR > m_del;
        filters::damper< Sample > m_damper;
        
    };
}

#endif /* sjf_rev_oneMultAP_h */
