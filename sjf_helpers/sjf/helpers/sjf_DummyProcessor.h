//
// Created by Simon Fay on 10/07/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_ParameterFactory.h>

namespace sjf
{
/**
 * @brief A template audio processor demonstrating state-managed parameter tracking,
 *        fast-path rendering, and sample-accurate linear parameter smoothing.
 *
 * This class serves as a reference implementation for integrating the `AudioParametersBase`
 * framework into a standard `juce::dsp` processing pipeline.
 */
class DummyProcessor
{
public:
    /**
     * @brief Parameters container managing the layout and tracking of all host-exposed controls.
     */
    struct Parameters : public helpers::AudioParametersBase
    {
        /**
         * @name Tracked Parameter States
         * @note TrackedState objects of the same type must be declared consecutively.
         * @{
         */
        FloatState  cutoff, gain;
        IntState    quality;
        BoolState   myBool;
        ChoiceState mode;
        /** @} */

        /**
         * @brief Builds the parameter hierarchy and registers tracking associations.
         *
         * @param factoryID Unique ID prefix prepended to all registered parameter IDs.
         * @param factoryName Display prefix prepended to all parameter automation labels.
         * @return A unique pointer to the configured ParameterFactory group.
         */
        std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) override
        {
            auto factory = helpers::ParameterFactory::create (factoryID, factoryName);
            createTrackedParameter  (*factory, cutoff, "cutoff",  "Cutoff Freq",  { 20.0f, 20000.0f, 0.5f }, 1000.0f);
            createTrackedParameter  (*factory, quality, "quality", "Oversampling", 1, 4, 1);
            createTrackedParameter  (*factory, myBool, "MyBool",  "My Bool", false);
            createTrackedParameter  (*factory, mode, "mode",    "Filter Mode",  { "Lowpass", "Highpass" }, 0);

            // Registers gain with an inline lambda mapping decibels to linear gain coefficients
            createTrackedParameter  (*factory, gain, "gain",    "Master Volume", { -60.0f, 6.0f }, 0.0f,
                                                    [](const float dB) { return juce::Decibels::decibelsToGain (dB); });

            return factory;
        }
    } parameters;

    /**
     * @brief Prepares the processor and its parameters for playback using the specified audio specification.
     * @param spec_ The sample rate, block size, and channel count specification.
     */
    void prepare (const juce::dsp::ProcessSpec& spec_)
    {
        spec = spec_;
        parameters.prepare(spec);
        reset();
    }

    /**
     * @brief Resets the processor state and forces all internal parameter states to instantly match the host.
     */
    void reset()
    {
        parameters.reset();
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
        const auto& inputBlock = context.getInputBlock();
        auto& outputBlock      = context.getOutputBlock();
        const auto numChannels = outputBlock.getNumChannels();
        const auto numSamples  = outputBlock.getNumSamples();

        jassert (inputBlock.getNumChannels() == numChannels);
        jassert (inputBlock.getNumSamples() == numSamples);

        if (parameters.checkForStateChange())
        {
            processSmoothedState(context);
        }
        else
        {
            processStaticState(context);
        }
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

    /**
     * @brief Updates the processor with timing and transport metadata from the host daw playhead.
     * @param positionInfo_ An optional structure containing metrics like BPM, time signature, and play-state.
     */
    void setPositionInfo(const Optional<juce::AudioPlayHead::PositionInfo>&)
    {

    }

private:
    /**
     * @brief Processes audio using vectorizable, channel-by-channel loops when parameters are stationary.
     *
     * @tparam ProcessContext A JUCE process context type.
     * @param context The current context containing input and output audio blocks.
     */
    template <typename ProcessContext>
    void processStaticState (const ProcessContext& context) noexcept
    {
        const auto& inputBlock = context.getInputBlock();
        auto& outputBlock      = context.getOutputBlock();
        const auto numChannels = outputBlock.getNumChannels();
        const auto numSamples  = outputBlock.getNumSamples();

        jassert (inputBlock.getNumChannels() == numChannels);
        jassert (inputBlock.getNumSamples() == numSamples);

        for (size_t channel = 0; channel < numChannels; ++channel)
        {
            auto* inputSamples  = inputBlock.getChannelPointer (channel);
            auto* outputSamples = outputBlock.getChannelPointer (channel);

            for (size_t i = 0; i < numSamples; ++i)
                outputSamples[i] = inputSamples[i]; // Audio processing logic
        }
    }

    /**
     * @brief Processes audio sample-by-sample to recalculate linear parameter curves at audio rate.
     *
     * @tparam ProcessContext A JUCE process context type.
     * @param context The current context containing input and output audio blocks.
     */
    template <typename ProcessContext>
    void processSmoothedState (const ProcessContext& context) noexcept
    {
        const auto& inputBlock = context.getInputBlock();
        auto& outputBlock      = context.getOutputBlock();
        const auto numChannels = outputBlock.getNumChannels();
        const auto numSamples  = outputBlock.getNumSamples();

        jassert (inputBlock.getNumChannels() == numChannels);
        jassert (inputBlock.getNumSamples() == numSamples);

        for (size_t i = 0; i < numSamples; ++i)
        {
            parameters.tickSmoothers();

            // Sample-by-sample audio processing logic occurs here
        }
    }

    juce::dsp::ProcessSpec spec{};
};
}