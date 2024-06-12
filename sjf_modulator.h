//
//  sjf_modulator.h
//  sjf_verb
//
//  Created by Simon Fay on 06/06/2024.
//  Copyright © 2024 sjf. All rights reserved.
//

#ifndef sjf_modulator_h
#define sjf_modulator_h

#include "sjf_audioUtilitiesC++.h"
#include "sjf_filters.h"
#include "sjf_mathsApproximations.h"

namespace sjf::modulator
{
    template< typename T >
    class randMod
    {
    public:
        randMod() :  m_r((rand01()*2.0 )-1.0) {}
        
        void initialise( T initialValue ) { m_lpf.reset( initialValue ); }
        T operator()( const T val, const T phase, const T depth, const T damping )
        {
            if ( phase > 0.5 && m_lastPhase < 0.5 )
                m_r = ( ( rand01() * 2.0 ) - 1.0 );
            m_lastPhase = phase;
            return m_lpf.process( val + ( m_r * val * depth ), damping );
        }
        
    private:
        T m_r{0.0}, m_lastPhase{0.0};
        sjf::filters::damper< T > m_lpf;
    };


    template< typename T >
    class sinMod
    {
    public:
        sinMod(){}
        
        void initialise( T initialValue ) { }
        T operator()( const T val, const T phase, const T depth, const T damping )
            { return val + ( sjf::maths::sinApprox( (phase*2 - 1)*M_PI ) * val * depth ); }
    };

    /** class for modulation of  values --> outputs maximum of 0 --> 2* nominal val */
    template < typename T, typename MODFUNCTOR = randMod< T > >
    class modVoice
    {
    public:
        void initialise( T offset, T initialValue )
        {
            m_offset = offset;
            m_mod.initialise( initialValue );
        }
        
        T process( const T val, T phase, const T depth, const T damping )
        {
            phase += m_offset;
            phase = phase >= 1.0 ? phase - 1.0 : phase;
            return m_mod( val, phase, depth, damping );
        }
        
    private:
        T m_offset{0.0};
        MODFUNCTOR m_mod;
    };
}

#endif /* sjf_modulator_h */
