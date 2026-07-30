/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 30/07/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_ParameterFactory.h>

namespace sjf::helpers
{

template<int Minimum_dB = -100, int Maximum_dB = 12, int Default_dB = 0, int SkewForCentre_dB = 0>
class Gain
{
public:
	struct Parameters : public helpers::AudioParametersBase
	{
		FloatState  gain;

		std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) override
		{
			static_assert(Minimum_dB < Maximum_dB, "Minimum_dB must be less than Maximum_dB");
			auto factory = helpers::ParameterFactory::create (factoryID, factoryName);
			auto range = NormalisableRange<float>{Minimum_dB, Maximum_dB, 0.01f};
			const auto skew = SkewForCentre_dB > Minimum_dB && SkewForCentre_dB < Maximum_dB ? SkewForCentre_dB : jmap<float>(0.5f, Minimum_dB, Maximum_dB);
			range.setSkewForCentre(skew);
			const auto default_dB = Default_dB > Minimum_dB && Default_dB < Maximum_dB ? Default_dB : skew;
			auto attributes = AudioParameterFloatAttributes{}.withLabel("dB");
			createTrackedParameter  (*factory, gain, "Gain",    "Gain", range, default_dB,
													[](const float dB) { return juce::Decibels::decibelsToGain (dB); }, attributes);

			return factory;
		}
	} parameters;


	void prepare (const juce::dsp::ProcessSpec& spec_)
	{
		spec = spec_;
		parameters.prepare(spec);
		reset();
	}

	void reset()
	{
		parameters.reset();
	}

	template <typename ProcessContext>
	void process (const ProcessContext& context) noexcept
	{
		const auto& inputBlock = context.getInputBlock();
		auto& outputBlock      = context.getOutputBlock();
		const auto numChannels = outputBlock.getNumChannels();
		const auto numSamples  = outputBlock.getNumSamples();

		jassert (inputBlock.getNumChannels() == numChannels);
		jassert (inputBlock.getNumSamples() == numSamples);

		if (parameters.checkForStateChange())
		{
			processSmoothedState(context);
		}
		else
		{
			if (ProcessContext::usesSeparateInputAndOutputBlocks())
			{
				context.getOutputBlock().copyFrom(context.getInputBlock());
			}

			context.getOutputBlock().multiplyBy(parameters.gain.currentValue);
		}
	}

	std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName)
	{
		return parameters.createParameters (factoryID, factoryName);
	}

private:
	template <typename ProcessContext>
	void processSmoothedState (const ProcessContext& context) noexcept
	{
		const auto& inputBlock = context.getInputBlock();
		auto& outputBlock      = context.getOutputBlock();
		const auto numChannels = outputBlock.getNumChannels();
		const auto numSamples  = outputBlock.getNumSamples();

		jassert (inputBlock.getNumChannels() == numChannels);
		jassert (inputBlock.getNumSamples() == numSamples);

		for (auto i = 0ul; i < numSamples; ++i)
		{
			parameters.tickSmoothers();
			const auto g = parameters.gain.currentValue;
			for ( auto c = 0ul; c < numChannels; ++c)
			{
				outputBlock.getChannelPointer(c)[i] = inputBlock.getChannelPointer(c)[i] * g;
			}
		}
	}

	juce::dsp::ProcessSpec spec{};
};

}


//DUMMY_PLUGIN_SJF_GAIN_H
