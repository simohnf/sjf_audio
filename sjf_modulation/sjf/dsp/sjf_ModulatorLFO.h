/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 28/09/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_ParameterFactory.h>
#include <sjf/dsp/sjf_Modulation.h>
#include <sjf/oscillators/LFO/sjf_LFO.h>

namespace sjf::dsp::modulation{
template<typename... Configurations>
class ModulatorLFO : public ModulatorBase
{
public:
	juce::String getModulatorID() const override
	{
		return id;
	}

	float getModulationSample() const noexcept override
	{
		return output;
	}

	void prepare (const juce::dsp::ProcessSpec& spec_)
    {
        lfo.prepare(spec_);
    }

    void reset()
    {
        lfo.reset();
    }

    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
		lfo.process(context);
		output = lfo.getLfoOutput().getSample(0, 0);
    }

    std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName)
    {
		id = factoryID;
		return lfo.createParameters(factoryID, factoryName);
    }

	void setPositionInfo(const juce::AudioPlayHead::PositionInfo& positionInfo)
    {
	    lfo.setPositionInfo(positionInfo);
    }

private:
	juce::String id{};
	sjf::dsp::oscillators::lfo::LFO<Configurations...> lfo;
	float output{};
};



	namespace default_modulator_lfo_config
	{
		using LFO = sjf::dsp::modulation::ModulatorLFO<oscillators::lfo::DefaultWaveformProvider,
											dsp::oscillators::lfo::lfo_config::TempoSync,
											dsp::oscillators::lfo::lfo_config::Smooth>;
	}

	using BasicLFO = default_modulator_lfo_config::LFO;
}


