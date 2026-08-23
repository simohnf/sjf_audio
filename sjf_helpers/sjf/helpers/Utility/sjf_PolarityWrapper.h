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
#include <sjf/helpers/sjf_ParameterFactory.h>
#include <sjf/helpers/sjf_OptionalCalls.h>

#include "sjf/processors/sjf_Utility.h"

namespace sjf::helpers
{
/**
 * @brief A stereo polarity inversion wrapper template providing optional per-channel input and output phase flips.
 *
 * This wrapper adds per-channel polarity inversion switches ($\varnothing$) to pre- and/or post-processing stages of a target audio processor.
 * Polarity transitions are smoothed between positive ($+1.0$) and inverted ($-1.0$) states using linear interpolation (`juce::LinearSmoothedValue`)
 * to prevent clicks and pop artifacts during real-time parameter changes.
 *
 * @tparam Processor The target audio processor class to be wrapped. Must expose standard JUCE DSP interface methods (`prepare`, `reset`, `process`, `createParameters`).
 * @tparam InputPolarity Enables independent input channel polarity inversion toggles (Input Left $\varnothing$, Input Right $\varnothing$) when `true`.
 * @tparam OutputPolarity Enables independent output channel polarity inversion toggles (Output Left $\varnothing$, Output Right $\varnothing$) when `true`.
 */
template <typename Processor, bool InputPolarity, bool OutputPolarity>
class PolarityWrapper
{
	static constexpr size_t NumChannels = 2;
public:
    struct Parameters : public helpers::AudioParametersBase
    {
    	[[maybe_unused]] std::array<BoolState, NumChannels> input, output;
        std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String&, const juce::String&) override
        {
            if (targetFactory)
            {
                /// we insert the parameters into the processors parameter tree,
                /// BUT The PolarityWrapper::Parameters struct handles smoothing/reset etc the
                const auto Ø = juce::String(juce::CharPointer_UTF8 ("\xc3\x98"));
            	if constexpr (InputPolarity)
            	{

            		createTrackedParameter(*targetFactory, input[0], "InL" + Ø, "Input Left " + Ø, false);
            		createTrackedParameter(*targetFactory, input[1], "InR" + Ø, "Input Right " + Ø, false);
            	}

            	if constexpr (OutputPolarity)
            	{
            		createTrackedParameter(*targetFactory, output[0], "OutL" + Ø, "Output Left " + Ø, false);
            		createTrackedParameter(*targetFactory, output[1], "OutR" + Ø, "Output Right " + Ø, false);
            	}
             
            }
            else
            {
                jassertfalse;
            }

            targetFactory = nullptr;

            return std::unique_ptr<helpers::ParameterFactory>{nullptr};
        }

        void setParameterFactory(ParameterFactory* factoryToUse)
        {
            targetFactory = factoryToUse;
        }

    private:
        ParameterFactory* targetFactory{nullptr};
    } parameters;

    //==============================================================================
    void prepare (const juce::dsp::ProcessSpec& spec_)
    {
    	jassert(spec_.numChannels == NumChannels);

        spec = spec_;
        processor.prepare (spec);
        parameters.prepare (spec);

    	if constexpr (InputPolarity)
    	{
    		for (auto & in_: inSmoother)
    			in_.reset(spec.sampleRate, 0.05f);
    	}

    	if constexpr (OutputPolarity)
    	{
    		for (auto & out_: outSmoother)
    			out_.reset(spec.sampleRate, 0.05f);
    	}

        reset();
    }

    void reset()
    {
        processor.reset();
        parameters.reset();

    	if constexpr (InputPolarity)
    	{
    		for (auto i =0ul; i <NumChannels; ++i)
				inSmoother[i].setCurrentAndTargetValue(parameters.input[i].currentValue ? -1.0f : 1.0f);
    	}

    	if constexpr (OutputPolarity)
    	{
    		for (auto i =0ul; i <NumChannels; ++i)
    			outSmoother[i].setCurrentAndTargetValue(parameters.output[i].currentValue ? -1.0f : 1.0f);
    	}

    }

    //==============================================================================
    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
    	if constexpr (ProcessContext::usesSeparateInputAndOutputBlocks())
    		context.getOutputBlock().copyFrom(context.getInputBlock());

    	juce::dsp::ProcessContextReplacing<float> contextReplacing{context.getOutputBlock()};

    	if (!smoothersActive())
    	{
    		if (parameters.checkForStateChange())
    		{
    			parameters.reset(); // we handle smoothing manually
    			if constexpr (InputPolarity)
    			{
    				for (auto i =0ul; i <NumChannels; ++i)
    					inSmoother[i].setTargetValue(parameters.input[i].currentValue ? -1.0f : 1.0f);
    			}

    			if constexpr (OutputPolarity)
    			{
    				for (auto i =0ul; i <NumChannels; ++i)
    					outSmoother[i].setTargetValue(parameters.output[i].currentValue ? -1.0f : 1.0f);
    			}
    		}
    	}

    	if constexpr (InputPolarity)
    	{
    		for (auto i = 0ul; i <NumChannels; ++i)
    			contextReplacing.getOutputBlock().getSingleChannelBlock(i).multiplyBy(inSmoother[i]);
    	}

    	processor.process (contextReplacing);

    	if constexpr (OutputPolarity)
    	{
    		for (auto i = 0ul; i <NumChannels; ++i)
    			contextReplacing.getOutputBlock().getSingleChannelBlock(i).multiplyBy(outSmoother[i]);
    	}
    }

    //==============================================================================
    /**
        Generates the internal processor's parameter factory, hands it to our local
        parameters object to append the bypass state, and cleans up the transient pointer.
    */
    template <typename... Args>
    std::unique_ptr<helpers::ParameterFactory> createParameters (
        const juce::String& factoryID,
        const juce::String& factoryName,
        Args&&... configArgs)
    {
        // 1. Ask the wrapped processor to generate its parameter factory layout first
        auto factory = processor.createParameters (factoryID, factoryName, std::forward<Args> (configArgs)...);

        if (factory == nullptr)
        {
            jassertfalse;
            return nullptr;
        }

        parameters.setParameterFactory (factory.get());

        parameters.createParameters (factoryID, factoryName);


        return factory;
    }

    //==============================================================================
    [[nodiscard]] Processor& getProcessor() noexcept { return processor; }
    [[nodiscard]] const Processor& getProcessor() const noexcept { return processor; }

    void setPositionInfo(const juce::AudioPlayHead::PositionInfo& positionInfo)
    {
        sjf::optional_calls::setPositionInfo(processor, positionInfo);
    }

	int getLatencySamples()
    {
    	return sjf::optional_calls::getLatencySamples(processor);
    }

	void attachToState (juce::ValueTree& parentTree)
    {
    	sjf::optional_calls::attachToState(processor, parentTree);
    }
private:
	[[nodiscard]] bool smoothersActive() const
	{
		if constexpr (InputPolarity)
			for (auto& in_ : inSmoother)
				if (in_.isSmoothing())
					return true;
		if constexpr (OutputPolarity)
			for (auto& out_ : outSmoother)
				if (out_.isSmoothing())
					return true;
		return false;
	}


    juce::dsp::ProcessSpec spec{};
    Processor processor;
	[[maybe_unused]] std::array<juce::LinearSmoothedValue<float>, NumChannels> inSmoother, outSmoother;
};

}
