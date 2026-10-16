/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 15/10/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_ParameterFactory.h>

#include "sjf_Modulation.h"
#include "sjf/processors/sjf_EnvelopeFollower_juce.h"

namespace sjf::dsp::modulation
{
class ModulatorEnvelopeFollower : public ModulatorBase
{
public:
	[[nodiscard]] juce::String getModulatorID() const override
	{
		return id;
	}

	[[nodiscard]] float getModulationSample() const noexcept override
	{
		return output;
	}

	void prepare(const juce::dsp::ProcessSpec& spec_)
	{
		auto monoSpec = spec_;
		monoSpec.numChannels = 1;
		envelopeFollower.prepare(monoSpec);
		buffer.setSize(static_cast<int>(monoSpec.numChannels), static_cast<int>(monoSpec.maximumBlockSize));
		inputScale = 1.0f / static_cast<float>(std::sqrt(spec_.numChannels));
	}

	void reset()
	{
		envelopeFollower.reset();
		buffer.clear();
	}

	template <typename ProcessContext>
	void process(const ProcessContext& context) noexcept
	{
		[[maybe_unused]] const auto& inputBlock = context.getInputBlock();
		[[maybe_unused]] auto& outputBlock = context.getOutputBlock();
		[[maybe_unused]] const auto numChannels = outputBlock.getNumChannels();
		[[maybe_unused]] const auto numSamples = outputBlock.getNumSamples();

		auto block = juce::dsp::AudioBlock<float>{buffer}.getSubBlock(0, numSamples);
		for ( auto i = 0ul; i < numSamples; ++i)
			block.copyFrom(inputBlock.getSingleChannelBlock(i));

		block.multiplyBy(inputScale);

		const auto contextReplacing = juce::dsp::ProcessContextReplacing<float>{block};

		envelopeFollower.process(contextReplacing);
		output = juce::jmap(buffer.getSample(0, static_cast<int>(numSamples) - 1), 0.0f, 1.0f); // make output in range [-1, 1]

	}

	std::unique_ptr<helpers::ParameterFactory> createParameters(const juce::String& factoryID, const juce::String& factoryName)
	{
		id = factoryID;
		return envelopeFollower.createParameters(factoryID, factoryName);
	}
private:
	juce::String id{};
	sjf::dsp::EnvelopeFollower envelopeFollower;
	juce::AudioBuffer<float> buffer;
	float output{}, inputScale{1.0f};
};
}
