/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 24/07/2026.
//

#pragma once
#include <JuceHeader.h>

namespace sjf::dsp::oscillators
{
	/** sin/cos oscillator as described on Keith Barr's spinsemi site --> http://www.spinsemi.com/knowledge_base/effects.html#Simple_filters */
	class SinCos
	{
	private:
		struct scOut{ float cosOut, sinOut; };
	public:
		void prepare(const juce::dsp::ProcessSpec& spec_)
		{
			m_2PiOverSR = TWOPI/ static_cast<float>(spec_.sampleRate);
		}

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
		void setFrequency(const float f ) { m_coef = f * m_2PiOverSR; }

		/** reset the oscillator **/
		void reset()
		{
			m_cosY1 = 1; m_sinY1 = 0;
		}

		/** set the phase of the oscillators
		 Input
			phase must be between 0-->1
		 */
		void phase(const float p )
		{
			m_cosY1 = std::cos( TWOPI*p ); m_sinY1 = std::sin( TWOPI*p );
		}
	private:
		static constexpr float TWOPI = juce::MathConstants<float>::twoPi;
		float m_cosY1{1}, m_sinY1{0};
		float m_2PiOverSR{TWOPI/44100}, m_coef{440*m_2PiOverSR};
	};
}


//DUMMY_PLUGIN_SJF_SINCOS_H
