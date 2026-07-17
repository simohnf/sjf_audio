//
//  sjf_sinCos.h
//  sjf_verb
//
//  Created by Simon Fay on 15/05/2024.
//  Copyright © 2024 sjf. All rights reserved.
//

#ifndef sjf_sinCos_h
#define sjf_sinCos_h

namespace sjf::oscillators
{
    /** sin/cos oscillator as described on Keith Barr's spinsemi site --> http://www.spinsemi.com/knowledge_base/effects.html#Simple_filters */
    template < typename Sample >
    class sinCos
    {
    private:
        struct scOut{ Sample cosOut, sinOut; };
    public:
        /** Call this before first use to ensure the sample rate is set correctly */
        void initialise( Sample sampleRate ) { m_2PiOverSR = TWOPI/sampleRate; }
        
        /** call this to get the output of the dual oscillator */
        scOut operator()()
        {
            scOut output;
            output.cosOut = m_cosY1;
            output.sinOut = m_sinY1;
            m_sinY1 += m_coef * m_cosY1;
            m_cosY1 -= m_coef * m_sinY1;
            return output;
        }
        
        /** set the internal frequency of the oscillator */
        void setFrequency( Sample f ) { m_coef = f * m_2PiOverSR; }
        
        /** reset the oscillator **/
        void reset(){ m_cosY1 = 1; m_sinY1 = 0; }
        
        /** set the phase of the oscillators
         Input
            phase must be between 0-->1
         */
        void phase( Sample p ){ m_cosY1 = std::cos( TWOPI*p ); m_sinY1 = std::sin( TWOPI*p ); }
    private:
        static constexpr Sample TWOPI = 2.0*M_PI;
        Sample m_cosY1{1}, m_sinY1{0};
        Sample m_2PiOverSR{TWOPI/44100}, m_coef{440*m_2PiOverSR};
    };
}

#endif /* sjf_sinCos_h */
