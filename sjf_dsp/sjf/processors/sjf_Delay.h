//
// Created by Simon Fay on 10/07/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_ParameterFactory.h>
#include <sjf/helpers/sjf_DelayLine.h>

#include "sjf/helpers/sjf_Filter_juce.h"
#include "sjf/helpers/sjf_Waveshapers.h"

namespace sjf
{
class Delay
{
    static constexpr auto MAX_DELAY_MS = 10000.0f;
    static constexpr auto NUM_CHANNELS = 2;
public:
    struct SaturationTypes
    {
        enum class Enum {None, Soft, Overdrive, Tape, BucketBrigade, Hard, COUNT, DEFAULT = None};

        static StringArray getNames(){ return {"None", "Soft", "Overdrive", "Tape", "BucketBrigade", "Hard"};}

        static int getIndex(Enum e){ return static_cast<int>(e); }
        static Enum asEnum(int index){ return static_cast<Enum>(index); }
        static int getDefaultIndex(){ return getIndex(Enum::DEFAULT);}
    };

    struct Parameters : public helpers::AudioParametersBase
    {
        std::array<FloatState, NUM_CHANNELS> delayTimes;
        FloatState feedback, drive;

        BoolState link, pingPong;
        ChoiceState saturationType;

        juce::dsp::ProcessSpec& spec;

        explicit Parameters(juce::dsp::ProcessSpec& spec_) : spec(spec_) {}

        std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) override
        {
            static_assert(NUM_CHANNELS > 0, "Number of channels must be greater than 0!!!");
            auto factory = helpers::ParameterFactory::create (factoryID, factoryName);
            auto delayTimeStrings = [](){
                if constexpr(NUM_CHANNELS == 1)
                    return std::vector<String>{""};
                if constexpr (NUM_CHANNELS == 2)
                    return std::vector<String>{"Left", "Right"};
                return std::vector<String>{""};
            }();
            auto strIndex = 0ul;
            for (auto& delayTime : delayTimes)
            {
                const auto range = juce::NormalisableRange<float>{ 1.0f, MAX_DELAY_MS, 0.01f };
                const auto attributes = juce::AudioParameterFloatAttributes{}
                                                                                .withLabel("ms");
                const auto mapping = [&](const float x){ return x * spec.sampleRate * 0.001f;};
                const auto& str = delayTimeStrings[strIndex++];
                createTrackedParameter  (*factory, delayTime, "Time" + str.substring(0, 1),  "Time "+ str +" (ms)",  range, 100.0f, mapping, attributes);
            }
            {
                if constexpr (NUM_CHANNELS > 1)
                    createTrackedParameter(*factory, link, "Link", "Link", false);
                else
                    link.currentValue = true;
            }
            {
                if constexpr (NUM_CHANNELS > 1)
                    createTrackedParameter(*factory, pingPong, "PingPong", "Ping Pong", false);
                else
                    pingPong.currentValue = false;
            }

            {
                const auto range = juce::NormalisableRange<float>{ 0.0f, 100.0f, 0.01f };
                const auto attributes = juce::AudioParameterFloatAttributes{}
                                                                                .withLabel("%");
                const auto mapping = [&](const float x){ return x * 0.01f;};
                createTrackedParameter  (*factory, feedback, "Feedback",  "Feedback",  range, 0.0f, mapping, attributes);
            }

            {
                createTrackedParameter(*factory, saturationType, "SaturationType", "Saturation Type", SaturationTypes::getNames(), SaturationTypes::getDefaultIndex());
                const auto range = juce::NormalisableRange<float>{ 0.0f, 100.0f, 0.01f };
                const auto attributes = juce::AudioParameterFloatAttributes{}
                .withLabel("%");
                const auto mapping = [&](const float x){ return jmap(x * 0.01f, 0.01f, 1.0f);};
                createTrackedParameter  (*factory, drive, "Drive",  "Drive",  range, 0.0f, mapping, attributes);
            }



            return factory;
        }
    };

