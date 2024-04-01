//
//  sjf_nestedAP.h
//
//  Created by Simon Fay on 31/03/2024.
//


#ifndef sjf_rev_nestedAP_h
#define sjf_rev_nestedAP_h


#include "../sjf_rev.h"

namespace sjf::rev
{
    /**
     One multiply all pass as per Moorer - "about this reverberation business"
     This is a very bare bones implementation that essentially just serves as a wrapper for a delayline
     */
    template < typename T, int NSTAGES = 2 >
    class nestedAP
    {
    private:
        std::array< delay< T >, NSTAGES > m_delays;
        std::array< damper< T >, NSTAGES > m_dampers;
        
        std::array< T, NSTAGES > m_delayTimesSamps, m_coefs, m_damping;
    public:
        nestedAP()
        {
            initialise( 4096 ); // initialise to something
            for ( auto s = 0; s < NSTAGES; s++ )
            {
                m_delayTimesSamps[ s ] = ( rand01()*2048 ) + 1024; // random delay times just so it's initialised
                m_coefs[ s ] = 0;
                m_damping[ s ] = 0;
            }
        }
        ~nestedAP(){}
        
        /**
         This must be called before first use in order to set basic information such as maximum delay lengths
         Size should be a power of 2!!!
         */
        void initialise( int sizeInSamps )
        {
            if (!sjf_isPowerOf( sizeInSamps, 2 ) )
                sizeInSamps = sjf_nearestPowerAbove( sizeInSamps, 2 );
            for ( auto & d : m_delays )
                d.initialise( sizeInSamps );
        }
        
        /**
         This processes a single delay, it should be called once for every sample in the block
         Input:
            sample to process
            delay time in samples for each stage ( must be < max size set during initialisation )
            coefficient for each stage ( should be >-1 and <1 )
            damping to be apllied to feedback into each stage
            interpolation type ( optional, defaults to linear, see @sjf_interpolators )
         output:
            Processed sample
         */
        T process( T x,  int interpType = DEFAULT_INTERP )
        {
            auto firstStage = 0;
            return processStageRecursive( firstStage, x, interpType );
        }
        
        /**
         This sets all of the allpass coefficients to the give value
         */
        void setCoefs( T coef )
        {
            for ( auto a = 0; a < NSTAGES; a++ )
                m_coefs[ a ] = coef;
        }
        
        /**
         Set all of the delayTimes
         */
        void setDelayTimes( const std::array< T, NSTAGES >& dt )
        {
            for ( auto d = 0; d < NSTAGES; d++ )
                m_delayTimesSamps[ d ] = dt[ d ];
        }
        
        /**
         Set all of the damping coefficients
         */
        void setDamping( const std::array< T, NSTAGES >& damp )
        {
            for ( auto d = 0; d < NSTAGES; d++ )
                m_damping[ d ] = damp[ d ];
        }
        
    private:
        /**
         This recursive call will move through each stage of the structure, passing the results of the allpass calculations to the next and then previous stages
         */
        T processStageRecursive( int stage, T x, int interpType = DEFAULT_INTERP )
        {
            // base case
            if ( stage == NSTAGES )
                return x;
//            auto delayed = m_del.getSample( delay, interpType );
//            auto xhn = ( x - delayed ) * coef;
//            auto fb = damping > 0.0 && damping < 1.0 ? m_damper.process( ( x + xhn ), damping ) : ( x + xhn );
//            m_del.setSample( fb );
//            return delayed + xhn;
            auto delayed = m_delays[ stage ].getSample( m_delayTimesSamps[ stage ], interpType );
            auto xhn = ( x - delayed ) * m_coefs[ stage ];
            auto ff = x + xhn;
            ff = m_damping[ stage ] > 0.0 && m_damping[ stage ] < 1.0 ? m_dampers[ stage ].process( ff, m_damping[ stage ] ) : ff;
            m_delays[ stage ].setSample( processStageRecursive( stage + 1, ff, interpType ) );
            return delayed + xhn;
        }
    };
}

#endif /* sjf_rev_nestedAP_h */

