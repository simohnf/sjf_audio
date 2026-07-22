/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 20/07/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_ParameterFactory.h>
#include <sjf/processors/Waveshaper/sjf_WaveshaperTypeProvider.h>
#include <sjf/helpers/sjf_HelperFunctions.h>

#include "sjf/processors/sjf_Filter_juce.h"
#include <sjf_helpers/sjf/helpers/sjf_ProcessorSequence.h>
#include <sjf_helpers/sjf/helpers/sjf_OversamplingWrapper.h>

namespace sjf::dsp::waveshaper
{

/**
 * @class Waveshaper
 * @brief A multi-type waveshaping processor with sample-accurate parameter smoothing and auto-gain compensation.
 *
 * The Waveshaper class wraps a compile-time set of waveshaping/saturation algorithms provided via a
 * `WaveshaperTypeProvider`. It supports both static (stationary parameter) and smoothed (sample-by-sample
 * interpolated parameter) processing paths, automatically selecting the optimal execution model to minimize
 * CPU overhead during steady states.
 *
 * @tparam WaveshaperTypes A type provider satisfying the `WaveshaperTypeProvider` concept, supplying saturator types and names.
 * @tparam NUM_CHANNELS The number of audio channels to process concurrently (defaults to 2).
 *
 * see @WaveshaperTypeProvider
 */
template<typename WaveshaperTypes, size_t NUM_CHANNELS = 2>
class Waveshaper
{
public:
    struct Parameters : public helpers::AudioParametersBase
    {
        FloatState  drive;

        ChoiceState waveshaper;
        BoolState autoGain;

        std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) override
        {
            auto factory = helpers::ParameterFactory::create (factoryID, factoryName);
            createTrackedParameter(*factory, drive, "Drive", "Drive", {1.0f, 10.0f, 0.001f}, 1.0f);

            createTrackedParameter(*factory, waveshaper, "Type", "Type", WaveshaperTypes::getNames(), 0);
            createTrackedParameter(*factory, autoGain, "AutoGain", "Auto Gain", false);

            return factory;
        }
    } parameters;

    /**
     * @brief Prepares the processor and its parameters for playback using the specified audio specification.
     * @param spec_ The sample rate, block size, and channel count specification.
     */
    void prepare (const juce::dsp::ProcessSpec& spec_) noexcept
    {
        static_assert(helpers::functions::utilities::is_instantiation_of<WaveshaperTypeProvider, WaveshaperTypes>, "The provided template argument is not an instantiation of WaveshaperTypeProvider");
        jassert(spec_.numChannels == NUM_CHANNELS);

        spec = spec_;
        parameters.prepare(spec);

        auto monoSpec = spec;
        monoSpec.numChannels = 1;
        for (auto& ws : waveshapers)
            ws.prepare(monoSpec);

        reset();
    }

    /**
     * @brief Resets the processor state and forces all internal parameter states to instantly match the host.
     */
    void reset() noexcept
    {
        parameters.reset();
        for (auto& ws : waveshapers)
            ws.reset();

        lastWaveshaperIndex = static_cast<size_t>(parameters.waveshaper.currentValue);
    }

    /**
     * @brief Processes an incoming block of audio using either static or smoothed processing paths.
     *
     * @tparam ProcessContext A JUCE process context type (replacing or non-replacing).
     * @param context The current context containing input and output audio blocks.
     */
    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
        jassert (context.getInputBlock().getNumChannels() == NUM_CHANNELS);
        jassert (context.getOutputBlock().getNumChannels() == NUM_CHANNELS);


        parameters.checkForStateChange();

        if (static_cast<size_t>(parameters.waveshaper.currentValue) != lastWaveshaperIndex)
            for (auto& ws : waveshapers)
                ws.reset();

        lastWaveshaperIndex = static_cast<size_t>(parameters.waveshaper.currentValue);

        dispatch(lastWaveshaperIndex, std::make_index_sequence<WaveshaperTypes::numSaturators>{}, context);
    }

    /**
     * @brief Helper accessor to generate and return the underlying parameter layout.
     *
     * @param factoryID Unique ID prefix prepended to all registered parameter IDs.
     * @param factoryName Display prefix prepended to all parameter automation labels.
     * @return A unique pointer to the configured ParameterFactory group.
     */
    std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName)
    {
        return parameters.createParameters (factoryID, factoryName);
    }

    // /**
    //  * @brief Updates the processor with timing and transport metadata from the host daw playhead.
    //  * @param positionInfo_ An optional structure containing metrics like BPM, time signature, and play-state.
    //  */
    // void setPositionInfo(const Optional<juce::AudioPlayHead::PositionInfo>&)
    // {
    //
    // }

