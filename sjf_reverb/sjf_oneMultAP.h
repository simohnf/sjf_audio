#ifndef sjf_rev_oneMultAP_h
#define sjf_rev_oneMultAP_h

#include "../sjf_audioUtilitiesC++.h"
#include "../sjf_interpolators.h"
#include "../gcem/include/gcem.hpp"



namespace sjf::rev
{
    template < typename  T >
    class oneMultAP
    {
    private:
        delay< T > m_del;
    public:
        oneMultAP(){}
        ~oneMultAP(){}
        
        void initialise( int sizeInSamps_pow2 )
        {
            m_del.initialise( sizeInSamps_pow2 );
        }
        
        T process( T x, T delay, T coef )
        {
            auto delayed = m_del.getSample( delay );
            auto xhn = ( x - delayed ) * coef;
            m_del.setSample( x + xhn );
            return delayed + xhn;
        }
    };
}

#endif /* sjf_rev_oneMultAP_h */