    Delay() : parameters(spec), inputChannelPointers({}), outputChannelPointers({})
    {
    }


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
        filter.prepare(spec);
        reset();
    }

    void reset()
    {
        parameters.reset();
        for (auto& dl : delayLine)
            dl.reset();
        filter.reset();
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
        auto factory = parameters.createParameters (factoryID, factoryName);;
        factory->addChild(filter.createParameters("Filter", "Filter"));
        return factory;
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

        AudioBuffer<float> oneSampleBuffer(1, 1);
        dsp::AudioBlock<float> onesampleBlock(oneSampleBuffer);
        dsp::ProcessContextReplacing<float> oneSampleContext(onesampleBlock);
        auto oneSamplePointer = onesampleBlock.getChannelPointer (0);
        auto& sample = oneSamplePointer[0];


        const auto feedback = parameters.feedback.currentValue;
        const auto saturationType = SaturationTypes::asEnum(parameters.saturationType.currentValue);

        const auto drive = saturationType != SaturationTypes::Enum::None ? parameters.drive.currentValue : 1.0f;
        const auto norm = drive > 0.0f ? 1.0f / applySaturation(drive, saturationType) : 1.0f;


        for (auto channel = 0ul; channel < numChannels; ++channel)
        {
            inputChannelPointers[channel] = inputBlock.getChannelPointer (channel);
            outputChannelPointers[channel] = outputBlock.getChannelPointer (channel);
        }


        if (!parameters.pingPong.currentValue)
        {
            for (auto i = 0ul; i < numSamples; ++i)
            {
                for (auto channel = 0ul; channel < numChannels; ++channel)
                {
                    const auto delayTime = parameters.link.currentValue ? parameters.delayTimes[0].currentValue : parameters.delayTimes[channel].currentValue;
                    sample = delayLine[channel].readSample<sjf::interpolation::InterpolatorTypes::cubic>(delayTime);
                    sample = applySaturation(sample * drive, saturationType) * norm;

                    filter.process(oneSampleContext, channel);
                    delayLine[channel].writeSample(inputChannelPointers[channel][i] + feedback * sample);
                    outputChannelPointers[channel][i] = sample;
                }
            }
        }
        else
        {
            const auto scale = 1.0f / sqrtf(NUM_CHANNELS);
            for (auto i = 0ul; i < numSamples; ++i)
            {
                auto input = 0.0f;
                for (auto channel = 0ul; channel < numChannels; ++channel)
                    input += inputChannelPointers[channel][i];
                input *= scale;
                for (auto channel = 0ul; channel < numChannels; ++channel)
                {

                    const auto delayTime = parameters.link.currentValue ? parameters.delayTimes[0].currentValue : parameters.delayTimes[channel].currentValue;
                    sample = delayLine[channel].readSample<sjf::interpolation::InterpolatorTypes::cubic>(delayTime);
                    sample = applySaturation(sample * drive, saturationType) * norm;

                    filter.process(oneSampleContext, channel);
                    const auto wc = channel+1 < NUM_CHANNELS ? channel+1 : 0;
                    delayLine[wc].writeSample(input + feedback * sample);
                    input = 0.0f;
                    outputChannelPointers[channel][i] = sample;
                }
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

        AudioBuffer<float> oneSampleBuffer(1, 1);
        dsp::AudioBlock<float> onesampleBlock(oneSampleBuffer);
        dsp::ProcessContextReplacing<float> oneSampleContext(onesampleBlock);
        auto oneSamplePointer = onesampleBlock.getChannelPointer (0);
        auto& sample = oneSamplePointer[0];

        for (auto channel = 0ul; channel < numChannels; ++channel)
        {
            inputChannelPointers[channel] = inputBlock.getChannelPointer (channel);
            outputChannelPointers[channel] = outputBlock.getChannelPointer (channel);
        }

        const auto saturationType = SaturationTypes::asEnum(parameters.saturationType.currentValue);
        if (!parameters.pingPong.currentValue)
        {

            for (size_t i = 0; i < numSamples; ++i)
            {
                // Remember to tick the smoothers!!! for every sample
                parameters.tickSmoothers();

                const auto feedback = parameters.feedback.currentValue;
                const auto drive = saturationType != SaturationTypes::Enum::None ? parameters.drive.currentValue : 1.0f;
                const auto norm = drive > 0.0f ? 1.0f / applySaturation(drive, saturationType) : 1.0f;
                for (auto channel = 0ul; channel < numChannels; ++channel)
                {
                    const auto delayTime = parameters.link.currentValue ? parameters.delayTimes[0].currentValue : parameters.delayTimes[channel].currentValue;
                    sample = delayLine[channel].readSample<sjf::interpolation::InterpolatorTypes::cubic>(delayTime);
                    sample = applySaturation(sample * drive, saturationType) * norm;
                    filter.process(oneSampleContext, channel);


                    delayLine[channel].writeSample(inputChannelPointers[channel][i] + feedback * sample);
                    outputChannelPointers[channel][i] = sample;
                }
            }
        }
        else
        {
            const auto scale = 1.0f / sqrtf(NUM_CHANNELS);
            for (size_t i = 0; i < numSamples; ++i)
            {
                // Remember to tick the smoothers!!! for every sample
                parameters.tickSmoothers();

                auto input = 0.0f;
                for (auto channel = 0ul; channel < numChannels; ++channel)
                    input += inputChannelPointers[channel][i];
                input *= scale;


                const auto feedback = parameters.feedback.currentValue;
                const auto drive = saturationType != SaturationTypes::Enum::None ? parameters.drive.currentValue : 1.0f;
                const auto norm = drive > 0.0f ? 1.0f / applySaturation(drive, saturationType) : 1.0f;
                for (auto channel = 0ul; channel < numChannels; ++channel)
                {
                    const auto delayTime = parameters.link.currentValue ? parameters.delayTimes[0].currentValue : parameters.delayTimes[channel].currentValue;
                    sample = delayLine[channel].readSample<sjf::interpolation::InterpolatorTypes::cubic>(delayTime);
                    sample = applySaturation(sample * drive, saturationType) * norm;
                    filter.process(oneSampleContext, channel);
                    const auto wc = channel+1 < NUM_CHANNELS ? channel+1 : 0;
                    delayLine[wc].writeSample(input + feedback * sample);
                    input = 0.0f;
                    outputChannelPointers[channel][i] = sample;
                }
            }
        }

    }

    static float applySaturation(float x, const SaturationTypes::Enum type)
    {
        using Types = SaturationTypes::Enum;
        switch (type)
        {
        case Types::None:
            return applySaturationTagged<Types::None>(x);
        case SaturationTypes::Enum::Soft:
            return applySaturationTagged<Types::Soft>(x);
        case SaturationTypes::Enum::Overdrive:
            return applySaturationTagged<Types::Overdrive>(x);
        case SaturationTypes::Enum::Tape:
            return applySaturationTagged<Types::Tape>(x);
        case SaturationTypes::Enum::BucketBrigade:
            return applySaturationTagged<Types::BucketBrigade>(x);
        case SaturationTypes::Enum::Hard:
            return applySaturationTagged<Types::Hard>(x);
        case SaturationTypes::Enum::COUNT:
            return applySaturationTagged<Types::COUNT>(x);
        default:
            return applySaturationTagged<Types::None>(x);
        }
    }

    template <SaturationTypes::Enum type>
    static float applySaturationTagged(const float x)
    {
        if constexpr (type == SaturationTypes::Enum::Soft)
        {
            return sjf::helpers::Waveshapers::Clippers::soft(x);
        }
        else if constexpr (type == SaturationTypes::Enum::Overdrive)
        {
            return sjf::helpers::Waveshapers::Clippers::tanh(x);
        }
        else if constexpr (type == SaturationTypes::Enum::Tape)
        {
            return sjf::helpers::Waveshapers::Sigmoids::xOverOnePlusAbsX(x);
        }
        else if constexpr (type == SaturationTypes::Enum::BucketBrigade)
        {
            return sjf::helpers::Waveshapers::Misc::bucketBrigade(x);
        }
        else if constexpr (type == SaturationTypes::Enum::Hard)
        {
            return sjf::helpers::Waveshapers::Clippers::hard(x);
        }
        else // if constexpr (type == SaturationTypes::Enum::None)
        {
            return x;
        }

    }

    juce::dsp::ProcessSpec spec{};
    Parameters parameters;
    std::array<sjf::helpers::DelayLine, NUM_CHANNELS> delayLine;
    sjf::helpers::SVF<true, true> filter;
    std::array<const float*, NUM_CHANNELS> inputChannelPointers;
    std::array<float*, NUM_CHANNELS> outputChannelPointers;
};
}
