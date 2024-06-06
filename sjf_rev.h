//
//  sjf_apLoop.h
//
//  Created by Simon Fay on 18/03/2024.
//



#ifndef sjf_rev_h
#define sjf_rev_h


#include "sjf_audioUtilitiesC++.h"
#include "gcem/include/gcem.hpp"

#include "sjf_filters.h"
#include "sjf_delays.h"
#include "sjf_nonlinearities.h"
#include "sjf_mixers.h"


#include "sjf_reverb/sjf_revTypdefs.h"
#include "sjf_reverb/sjf_seriesAP.h"
#include "sjf_reverb/sjf_multitap.h"
#include "sjf_reverb/sjf_apLoop.h"
#include "sjf_reverb/sjf_fdn.h"
#include "sjf_reverb/sjf_umss.h"
#include "sjf_reverb/sjf_nestedAP.h"
#include "sjf_reverb/sjf_rotDelDif.h"


/**
 A collection of basic building blocks for reverb design
 */
namespace sjf::rev
{
    
    /** random modulation --> outputs maximum of 0 --> 2* nominal val */
    template < typename T >
    class dtModulatorVoice
    {
    public:
        dtModulatorVoice( ) : m_r((rand01()*2.0 )-1.0) {}
        ~dtModulatorVoice(){}
        
        void initialise( T offset, T initialValue )
        {
            m_offset = offset;
            m_lpf.reset( initialValue );
        }
        
        T process( T val, T phase, T depth, T damping )
        {
            phase += m_offset;
            phase = phase >= 1.0 ? phase - 1.0 : phase;
            if ( phase > 0.5 && m_lastPhase < 0.5 )
                m_r = ( ( rand01() * 2.0 ) - 1.0 ) ;
            m_lastPhase = phase;
            return m_lpf.process( val + ( m_r * val * depth ), damping );
        }
        
    private:
        T m_r = 0.0, m_offset, m_lastPhase = 0.0;
        sjf::filters::damper< T > m_lpf;
    };


//========================//========================//========================//========================//========================
//========================//========================//========================//========================//========================
//========================//========================//========================//========================//========================
//========================//========================//========================//========================//========================
//========================//========================//========================//========================//========================

    

}


#endif /* sjf_rev_h */



