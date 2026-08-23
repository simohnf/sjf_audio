/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */

//
// Created by Simon Fay on 15/07/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_ParameterFactory.h>

namespace sjf::helpers
{
/**
 * @brief A high-pass filter module designed to remove low-frequency DC offset transients from an audio signal.
 *
 *
 * @tparam NumChannels Configure the number of channels
 * @tparam FIRST_ORDER Configures the filter order. Defaults to `true` (first-order / 6 dB/octave high-pass).
 */

template<size_t NumChannels = 2, bool FIRST_ORDER = true>
class DCBlocker
{
public:
	void prepare (const juce::dsp::ProcessSpec& spec_)
	{
		jassert(spec_.numChannels == NumChannels);
		spec = spec_;
		for (auto& dc : dcBlocker)
		{
			dc.prepare(spec);
			if  constexpr (FIRST_ORDER)
				dc.coefficients = juce::dsp::IIR::Coefficients<float>::makeFirstOrderHighPass(spec.sampleRate, 15.0f);
			else
				dc.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(spec.sampleRate, 15.0f);
		}
		reset();
	}

	void reset()
	{
		for (auto& dc : dcBlocker)
			dc.reset();
	}


	template <typename ProcessContext>
	void process (const ProcessContext& context) noexcept
	{
		if constexpr (ProcessContext::usesSeparateInputAndOutputBlocks())
			context.getOutputBlock().copyFrom(context.getInputBlock());
		for (auto c = 0ul; c < context.getOutputBlock().getNumChannels(); ++c)
		{
			auto& dc = dcBlocker[c];
			auto channelBlock = context.getOutputBlock().getSingleChannelBlock(c);
			auto channelContext = juce::dsp::ProcessContextReplacing<float>(channelBlock);
			dc.process(channelContext);
		}
	}

private:
	std::array<juce::dsp::IIR::Filter<float>, NumChannels> dcBlocker;
	juce::dsp::ProcessSpec spec{};
};

}