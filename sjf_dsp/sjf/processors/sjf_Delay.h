//
// Created by Simon Fay on 10/07/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_ParameterFactory.h>
#include <sjf/helpers/sjf_DelayLine.h>

#include "sjf_Filter_juce.h"
#include "sjf/helpers/sjf_DCBlock.h"
#include "sjf/helpers/sjf_ProcessDuplicator.h"
#include "sjf/helpers/sjf_Waveshapers.h"
#include "sjf/oscillators/LFO/sjf_LFO.h"

namespace sjf::dsp
{
class Delay
{
    static constexpr auto MAX_DELAY_MS = 10000.0f;
    static constexpr auto NUM_CHANNELS = 2;

    using LFO = sjf::dsp::oscillators::lfo::LFO<oscillators::lfo::DefaultWaveformProvider,
                                                oscillators::lfo::configurations::Depth,
                                                oscillators::lfo::configurations::Invert,
                                                oscillators::lfo::configurations::PhaseOffset,
                                                oscillators::lfo::configurations::Smooth>;

    using Filter = sjf::helpers::BypassWrapper<sjf::dsp::SVF<true, true>, helpers::bypass_wrapper_config::Bypass>;

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
        std::array<helpers::SyncedDurationParameter<>, NUM_CHANNELS> delayTimes{helpers::SyncedDurationParameter<>{1.0f, 10000.0f, 100.0f, 1000.0f},
                                                                                helpers::SyncedDurationParameter<>(1.0f, 10000.0f, 100.0f, 1000.0f)};
        std::array<FloatState, NUM_CHANNELS> detune;
        FloatState feedback, drive;

        BoolState link, pingPong;
        ChoiceState saturationType;

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

            // Delay Times
            {
                auto strIndex = 0ul;
                for (auto& delayTime : delayTimes)
                {
                    const auto& str = delayTimeStrings[strIndex++];
                    factory->addChild(delayTime.createParameters("Time" + str.substring(0, 1), "Time " + str));
                    addTrackedChildParameters(delayTime);
                }
            }

            // Detune
            {
                auto strIndex = 0ul;
                for (auto& d : detune)
                {
                    auto range = juce::NormalisableRange<float>{ -100.0f, 100.0f, 0.01f };
                    const auto attributes = juce::AudioParameterFloatAttributes{}   .withLabel("cents");
                    const auto mapping = [&](const float x){ return x * 0.01f;};
                    const auto& str = delayTimeStrings[strIndex++];
                    createTrackedParameter  (*factory, d, "Detune" + str.substring(0, 1),  "Detune "+ str +" (cents)",  range, 0.0f, mapping, attributes);
                }
            }

            // Link
            {
                if constexpr (NUM_CHANNELS > 1)
                    createTrackedParameter(*factory, link, "Link", "Link", false);
                else
                    link.currentValue = true;
            }

            // Ping Pong
            {
                if constexpr (NUM_CHANNELS > 1)
                    createTrackedParameter(*factory, pingPong, "PingPong", "Ping Pong", false);
                else
                    pingPong.currentValue = false;
            }

            // Feedback
            {
                const auto range = juce::NormalisableRange<float>{ 0.0f, 100.0f, 0.01f };
                const auto attributes = juce::AudioParameterFloatAttributes{}   .withLabel("%");
                const auto mapping = [&](const float x){ return x * 0.01f;};
                createTrackedParameter  (*factory, feedback, "Feedback",  "Feedback",  range, 0.0f, mapping, attributes);
            }

            // Saturation
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

        void setPositionInfo(const Optional<juce::AudioPlayHead::PositionInfo>& positionInfo)
        {
            for ( auto& dt : delayTimes)
                dt.setPositionInfo(positionInfo);
        }
    } parameters;

    Delay()
    : inputChannelPointers({}), outputChannelPointers({})
    {}


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

        dcBlocker.prepare(spec);

        filter.prepare(spec);
        lfo.prepare(spec);

        reset();
    }

    void reset()
    {
        parameters.reset();
        for (auto& dl : delayLine)
            dl.reset();
        dcBlocker.reset();

        filter.reset();
        lfo.reset();
    }

    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
        parameters.checkForStateChange();
        processInternal(context);
    }

    std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName)
    {
        auto factory = parameters.createParameters (factoryID, factoryName);
        factory->addChild(filter.createParameters("Filter", "Filter"));
        factory->addChild(dcBlocker.createParameters("DCBlock", "DCBlock"));
        factory->addChild(lfo.createParameters("Mod", "Modulation "));
        return factory;
    }

    void setPositionInfo(const Optional<juce::AudioPlayHead::PositionInfo>& positionInfo)
    {
        parameters.setPositionInfo(positionInfo);
    }

