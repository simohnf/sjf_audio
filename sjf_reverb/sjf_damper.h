#ifndef sjf_rev_damper_h
#define sjf_rev_damper_h

#include "../sjf_audioUtilitiesC++.h"
#include "../sjf_interpolators.h"
#include "../gcem/include/gcem.hpp"

namespace sjf::rev
{
    template < typename  T >
    class damper
    {
    private:
        T m_lastOut = 0;
    public:
        damper(){}
        ~damper(){}
        
        T process( T x, T coef )
        {
            m_lastOut = x + coef*( m_lastOut - x );
            return m_lastOut;
        }
    };
}


#endif /* sjf_rev_damper_h */
