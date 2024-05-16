//
//  sjf_apLoop.h
//
//  Created by Simon Fay on 12/05/2024.
//



#ifndef sjf_phasor_h
#define sjf_phasor_h


namespace sjf::oscillators
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
        void setFrequency( const T f, T sampleRate ) { m_increment = f / sampleRate; }
        
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

    

}


#endif /* sjf_phasor_h */



