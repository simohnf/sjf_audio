//
//  sjf_apLoop.h
//
//  Created by Simon Fay on 18/03/2024.
//



#ifndef sjf_rev_h
#define sjf_rev_h

#include "sjf_audioUtilitiesC++.h"
#include "sjf_interpolators.h"
#include "gcem/include/gcem.hpp"

#include "sjf_reverb/sjf_rev_consts.h"
#include "sjf_reverb/sjf_damper.h"
#include "sjf_reverb/sjf_delay.h"
#include "sjf_reverb/sjf_oneMultAP.h"
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

/**
 Simple phasor class for use with modulatorss
 */
template < typename T >
    class phasor{
        
        T m_increment;
        T m_phase = 0.0f;
    public:
        /**
         Constructor requires frequency and samplerate values to set initial increment
         */
        phasor( T frequency, T sampleRate ) : m_increment( frequency / sampleRate ) { };
        ~phasor() {};
        
        /**
         sets the internal frequency the phasor runs at
         */
        void setFrequency( const T f, T sampleRate )
        {
            m_increment = f / sampleRate;
        }
        
        /**
         Output one sample from the phasor
         */
        T process()
        {
            T p = m_phase;
            m_phase += m_increment;
            m_phase = (m_phase >= 1) ? m_phase - 1.0f : ( (m_phase < 0.0f) ? m_phase + 1.0f : m_phase);
            return p;
        }
    };

//========================//========================//========================//========================//========================
//========================//========================//========================//========================//========================
//========================//========================//========================//========================//========================
//========================//========================//========================//========================//========================
//========================//========================//========================//========================//========================
template < typename T >
    class dtModulatorVoice
    {
    public:
        dtModulatorVoice( )
        {
            m_r = ( rand01() * 2.0 ) - 1.0;
        }
        ~dtModulatorVoice(){}
        
        void initialise( T offset, T initialValue )
        {
            m_offset = offset;
            m_lpf.reset( initialValue );
        }
        
        T process( T dt, T phase, T depth, T damping )
        {
            phase += m_offset;
            phase = phase >= 1.0 ? phase - 1.0 : phase;
            if ( phase > 0.5 && m_lastPhase < 0.5 )
                m_r = ( ( rand01() * 2.0 ) - 1.0 ) ;
            m_lastPhase = phase;
            return m_lpf.process( dt + ( m_r * dt * depth ), damping );
        }
        
    private:
        T m_r = 0.0, m_offset, m_lastPhase = 0.0;
        sjf::rev::damper< T > m_lpf;
    };


//========================//========================//========================//========================//========================
//========================//========================//========================//========================//========================
//========================//========================//========================//========================//========================
//========================//========================//========================//========================//========================
//========================//========================//========================//========================//========================

    

}


#endif /* sjf_rev_h */



