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

#include <sjf/helpers/sjf_BypassWrapper.h>
#include <sjf/processors/sjf_Filter_juce.h>
#include <sjf/helpers/Utility/sjf_MidSide.h>

namespace sjf::helpers{
template <typename Processor, bool StereoSpread, bool MonoBass>
class MidSideWrapper
{
	static constexpr size_t NumChannels = 2;
public:
	static_assert(StereoSpread || MonoBass, "You must choose at least one processing method");

    struct Parameters : public helpers::AudioParametersBase
    {
    	[[maybe_unused]] FloatState spread;

    	AudioProcessorParameterGroup dummy;
        std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String&, const juce::String&) override
        {
            if (targetFactory)
            {
                /// we insert the parameters into the processors parameter tree,
                /// BUT The MidSideWrapper::Parameters struct handles smoothing/reset etc the
                if constexpr (StereoSpread)
                {
                	const auto mapping = [](const float x){ return x*0.01f*0.5f;};
                	const auto att = juce::AudioParameterFloatAttributes{}.withLabel("%");
	                createTrackedParameter(*targetFactory, spread, "Spread", "Stereo Spread", {0.0f, 200.0f, 0.01f}, 100.0f, mapping, att);
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

    	if constexpr (StereoSpread)
			for ( auto& smoother : msSmoother)
				smoother.reset(spec.sampleRate, 0.05f);

    	if constexpr (MonoBass)
    	{
    		auto monoSpec = spec;
    		monoSpec.numChannels = 1;
    		monoBass.prepare(monoSpec);
    	}

        reset();
    }

    void reset()
    {
        processor.reset();
        parameters.reset();
    	if constexpr (StereoSpread)
    	{
    		msSmoother[0].setCurrentAndTargetValue(std::sqrt(1.0f - parameters.spread.currentValue));
    		msSmoother[1].setCurrentAndTargetValue(std::sqrt(parameters.spread.currentValue));
    	}

    	if constexpr (MonoBass)
    		monoBass.reset();
    }

    //==============================================================================
    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
    	if (!smoothersActive())
	    {
		    if (parameters.checkForStateChange())
		    {
		    	parameters.reset();
		    	if constexpr (StereoSpread)
		    	{
		    		msSmoother[0].setTargetValue(std::sqrt(1.0f - parameters.spread.currentValue));
		    		msSmoother[1].setTargetValue(std::sqrt(parameters.spread.currentValue));
		    	}
		    }
	    }

    	if constexpr (ProcessContext::usesSeparateInputAndOutputBlocks())
    		context.getOutputBlock().copyFrom(context.getInputBlock());

    	juce::dsp::ProcessContextReplacing<float> contextReplacing{context.getOutputBlock()};

    	processor.process (contextReplacing);

    	auto block0 = contextReplacing.getOutputBlock().getSingleChannelBlock(0);
    	auto block1 = contextReplacing.getOutputBlock().getSingleChannelBlock(1);

    	sjf::helpers::MidSide::encode(block0, block1);

    	if constexpr (MonoBass)
    	{
    		const auto sideContext = juce::dsp::ProcessContextReplacing<float>{block1};
    		monoBass.process(sideContext);
    	}

    	if constexpr (StereoSpread)
    	{
    		for (auto i = 0ul; i < NumChannels; i++)
    			contextReplacing.getOutputBlock().getSingleChannelBlock(i).multiplyBy(msSmoother[i]);
    	}

    	sjf::helpers::MidSide::decode(block0, block1);
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

    	if constexpr (MonoBass)
    		factory->addChildFactory(monoBass.createParameters(factoryID+"MB", factoryName+" Mono Bass"));

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
		if constexpr (StereoSpread)
			return msSmoother[0].isSmoothing() || msSmoother[1].isSmoothing();
		return false;
	}

    juce::dsp::ProcessSpec spec{};
    Processor processor;
	[[maybe_unused]] std::array<juce::LinearSmoothedValue<float>, NumChannels> msSmoother;
	using MBF = sjf::dsp::SVF<dsp::FixedFilterType::HighPass, true, dsp::filter_config::FrequencyRange<20.0f, 500.0f, 120.0f, 120.0f>, false>;
	[[maybe_unused]] BypassWrapper<MBF, bypass_wrapper_config::OnOff> monoBass;
};

}
