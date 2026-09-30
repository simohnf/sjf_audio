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
/**
 * @brief Bridge adapter wrapping a compile-time configured LFO for the modulation system.
 *
 * `ModulatorLFO` adapts `sjf::dsp::oscillators::lfo::LFO<Configurations...>` to inherit from
 * `ModulatorBase`, enabling variadic LFO instances to function directly within `ModulatorChain`
 * and `ModulationSystem`.
 *
 * The class delegates internal parameter creation, state tracking, host transport position syncing,
 * and DSP calculation to the underlying template LFO instance while exposing lock-free sample reads
 * via `getModulationSample()`.
 *
 * ### Example Usage:
 * @code
 * // Wrap an LFO configured with tempo sync capabilities
 * using MyModulatorLFO = sjf::dsp::modulation::ModulatorLFO<
 *     sjf::dsp::oscillators::lfo::DefaultWaveformProvider,
 *     sjf::dsp::oscillators::lfo::lfo_config::TempoSync
 * >;
 *
 * MyModulatorLFO modulatorLfo;
 * @endcode
 *
 * @tparam Configurations Variadic type pack consisting of an `LFOWaveformProvider` and `lfo_config` feature tags.
 * @see ModulatorBase, ModulatorChain, ModulationSystem
 */
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
										dsp::oscillators::lfo::lfo_config::TempoSync>;
}

using BasicLFO = default_modulator_lfo_config::LFO;
}


