//
// Created by Simon Fay on 10/07/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_ParameterFactory.h>

namespace sjf
{
class DummyProcessor
{
public:
    struct Parameters : public helpers::AudioParametersBase
    {
        /// NOTE: You need to ensure all TrackedState objects of a given type are declared consecutively!!!
        FloatState  cutoff, gain;
        IntState    quality;
        BoolState   myBool;
        ChoiceState mode;


        std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) override
        {
            auto factory = helpers::ParameterFactory::create (factoryID, factoryName);
            createTrackedParameter  (*factory, cutoff, "cutoff",  "Cutoff Freq",  { 20.0f, 20000.0f, 0.5f }, 1000.0f);
            createTrackedParameter    (*factory, quality, "quality", "Oversampling", 1, 4, 1);
            createTrackedParameter   (*factory, myBool, "MyBool",  "My Bool", false);
            createTrackedParameter (*factory, mode, "mode",    "Filter Mode",  { "Lowpass", "Highpass" }, 0);

            createTrackedParameter (*factory, gain, "gain",    "Master Volume", { -60.0f, 6.0f }, 0.0f,
                                                    [](const float dB) { return juce::Decibels::decibelsToGain (dB); });

            return factory;
        }
    };



    void prepare (const juce::dsp::ProcessSpec& spec_)
    {
        spec = spec_;
        parameters.prepare(spec);
        reset();
    }

    void reset()
    {
        parameters.reset();
    }

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

    std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName)
    {
        return parameters.createParameters (factoryID, factoryName);
    }

private:
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
                outputSamples[i] = inputSamples[i]; // your process logic goes here
        }

    }

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
            // Remember to tick the smoothers!!! for every sample
            parameters.tickSmoothers();

            /// Do all of your audio processing in here
            /// Unfortunately, it need to do approach this by sample, rather than by channel
            /// so that the smoothers are updated correctly
        }

    }

    juce::dsp::ProcessSpec spec{};
    Parameters parameters;
};
}