private:
    template <std::size_t... Indices, typename ProcessContext>
    void dispatch (const size_t targetIndex, std::index_sequence<Indices...>, const ProcessContext& context) noexcept
    {
        if (parameters.isSmoothing())
            (void)((targetIndex == static_cast<size_t>(Indices) ? (processSmoothedState<Indices>(context), true) : false) || ...);
        else
            (void)((targetIndex == static_cast<size_t>(Indices) ? (processStaticState<Indices>(context), true) : false) || ...);
    }


    /**
     * @brief Processes audio using vectorizable, channel-by-channel loops when parameters are stationary.
     *
     * @tparam ProcessContext A JUCE process context type.
     * @param context The current context containing input and output audio blocks.
     */
    template <size_t WaveshaperIndex, typename ProcessContext>
    void processStaticState (const ProcessContext& context) noexcept
    {
        const auto& inputBlock = context.getInputBlock();
        auto& outputBlock      = context.getOutputBlock();
        const auto numSamples  = outputBlock.getNumSamples();

        jassert (inputBlock.getNumSamples() == numSamples);

        const auto drive = parameters.drive.currentValue;
        const auto autoGain = parameters.autoGain.currentValue;
        for (size_t channel = 0; channel < NUM_CHANNELS; ++channel)
        {
            const auto input = inputBlock.getChannelPointer (channel);
            auto output = outputBlock.getChannelPointer (channel);

            auto gainCompensation = autoGain ? waveshapers[channel].template getCompensationGain<WaveshaperIndex>(drive) : 1.0f;

            for (size_t i = 0; i < numSamples; ++i)
                output[i] = gainCompensation * waveshapers[channel].template processSample<WaveshaperIndex>(input[i]);
        }
    }

    /**
     * @brief Processes audio sample-by-sample to recalculate linear parameter curves at audio rate.
     *
     * @tparam ProcessContext A JUCE process context type.
     * @param context The current context containing input and output audio blocks.
     */
    template <size_t WaveshaperIndex, typename ProcessContext>
    void processSmoothedState (const ProcessContext& context) noexcept
    {
        const auto& inputBlock = context.getInputBlock();
        auto& outputBlock      = context.getOutputBlock();
        const auto numSamples  = outputBlock.getNumSamples();

        jassert (inputBlock.getNumSamples() == numSamples);

        std::array<const float*, NUM_CHANNELS> inputChannelPointers;
        std::array<float*, NUM_CHANNELS> outputChannelPointers;

        for (auto channel = 0ul; channel < NUM_CHANNELS; ++channel)
        {
            inputChannelPointers[channel] = context.getInputBlock().getChannelPointer (channel);
            outputChannelPointers[channel] = context.getOutputBlock().getChannelPointer (channel);
        }


        const auto autoGain = parameters.autoGain.currentValue;

        for (size_t i = 0; i < numSamples; ++i)
        {
            parameters.tickSmoothers();
            const auto drive = parameters.drive.currentValue;
            for (size_t channel = 0; channel < NUM_CHANNELS; ++channel)
            {
                const auto gainCompensation = autoGain ? waveshapers[channel].template getCompensationGain<WaveshaperIndex>(drive) : 1.0f;
                outputChannelPointers[channel][i] = gainCompensation * waveshapers[channel].template processSample<WaveshaperIndex>(inputChannelPointers[channel][i]);
            }
        }
    }

    std::array<WaveshaperTypes, NUM_CHANNELS> waveshapers;
    juce::dsp::ProcessSpec spec{};

    size_t lastWaveshaperIndex = 0;
};




/**
 * @class FilteredWaveshaper
 * @brief A composite audio processor combining pre/post state-variable filters with an oversampled waveshaper.
 *
 * Utilizes a `ProcessorSequence` to chain a pre-filter, an oversampled `Waveshaper`, and a post-filter.
 * Provides hierarchical parameter registration and transport state management.
 *
 * @tparam WaveshaperTypes A type provider supplying saturator types for the core waveshaper.
 * @tparam NUM_CHANNELS The number of audio channels to process concurrently (defaults to 2).
 */
template <typename WaveshaperTypes, size_t NUM_CHANNELS = 2>
class FilteredWaveshaper
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec_)
    {
        sequence.prepare(spec_);
    }

    void reset()
    {
        sequence.reset();
    }

    template<typename ProcessContext>
    void process(const ProcessContext& context) noexcept
    {
        sequence.process(context);
    }

    std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName)
    {
        return sequence.createParameters (factoryID, factoryName,
                                            helpers::processor_sequence::SubFactoryConfig("PreFilter", " Pre Filter"),
                                            helpers::processor_sequence::SubFactoryConfig("Waveshaper", " Waveshaper"),
                                            helpers::processor_sequence::SubFactoryConfig("PostFilter", " Post Filter")
                                            );
    }


    void setPositionInfo(const juce::AudioPlayHead::PositionInfo& positionInfo)
    {
        sequence.setPositionInfo(positionInfo);
    }

private:

    using Filter = helpers::BypassWrapper<SVF<>, helpers::bypass_wrapper_config::Bypass>;

    /**
     * @brief Internal DSP processing sequence: [Pre-Filter] -> [Oversampled Waveshaper] -> [Post-Filter].
     */
    sjf::helpers::ProcessorSequence<Filter, helpers::OversamplingWrapper<Waveshaper<WaveshaperTypes, NUM_CHANNELS>>, Filter> sequence;
};

}
