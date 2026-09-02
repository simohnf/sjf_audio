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
#include <sjf/helpers/sjf_DCBlock.h>

namespace sjf::helpers{
/**
* @brief A DSP wrapper template that appends a DC offset blocking filter post-processing stage to an underlying processor.
*
* This class wraps a target audio processor, executing its processing chain first and then passing the resulting audio block
* through an integrated, bypassable DC blocker (`DCBlocker`) to remove any residual ultra-low frequency or DC offset transients.
*
* @tparam Processor The target audio processor class to be wrapped. Must expose standard JUCE DSP interface methods (`prepare`, `reset`, `process`, `createParameters`).
* @tparam FirstOrder Configures the filter order of the underlying DC blocker. Set to `true` for a 1st-order (6 dB/octave) high-pass filter, or `false` for higher-order blocking. Defaults to `true`.
*/
template <typename Processor, bool FirstOrder = true, bool AddOnSwitch = false>
class DCBlockerWrapper
{
public:
    struct Parameters : public helpers::AudioParametersBase
    {
    	BoolState on;
        std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String&, const juce::String&) override
        {
            if (targetFactory)
            {
                /// we insert the parameters into the processors parameter tree,
                /// BUT The DCBlockerWrapper::Parameters struct handles smoothing/reset etc the

            	if constexpr (AddOnSwitch)
            		createTrackedParameter(*targetFactory, on, "DC", "DC Block", true);

             
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
    	dcBlocker.prepare(spec);
        
        reset();
    }

    void reset()
    {
        processor.reset();
        parameters.reset();
    	dcBlocker.reset();
    }

    //==============================================================================
    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
        if constexpr (ProcessContext::usesSeparateInputAndOutputBlocks())
                context.getOutputBlock().copyFrom(context.getInputBlock());
            
        juce::dsp::ProcessContextReplacing<float> contextReplacing{context.getOutputBlock()};

    	parameters.checkForStateChange();

    	processor.process (contextReplacing);

    	if (applyDCBlocker())
			dcBlocker.process (contextReplacing);
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
	bool applyDCBlocker()
	{
		if constexpr (AddOnSwitch)
			return parameters.on.currentValue;
		else
			return true;
	}

    juce::dsp::ProcessSpec spec{};
    Processor processor;
	BypassWrapper<DCBlocker<2, FirstOrder>, bypass_wrapper_config::OnOff> dcBlocker;
};

}
