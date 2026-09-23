/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 23/09/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_ParameterFactory.h>
#include <sjf/helpers/sjf_OptionalCalls.h>

#include <sjf/helpers/sjf_DCBlock.h>

namespace sjf::helpers{
template <typename Processor, bool AddDCBlock = true, size_t NumChannels = 2>
class BiasWrapper
{
public:
    struct Parameters : public helpers::AudioParametersBase
    {
    	FloatState bias;
        std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String&, const juce::String&) override
        {
            if (targetFactory)
            {
                /// we insert the parameters into the processors parameter tree,
                /// BUT The BiasWrapper::Parameters struct handles smoothing/reset etc the

            	createTrackedPercentParameter(*targetFactory, bias, "Bias", "Bias", -100.0f, 100.0f, 0.0f, 0.0f, [](const float x){ return sjf::helpers::Waveshapers::Clippers::tanh( x*0.01f);});
             
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
    	jassert(NumChannels == spec_.numChannels);
        spec = spec_;
        processor.prepare (spec);
        parameters.prepare (spec);

    	if constexpr (AddDCBlock)
    		dcBlock.prepare (spec);

    	biasValues.setSize(1, spec.maximumBlockSize);
        reset();
    }

    void reset()
    {
        processor.reset();
        parameters.reset();
    	if constexpr (AddDCBlock)
    		dcBlock.reset ();

    	biasValues.clear();
    }

    //==============================================================================
    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
        if constexpr (ProcessContext::usesSeparateInputAndOutputBlocks())
                context.getOutputBlock().copyFrom(context.getInputBlock());
            
    	juce::dsp::ProcessContextReplacing<float> contextReplacing{context.getOutputBlock()};

    	if (parameters.checkForStateChange())
    	{
    		const auto block = contextReplacing.getOutputBlock();
    		for (auto i = 0ul; i < block.getNumSamples(); i++)
    		{
    			parameters.tickSmoothers();
    			biasValues.getWritePointer(0)[i] = parameters.bias.currentValue;
    		}

    		for ( auto chan = 0ul; chan < block.getNumChannels(); chan++)
    			juce::FloatVectorOperations::add(block.getChannelPointer (chan), block.getChannelPointer (chan), biasValues.getWritePointer(0), block.getNumSamples());
    	}
    	else
    	{
    		context.getOutputBlock().add(parameters.bias.currentValue);
    	}

    	processor.process(contextReplacing);


    	if constexpr (AddDCBlock)
    		dcBlock.process (contextReplacing);
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
    juce::dsp::ProcessSpec spec{};
    Processor processor;
	[[maybe_unused]] sjf::helpers::DCBlocker<NumChannels> dcBlock;
	juce::AudioBuffer<float> biasValues;
};

}
