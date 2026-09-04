/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 04/09/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_ParameterFactory.h>
#include <sjf/processors/sjf_Tremolo.h>

#include <sjf/helpers/sjf_MultiCrossoverWrapper.h>
#include <sjf/helpers/Utility/sjf_GainWrapper.h>
#include <sjf/helpers/sjf_BypassWrapper.h>
#include <sjf/processors/sjf_Delay.h>

namespace sjf::dsp{

class BandProcessor
{
	using DelayLFO = sjf::dsp::oscillators::lfo::LFO<dsp::oscillators::lfo::LFOWaveformProvider	<	dsp::oscillators::lfo::Sine>,
																									dsp::oscillators::lfo::lfo_config::Smooth,
																									dsp::oscillators::lfo::lfo_config::Depth
																								>;
	using Delay_ = dsp::Delay<DelayLFO, delay_config::TimeValues<0.1f, 500.0f, 1.0f, 10.0f>, delay_config::Link, delay_config::Offset, delay_config::Feedback>;
	using Delay = sjf::helpers::BypassWrapper<Delay_, helpers::bypass_wrapper_config::Mix, helpers::bypass_wrapper_config::DefaultMixLevel<0.0f>>;

	using Trem = BasicTremolo;
public:
    struct Parameters : public helpers::AudioParametersBase
    {
        std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) override
        {
            auto factory = helpers::ParameterFactory::create (factoryID, factoryName);
            return factory;
        }
    } parameters;


    void prepare (const juce::dsp::ProcessSpec& spec_)
    {
        spec = spec_;
        parameters.prepare(spec);
    	delay.prepare(spec);
    	trem.prepare(spec);
        reset();
    }

    void reset()
    {
        parameters.reset();
    	delay.reset();
    	trem.reset();
    }

    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
        [[maybe_unused]] const auto& inputBlock = context.getInputBlock();
        [[maybe_unused]] auto& outputBlock      = context.getOutputBlock();
        [[maybe_unused]] const auto numChannels = outputBlock.getNumChannels();
        [[maybe_unused]] const auto numSamples  = outputBlock.getNumSamples();

        jassert (inputBlock.getNumChannels() == numChannels);
        jassert (inputBlock.getNumSamples() == numSamples);

        if (parameters.checkForStateChange())
        {
        	jassertfalse;
        }

    	trem.process(context);
    	delay.process(context);
    }

    std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName)
    {
        auto factory =  parameters.createParameters (factoryID, factoryName);

    	factory->addChildFactory(trem.createParameters(factoryID + "Trem", factoryName + " Tremolo"));
    	factory->addChildFactory(delay.createParameters(factoryID + "Del", factoryName + " Delay"));
    	return factory;
    }

    void setPositionInfo(const juce::AudioPlayHead::PositionInfo& positionInfo)
    {
    	delay.setPositionInfo(positionInfo);
    	trem.setPositionInfo(positionInfo);
    }

private:
	Delay delay;
	Trem trem;
    juce::dsp::ProcessSpec spec{};
};


template <size_t NumBands = 16>
using SpectralProcessor = helpers::MultiCrossoverWrapper<helpers::BypassWrapper<helpers::GainWrapper<BandProcessor, true, false>, helpers::bypass_wrapper_config::Mute>, NumBands, true, true>;

}