private:
    template <typename ProcessContext>
    void processInternal (const ProcessContext& context) noexcept
    {
        const auto& inputBlock = context.getInputBlock();
        auto& outputBlock      = context.getOutputBlock();
        const auto numChannels = outputBlock.getNumChannels();
        const auto numSamples  = outputBlock.getNumSamples();

        jassert (inputBlock.getNumChannels() == NUM_CHANNELS);
        jassert (outputBlock.getNumChannels() == NUM_CHANNELS);

        AudioBuffer<float> oneSampleBuffer(NUM_CHANNELS, 1);
        const auto wptrs = oneSampleBuffer.getArrayOfWritePointers();
        juce::dsp::AudioBlock<float> oneSampleBlock(oneSampleBuffer);
        const juce::dsp::ProcessContextReplacing<float> oneSampleContext(oneSampleBlock);


        for (auto channel = 0ul; channel < numChannels; ++channel)
        {
            inputChannelPointers[channel] = inputBlock.getChannelPointer (channel);
            outputChannelPointers[channel] = outputBlock.getChannelPointer (channel);
        }

        const auto saturationType = SaturationTypes::asEnum(parameters.saturationType.currentValue);
        const auto pingPong = parameters.pingPong.currentValue;



        const auto calculateInput = [&](const size_t channel, const size_t i)
        {
            if (pingPong)
            {
                const auto sumInputs = [&]()
                {
                    const auto scale = 1.0f / sqrtf(NUM_CHANNELS);
                    auto input = 0.0f;
                    for (const auto cptr : inputChannelPointers)
                        input += cptr[i];
                    input *= scale;
                    return input;
                };

                if (channel == 0)
                    return sumInputs();

                return 0.0f;
            }
            return inputChannelPointers[channel][i];
        };

        const auto calculateWriteChannel = [&](const size_t channel)
        {
            if (pingPong)
                return channel+1 < NUM_CHANNELS ? channel+1 : 0;
            return channel;
        };

        lfo.process(context);

        const auto lfoBlock = lfo.getLfoOutput().getSubBlock(0, numSamples);


        for (size_t i = 0; i < numSamples; ++i)
        {
            // Remember to tick the smoothers!!! for every sample
            parameters.tickSmoothers();

            const auto feedback = parameters.feedback.currentValue;
            const auto drive = saturationType != SaturationTypes::Enum::None ? parameters.drive.currentValue : 1.0f;
            const auto norm = drive > 0.0f ? 1.0f / applySaturation(drive, saturationType) : 1.0f;

            for (auto channel = 0ul; channel < numChannels; ++channel)
            {
                auto& sample = wptrs[channel][0];
                auto delayTime = parameters.link.currentValue ? parameters.delayTimes[0].time.currentValue : parameters.delayTimes[channel].time.currentValue;
                delayTime += delayTime * lfoBlock.getSample(static_cast<int>(channel), static_cast<int>(i));
                const auto detune = parameters.detune[channel].currentValue;
                delayLine[channel].setPitchShift(detune);
                sample = delayLine[channel].readSample<sjf::interpolation::InterpolatorTypes::cubic>(delayTime);
                sample = applySaturation(sample * drive, saturationType) * norm;
            }

            dcBlocker.process(oneSampleContext);
            filter.process(oneSampleContext);

            for (auto channel = 0ul; channel < numChannels; ++channel)
            {
                const auto& sample = wptrs[channel][0];
                const auto wc = calculateWriteChannel(channel);
                const auto input = calculateInput(channel, i);
                delayLine[wc].writeSample(input + feedback * sample);
                outputChannelPointers[channel][i] = sample;
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
    std::array<sjf::helpers::PitchShiftDelayLine<>, NUM_CHANNELS> delayLine;
    LFO lfo;
    sjf::helpers::ProcessorDuplicator<helpers::DCBlocker<>> dcBlocker;
    Filter filter;
    std::array<const float*, NUM_CHANNELS> inputChannelPointers;
    std::array<float*, NUM_CHANNELS> outputChannelPointers;
};
}
