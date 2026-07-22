/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */

//
// Created by Simon Fay on 14/07/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_ParameterFactory.h>
#include <sjf/helpers/sjf_ChunkedWrapper.h>
#include "sjf_OptionalCalls.h"

namespace sjf::helpers
{
    /**
     * @brief A compile-time wrapper that adds dynamic, runtime-configurable oversampling
     *        to any compatible DSP processor.
     *
     * This wrapper transparently handles the upsampling, downsampling, and block sizing requirements
     * of a nested `Processor` instance. It leverages the `AudioParametersBase` architecture to watch
     * for live host changes to the oversampling factor or filter choices.
     *
     * If a parameter change is intercepted at the boundary of a processing block, the wrapper
     * dynamically tears down and reinitializes the underlying `juce::dsp::Oversampling` object
     * and recalculates the upsampled `juce::dsp::ProcessSpec` allocations on-the-fly.
     *
     * @tparam Processor A DSP class type that implements the standard JUCE lifecycle methods:
     *                   `prepare(const juce::dsp::ProcessSpec&)`, `reset()`, and `process(const Context&)`.
     *                   And also provides a std::unique_ptr<ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) method
     *                      (see @sjf::helpers::DummyProcessor)
     */
    template <typename Processor>
    class OversamplingWrapper
    {
    public:
        struct Parameters : sjf::helpers::AudioParametersBase
        {
            ChoiceState ratio, filterType;
            std::unique_ptr<ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) override
            {
                auto factory = ParameterFactory::create (factoryID, factoryName);
                const auto attributes = AudioParameterChoiceAttributes {};//.withAutomatable(false);
                createTrackedParameter(*factory, ratio, "Ratio", "Ratio",{"Off", "2X", "4X", "8X", "16X"}, 0, {}, attributes);
                createTrackedParameter(*factory, filterType, "FilterType", "Filter Type", {"FIR Equiripple", "IIR Polyphase"}, 0, {}, attributes);
                return factory;
            }

        } parameters;

        template <typename... Args>
        OversamplingWrapper (Args&&... args) : processor (std::forward<Args> (args)...) {}

        /** Initialises the processor and internal specs. */
        void prepare (const juce::dsp::ProcessSpec& spec_) noexcept
        {
            spec = spec_;

            createOversampling();
            prepareProcessorUpsampled();

            reset();
        }
        void reset () noexcept
        {
            if (parameters.checkForStateChange() || !oversampling)
            {
                parameters.reset();
                createOversampling();
                prepareProcessorUpsampled();
            }
            else
            {
                oversampling->reset();
            }
            processor.reset();

        }

        template <typename ProcessContext>
        void process (const ProcessContext& context) noexcept
        {
            if (parameters.checkForStateChange())
            {
                parameters.reset();
                createOversampling();
                prepareProcessorUpsampled();
            }
            if (parameters.ratio.currentValue != 0 /* OFF!!! */)
            {
                auto upsampledBlock = oversampling->processSamplesUp(context.getInputBlock());
                dsp::ProcessContextReplacing<float> upsampledContext{upsampledBlock};
                processor.process(upsampledContext);
                oversampling->processSamplesDown(context.getOutputBlock());
            }
            else
            {
                processor.process(context);
            }
        }

        std::unique_ptr<ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName)
        {
            auto factory = processor.createParameters (factoryID, factoryName);
            factory->addChildFactory(parameters.createParameters (factoryID + "OS", factoryName + " Oversampling"));
            return factory;
        }

        void setPositionInfo(const juce::AudioPlayHead::PositionInfo& positionInfo)
        {
            sjf::optional_calls::setPositionInfo(processor, positionInfo);
        }

    	int getLatencySamples()
        {
        	return static_cast<int>(oversampling->getLatencyInSamples()) + sjf::optional_calls::getLatencySamples(processor);
        }

    private:
        void createOversampling()
        {
            oversampling = std::make_unique<juce::dsp::Oversampling<float>>(spec.numChannels, parameters.ratio.currentValue, static_cast<dsp::Oversampling<float>::FilterType>(parameters.filterType.currentValue));
            oversampling->initProcessing(spec.maximumBlockSize);
        	oversampling->setUsingIntegerLatency(true);
        }

        void prepareProcessorUpsampled()
        {
            auto upsampledSpec = spec;
            upsampledSpec.sampleRate *= std::pow(2, parameters.ratio.currentValue);
            upsampledSpec.maximumBlockSize *= static_cast<uint32>(std::pow(2, parameters.ratio.currentValue));
            processor.prepare (upsampledSpec);
        }

        std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
        Processor processor;
        juce::dsp::ProcessSpec spec{};
    };
}
