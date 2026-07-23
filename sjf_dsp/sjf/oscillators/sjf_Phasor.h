/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */

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

struct Phasor
{
    float m_increment;
    float m_phase = 0.0f;

    Phasor(){}

    void prepare(const  juce::dsp::ProcessSpec& spec_)
    {
        spec = spec_;
        reset();
    }

    void reset()
    {
        m_phase = 0.0f;
    }

    void setFrequency( const float f )
    {
        setFrequency(f, static_cast<float>(spec.sampleRate));
    }

    /**
     sets the internal frequency the phasor runs at
     */
    void setFrequency( const float f, const float sampleRate )
    {
        setIncrement( f / sampleRate);
        spec.sampleRate = sampleRate;
    }

    /**
     sets the increment per sample
     */
    void setIncrement( const float inc )
    {
        jassert(inc < 1.0f && inc > -1.0f);
        m_increment = inc >= 0.0f ? inc : 1.0f - inc;
    }

    /**
     Output one sample from the phasor
     */
    float process()
    {
        float p = m_phase;
        m_phase += m_increment;
        m_phase = m_phase - static_cast<float>(static_cast<int>(m_phase));
        return p;
    }

    void skip(size_t numSamplesToSkip)
    {
        m_phase += m_increment*static_cast<float>(numSamplesToSkip);
        m_phase = m_phase - static_cast<float>(static_cast<int>(m_phase));
    }
private:
    juce::dsp::ProcessSpec spec{};
};


template<typename SyncedRatesProvider = helpers::DefaultSyncRatesProvider>
struct TempoSyncedPhasor
{
	using FrequencyParameter = helpers::SyncedFrequencyParameter<SyncedRatesProvider>;

	explicit TempoSyncedPhasor(FrequencyParameter& frequency_)
	: frequency(frequency_)
	{}

	void prepare(const  juce::dsp::ProcessSpec& spec_)
	{
		phasor.prepare(spec_);
		reset();
	}

	void reset()
	{
		updateFrequency();
		phasor.reset();
	}


	/**
	 Output one sample from the phasor
	 */
	template<bool UpdateFrequency>
	float process()
	{
		if  constexpr (UpdateFrequency)
			updateFrequency();

		return phasor.process();
	}

	void updateFrequency()
	{
		phasor.setFrequency(frequency.frequency.currentValue);
	}

	void skip(const size_t numSamplesToSkip)
	{
		phasor.skip(numSamplesToSkip);
	}

	void setPositionInfo(const juce::AudioPlayHead::PositionInfo& positionInfo_)
	{
		frequency.setPositionInfo(positionInfo_);
		if (frequency.sync.currentValue)
		{
			auto ppq = frequency.positionInfo.getPpqPosition();
			if (ppq.hasValue() && frequency.positionInfo.getIsPlaying())
			{
				const auto barLength = static_cast<float>(frequency.syncedNumerator.currentValue * 4) / static_cast<float>(frequency.syncedDenominator.currentValue);
				auto p =  static_cast<float>(*ppq) / barLength;
				phasor.m_phase = sjf::helpers::functions::waveforms::wrapPhase(p);
			}
		}
	}

	float getPhase() const
	{
		return phasor.m_phase;
	}

private:
	FrequencyParameter& frequency;
	Phasor phasor;

};


}





