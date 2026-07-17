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
#include <sjf/processors/Waveshaper/sjf_WaveshaperTypeProvider.h>
#include "sjf/oscillators/LFO/sjf_LFO.h"

namespace sjf::dsp
{

namespace delay_config
{
    struct PingPong{};
    struct Filter{};
    struct Detune{};
    struct TempoSync{};
    struct LFOTempoSync{};
    struct Link{};

}


template<typename ... Configurations>
class Delay
{
public:
    static constexpr auto MAX_DELAY_MS = 10000.0f;
    static constexpr auto NUM_CHANNELS = 2; // TO DO: Enable Mono Delay



    static constexpr auto hasPingPong = sjf::helpers::functions::utilities::configurationAvailable<delay_config::PingPong, Configurations...>;
    static constexpr auto hasFilter = sjf::helpers::functions::utilities::configurationAvailable<delay_config::Filter, Configurations...>;
    static constexpr auto hasDetune = sjf::helpers::functions::utilities::configurationAvailable<delay_config::Detune, Configurations...>;
    static constexpr auto hasTempoSync = sjf::helpers::functions::utilities::configurationAvailable<delay_config::TempoSync, Configurations...>;
    static constexpr auto hasLink = sjf::helpers::functions::utilities::configurationAvailable<delay_config::Link, Configurations...>;

    static constexpr auto hasModulation = sjf::helpers::functions::utilities::has_any_instantiation<sjf::dsp::oscillators::lfo::LFO, Configurations...>;
    using LFO = sjf::helpers::functions::utilities::find_instantiation_of_t<
                                                                                sjf::dsp::oscillators::lfo::LFO,
                                                                                helpers::functions::utilities::DummyStruct,
                                                                                Configurations...
                                                                            >;

    static constexpr auto hasSaturation = sjf::helpers::functions::utilities::has_any_instantiation<sjf::dsp::waveshaper::WaveshaperTypeProvider, Configurations...>;
    using Saturation = sjf::helpers::functions::utilities::find_instantiation_of_t <
                                                                                        sjf::dsp::waveshaper::WaveshaperTypeProvider,
                                                                                        sjf::dsp::waveshaper::WaveshaperTypeProvider<sjf::dsp::waveshaper::None>,
                                                                                        Configurations...
                                                                                    >;

    using Filter = sjf::helpers::BypassWrapper<sjf::dsp::SVF<true, true>, helpers::bypass_wrapper_config::Bypass>;
    using DelayLine = std::conditional_t<hasDetune, sjf::helpers::PitchShiftDelayLine<>, sjf::helpers::DelayLine>;

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
        using Duration = helpers::SyncedDurationParameter<>;
        std::array<FloatState, NUM_CHANNELS> detune;
        FloatState feedback, drive;
        std::array<Duration, NUM_CHANNELS> delayTimes{Duration{1.0f, 10000.0f, 100.0f, 1000.0f},
                                                      Duration(1.0f, 10000.0f, 100.0f, 1000.0f)};


        BoolState link, pingPong, saturation;
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
                    if constexpr (hasTempoSync)
                    {
                        factory->addChild(delayTime.createParameters("Time" + str.substring(0, 1), "Time " + str));
                        addTrackedChildParameters(delayTime);
                    }
                    else
                    {
                        const auto mapping = [this](const float x) { return  x * spec.sampleRate * 0.001f;};
                        createTrackedParameter(*factory, delayTime.time, "Time" + str.substring(0, 1),  "Time " + str, delayTime.getDurationRange(), delayTime.defaultTimeMS, mapping, Duration::getDurationAttributes());
                    }
                }
            }

            // Detune
            if constexpr (hasDetune)
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
            if constexpr (hasLink)
            {
                if constexpr (NUM_CHANNELS > 1)
                    createTrackedParameter(*factory, link, "Link", "Link", false);
                else
                    link.currentValue = true;
            }

            // Ping Pong
            if constexpr (hasPingPong)
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
            if constexpr (hasSaturation)
            {
                createTrackedParameter(*factory, saturation, "Saturation",  "Saturation",  false);

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
    {
        static_assert(NUM_CHANNELS > 1 || !hasPingPong, "Can't have PingPong with a mono delay");
        static_assert(NUM_CHANNELS > 1 || !hasLink, "Can't Link timings with a mono delay");
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

        dcBlocker.prepare(spec);

        if constexpr (hasFilter)
            filter.prepare(spec);

        if constexpr (hasModulation)
            lfo.prepare(spec);

        if constexpr (hasSaturation)
            saturation.prepare(spec);


        reset();
    }

    void reset()
    {
        parameters.reset();
        for (auto& dl : delayLine)
            dl.reset();

        dcBlocker.reset();

        if constexpr (hasFilter)
            filter.reset();

        if constexpr (hasModulation)
            lfo.reset();

        if constexpr (hasSaturation)
        {
            lastSaturationType = parameters.saturationType.currentValue;
            saturation.reset();
        }
    }

    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
        parameters.checkForStateChange();

        if constexpr (hasSaturation)
        {
            const auto currentSaturationType = static_cast<size_t>(parameters.saturationType.currentValue);
            if (currentSaturationType != lastSaturationType)
                saturation.reset();
            lastSaturationType = currentSaturationType;

            if (parameters.saturation.currentValue)
                dispatch<true>(currentSaturationType, std::make_index_sequence<Saturation::numSaturators>{}, context);
            else
                dispatch<false>(currentSaturationType, std::make_index_sequence<Saturation::numSaturators>{}, context);
        }
        else
        {
            dispatch<false>(0, std::make_index_sequence<Saturation::numSaturators>{}, context);
        }

        dcBlocker.process(context);
    }

    std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName)
    {
        auto factory = parameters.createParameters (factoryID, factoryName);

        if constexpr (hasFilter)
            factory->addChild(filter.createParameters("Filter", "Filter"));

        factory->addChild(dcBlocker.createParameters("DCBlock", "DCBlock"));

        if constexpr (hasModulation)
            factory->addChild(lfo.createParameters("Mod", "Modulation "));

        return factory;
    }

    void setPositionInfo(const Optional<juce::AudioPlayHead::PositionInfo>& positionInfo)
    {
        parameters.setPositionInfo(positionInfo);
    }

