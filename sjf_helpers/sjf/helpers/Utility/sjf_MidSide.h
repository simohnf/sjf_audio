/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 15/08/2026.
//

#pragma once
#include <JuceHeader.h>

namespace sjf::helpers
{
struct MidSide {
	template <typename ProcessContext>
	forcedinline static void encode(const ProcessContext& context)
	{
		if constexpr (ProcessContext::usesSeparateInputAndOutputBlocks())
			context.getOutputBlock().copyFrom(context.getInputBlock());

		auto leftBlock = context.getOutputBlock().getSingleChannelBlock(0);
		auto rightBlock = context.getOutputBlock().getSingleChannelBlock(1);

		encode(leftBlock, rightBlock);
	}


	forcedinline static void encode(juce::dsp::AudioBlock<float>& leftBlock, juce::dsp::AudioBlock<float>& rightBlock)
	{
		leftBlock.add(rightBlock);
		rightBlock.multiplyBy(-2.0f);
		rightBlock.add(leftBlock);
	}

	forcedinline static std::pair<float, float> encode(float left, float right)
	{
		left += right;
		right *= -2.0f;
		right += left;
		return {left, right};
	}

	template<typename ProcessContext>
	forcedinline static void decode(const ProcessContext& context)
	{
		if constexpr (ProcessContext::usesSeparateInputAndOutputBlocks())
			context.getOutputBlock().copyFrom(context.getInputBlock());

		auto midBlock = context.getOutputBlock().getSingleChannelBlock(0);
		auto sideBlock = context.getOutputBlock().getSingleChannelBlock(1);

		decode(midBlock, sideBlock);
	}

	forcedinline static void decode(juce::dsp::AudioBlock<float>& midBlock, juce::dsp::AudioBlock<float>& sideBlock)
	{
		midBlock.add(sideBlock);
		midBlock.multiplyBy(0.5f);
		sideBlock.multiplyBy(-1.0f);
		sideBlock.add(midBlock);
	}

	forcedinline static std::pair<float, float> decode(float mid, float side)
	{
		mid += side;
		mid *= 0.5f;
		side = mid - side;
		return {mid, side};
	}
};
}