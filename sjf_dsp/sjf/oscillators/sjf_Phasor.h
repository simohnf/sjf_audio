//
//  sjf_phasor.h
//
//  Created by Simon Fay on 12/05/2024.
//



#pragma once
#include <JuceHeader.h>


namespace sjf::dsp::oscillators
{

    /**
     Simple phasor class for use with modulators and other oscillators
     */
    template < typename Sample >
    struct Phasor
    {
        Sample m_increment;
        Sample m_phase = 0.0f;

        Phasor(){}

        /**
         Constructor requires frequency and samplerate values to set initial increment
         */
        Phasor( Sample frequency, Sample sampleRate ) : m_increment( frequency / sampleRate )
        { spec.sampleRate = sampleRate; }


        ~Phasor() {}

        void prepare(const  juce::dsp::ProcessSpec& spec_)
        {
            spec = spec_;
            reset();
        }

        void reset()
        {
            m_phase = 0.0f;
        }

        void setFrequency( const Sample f )
        {
            setFrequency(f, static_cast<float>(spec.sampleRate));
        }

        /**
         sets the internal frequency the phasor runs at
         */
        void setFrequency( const Sample f, const Sample sampleRate )
        {
            setIncrement( f / sampleRate);
            spec.sampleRate = sampleRate;
        }

        /**
         sets the increment per sample
         */
        void setIncrement( const Sample inc )
        {
            jassert(inc < 1.0f && inc > -1.0f);
            m_increment = inc >= 0.0f ? inc : 1.0f - inc;
        }

        /**
         Output one sample from the phasor
         */
        Sample process()
        {
            Sample p = m_phase;
            m_phase += m_increment;
            m_phase = m_phase - static_cast<Sample>(static_cast<int>(m_phase));
            return p;
        }

        void skip(size_t numSamplesToSkip)
        {
            m_phase += m_increment*static_cast<Sample>(numSamplesToSkip);
            m_phase = m_phase - static_cast<Sample>(static_cast<int>(m_phase));
        }
    private:
        juce::dsp::ProcessSpec spec{};
    };



}