private:

    template <bool SaturationActive, std::size_t... Indices, typename ProcessContext>
    void dispatch (const size_t targetIndex, std::index_sequence<Indices...>, const ProcessContext& context) noexcept
    {
        (void)((targetIndex == static_cast<int>(Indices) ? (processInternal<Indices, SaturationActive>(context), true) : false) || ...);
    }

    template <int SaturationIndex, bool SaturationActive, typename ProcessContext>
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

        const auto pingPong = getPingPongActive();

        const auto link = getLinkActive();

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


        const auto lfoBlock = getLFOBlock(context);

        for (size_t i = 0; i < numSamples; ++i)
        {
            // Remember to tick the smoothers!!! for every sample
            parameters.tickSmoothers();

            const auto feedback = parameters.feedback.currentValue;
            const auto drive = getDrive<SaturationActive>();
            const auto norm = drive > 0.0f ? 1.0f / saturation.template processSample<SaturationIndex, SaturationActive>(drive) : 1.0f;

            for (auto channel = 0ul; channel < numChannels; ++channel)
            {
                auto& sample = wptrs[channel][0];
                auto delayTime = link ? parameters.delayTimes[0].time.currentValue : parameters.delayTimes[channel].time.currentValue;
                delayTime += delayTime * getLFOSample(static_cast<int>(channel), static_cast<int>(i), lfoBlock);
                if constexpr (hasDetune)
                {
                    const auto detune = parameters.detune[channel].currentValue;
                    delayLine[channel].setPitchShift(detune);
                }
                sample = delayLine[channel].template readSample<sjf::interpolation::InterpolatorTypes::cubic>(delayTime);
                sample = saturation.template processSample<SaturationIndex, SaturationActive>(sample * drive) * norm;
            }

            if constexpr (hasFilter)
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

    template <typename ProcessContext>
    juce::dsp::AudioBlock<float> getLFOBlock(const ProcessContext& context)
    {
        if constexpr (hasModulation)
        {
            const auto numSamples = context.getOutputBlock().getNumSamples();
            lfo.process(context);
            return lfo.getLfoOutput().getSubBlock(0, numSamples);
        }
        else
        {
            return juce::dsp::AudioBlock<float>{};
        }
    }

    float getLFOSample(const int channel, const int i, const juce::dsp::AudioBlock<float>& lfoBlock)
    {
        if constexpr (hasModulation)
        {
            if constexpr ( LFO::numChannels == 2)
                return lfoBlock.getSample(static_cast<int>(channel), static_cast<int>(i));
            else
                return lfoBlock.getSample(0, static_cast<int>(i));
        }
        else
        {
            return 0.0f;
        }
    }


    bool getPingPongActive()
    {
        if constexpr (hasPingPong)
            return parameters.pingPong.currentValue;
        else
            return false;
    }

    bool getLinkActive()
    {
        if constexpr (hasLink)
            return parameters.link.currentValue;
        else
            return false;
    }

    template<bool SaturationActive>
    float getDrive()
    {
        if constexpr (hasSaturation && SaturationActive)
            return parameters.drive.currentValue;
        else
            return 1.0f;
    }

    juce::dsp::ProcessSpec spec{};
    std::array<DelayLine, NUM_CHANNELS> delayLine;
    [[maybe_unused]] LFO lfo;
    sjf::helpers::ProcessorDuplicator<helpers::DCBlocker<false>> dcBlocker;
    [[maybe_unused]] Filter filter;
    [[maybe_unused]] Saturation saturation;
    std::array<const float*, NUM_CHANNELS> inputChannelPointers;
    std::array<float*, NUM_CHANNELS> outputChannelPointers;
    int lastSaturationType = -1;
};
}
