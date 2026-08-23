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

namespace sjf::helpers{
/**
* @brief A pre-processing channel matrix wrapper template that reconfigures stereo audio channels prior to reaching an underlying processor.
*
* This wrapper transforms input stereo channels before processing, offering selectable channel routing modes including dual-left,
* dual-right, channel swapping, and equal-power mono summing ($L + R \times \frac{1}{\sqrt{2}}$). Alternatively, it can operate as a lightweight
* toggle wrapper providing a dedicated mono summing switch.
*
* @tparam Processor The target audio processor class to be wrapped. Must expose standard JUCE DSP interface methods (`prepare`, `reset`, `process`, `createParameters`).
* @tparam OnlyMonoSwitch When `true`, exposes a single boolean mono switch instead of the full multi-choice channel matrix ("Stereo", "Left", "Right", "Swap", "Mono"). Defaults to `false`.
*/
template <typename Processor, bool OnlyMonoSwitch = false>
class InputMatrixWrapper
{
public:
    struct Parameters : public helpers::AudioParametersBase
    {
    	ChoiceState matrix;
    	BoolState mono;

        std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String&, const juce::String&) override
        {
            if (targetFactory)
            {
                /// we insert the parameters into the processors parameter tree,
                /// BUT The InputMatrix::Parameters struct handles smoothing/reset etc the

            	if constexpr (OnlyMonoSwitch)
            		createTrackedParameter(*targetFactory, mono, "Mono", "Mono", false );
            	else
					createTrackedParameter(*targetFactory, matrix, "Mode", "Mode", {"Stereo", "Left", "Right", "Swap", "Mono"}, 0 );
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
        spec = spec_;
        processor.prepare (spec);
        parameters.prepare (spec);
        
        reset();
    }

    void reset()
    {
        processor.reset();
        parameters.reset();
    }

    //==============================================================================
    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
        if constexpr (ProcessContext::usesSeparateInputAndOutputBlocks())
                context.getOutputBlock().copyFrom(context.getInputBlock());


        juce::dsp::ProcessContextReplacing<float> contextReplacing{context.getOutputBlock()};
    	auto block = contextReplacing.getOutputBlock();

    	parameters.checkForStateChange();

    	if constexpr(OnlyMonoSwitch)
    	{
    		if (parameters.mono.currentValue)
    		{
    			convertToMono(block);
    		}
    	}
    	else // Matrix
    	{
    		if (parameters.matrix.currentValue == 1) // Left
    		{
    			block.getSingleChannelBlock(1).copyFrom(block.getSingleChannelBlock(0));
    		}
    		else if (parameters.matrix.currentValue == 2) // Right
    		{
    			block.getSingleChannelBlock(0).copyFrom(block.getSingleChannelBlock(1));
    		}
    		else if (parameters.matrix.currentValue == 3) // Swap
    		{
    			auto r = block.getSingleChannelBlock(1);
    			block.getSingleChannelBlock(0).swap(r);
    		}
    		else if (parameters.matrix.currentValue == 4) // Mono
    		{
    			convertToMono(block);
    		}
    	}

    	processor.process (contextReplacing);
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
	forcedinline static void convertToMono(const juce::dsp::AudioBlock<float>& block)
	{
		constexpr auto sqrtPoint5 = 1.0f / juce::MathConstants<float>::sqrt2;
		block.getSingleChannelBlock(0).add(block.getSingleChannelBlock(1));
		block.getSingleChannelBlock(0).multiplyBy(sqrtPoint5);
		block.getSingleChannelBlock(1).copyFrom(block.getSingleChannelBlock(0));
	}

    juce::dsp::ProcessSpec spec{};
    Processor processor;
};

}
