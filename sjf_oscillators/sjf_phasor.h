//
//  sjf_phasor.h
//
//  Created by Simon Fay on 12/05/2024.
//



#ifndef sjf_phasor_h
#define sjf_phasor_h


namespace sjf::oscillators
{

/**
 Simple phasor class for use with modulators and other oscillators
 */
    template < typename Sample >
    class phasor{
        
        Sample m_increment;
        Sample m_phase = 0.0f;
    public:
        /**
         Constructor requires frequency and samplerate values to set initial increment
         */
        phasor( Sample frequency, Sample sampleRate ) : m_increment( frequency / sampleRate ) { };
        ~phasor() {};
        
        /**
         sets the internal frequency the phasor runs at
         */
        void setFrequency( const Sample f, const Sample sampleRate ) { m_increment = f / sampleRate; }
        
        /**
         sets the increment per sample
         */
        void setIncrement( const Sample inc ) { m_increment = inc; }
        
        /**
         Output one sample from the phasor
         */
        Sample process()
        {
            Sample p = m_phase;
            m_phase += m_increment;
            m_phase = (m_phase >= 1) ? m_phase - 1.0f : ( (m_phase < 0.0f) ? m_phase + 1.0f : m_phase);
            return p;
        }
    };

    

}


#endif /* sjf_phasor_h */



