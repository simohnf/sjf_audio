//
// Created by Simon Fay on 10/07/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_ParameterFactory.h>
#include <sjf/helpers/sjf_DelayLine.h>

namespace sjf
{
class Delay
{
    static constexpr auto MAX_DELAY_MS = 10000.0f;
    static constexpr auto NUM_CHANNELS = 2;
public:
    struct Parameters : public helpers::AudioParametersBase
    {
        FloatState  delayTime, feedback;
        // IntState    quality;
        // BoolState   myBool;
        // ChoiceState mode;

        juce::dsp::ProcessSpec& spec;

        explicit Parameters(juce::dsp::ProcessSpec& spec_) : spec(spec_) {}

        std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) override
        {
            auto factory = helpers::ParameterFactory::create (factoryID, factoryName);

            {
                const auto range = juce::NormalisableRange<float>{ 1.0f, MAX_DELAY_MS, 0.01f };
                const auto attributes = juce::AudioParameterFloatAttributes{}
                                                                                .withLabel("ms");
                const auto mapping = [&](const float x){ return x * spec.sampleRate * 0.001f;};
                createTrackedParameter  (*factory, delayTime, "Time",  "Time (ms)",  range, 100.0f, mapping, attributes);
            }

            {
                const auto range = juce::NormalisableRange<float>{ 0.0f, 100.0f, 0.01f };
                const auto attributes = juce::AudioParameterFloatAttributes{}
                                                                                .withLabel("%");
                const auto mapping = [&](const float x){ return x * 0.01f;};
                createTrackedParameter  (*factory, feedback, "Feedback",  "Feedback",  range, 0.0f, mapping, attributes);
            }

            return factory;
        }
    };

    Delay() : parameters(spec) {}


    void prepare (const juce::dsp::ProcessSpec& spec_)
    {
        spec = spec_;
        jassert(spec.numChannels == NUM_CHANNELS);

        parameters.prepare(spec);
        for (auto& dl : delayLine)
        {
            dl.prepare(spec);
            dl.setMaxDelayTimeMS(MAX_DELAY_MS);
        }
        reset();
    }

    void reset()
    {
        parameters.reset();
        for (auto& dl : delayLine)
            dl.reset();

    }

    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
        const auto& inputBlock = context.getInputBlock();
        auto& outputBlock      = context.getOutputBlock();
        const auto numChannels = outputBlock.getNumChannels();
        jassert(numChannels == NUM_CHANNELS);
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

        const auto delayTime = parameters.delayTime.currentValue;
        const auto feedback = parameters.feedback.currentValue;
        for (auto channel = 0ul; channel < numChannels; ++channel)
        {
            auto* inputSamples  = inputBlock.getChannelPointer (channel);
            auto* outputSamples = outputBlock.getChannelPointer (channel);

            for (auto i = 0ul; i < numSamples; ++i)
            {
                const auto popped = delayLine[channel].readSample<sjf::interpolation::InterpolatorTypes::cubic>(delayTime);
                delayLine[channel].writeSample(inputSamples[i] + feedback * popped);
                outputSamples[i] = popped;
            }
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

        for (auto channel = 0ul; channel < numChannels; ++channel)
        {
            inputChannelPointers[channel] = inputBlock.getChannelPointer (channel);
            outputChannelPointers[channel] = outputBlock.getChannelPointer (channel);
        }

        for (size_t i = 0; i < numSamples; ++i)
        {
            // Remember to tick the smoothers!!! for every sample
            parameters.tickSmoothers();
            const auto delayTime = parameters.delayTime.currentValue;
            const auto feedback = parameters.feedback.currentValue;
            for (auto channel = 0ul; channel < numChannels; ++channel)
            {
                const auto popped = delayLine[channel].readSample<sjf::interpolation::InterpolatorTypes::cubic>(delayTime);
                delayLine[channel].writeSample(inputChannelPointers[channel][i] + feedback * popped);
                outputChannelPointers[channel][i] = popped;
            }
        }

    }

    juce::dsp::ProcessSpec spec{};
    Parameters parameters;
    std::array<sjf::helpers::DelayLine, NUM_CHANNELS> delayLine;
    std::array<const float*, NUM_CHANNELS> inputChannelPointers;
    std::array<float*, NUM_CHANNELS> outputChannelPointers;
};
}