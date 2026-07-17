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
/**
 * @brief Configuration tags used to customise the features of sjf::dsp::Delay at compile time.
 *
 * Passing these tags into the Delay template parameter pack selectively enables parameters,
 * internal processors, and DSP routing options with zero runtime overhead.
 */
namespace delay_config
{
    /** @brief Forces the delay processor to run in single-channel mono mode. */
    struct Mono{};

    /** @brief Enables independent delay time offsets (percentage-based scaling) per channel. */
    struct Offset{};

    /** @brief Enables a feedback path for repeating echoes, complete with a smooth parameter control. */
    struct Feedback{};

    /** @brief Enables ping-pong style stereo echo routing (requires stereo mode). */
    struct PingPong{};

    /** @brief Inserts an internal State Variable Filter (SVF) directly into the feedback loop. */
    struct Filter{};

    /** @brief Enables pitch-shifting delay lines for micro-tonal pitch detuning per channel. */
    struct Detune{};

    /** @brief Switches the delay time controls to synchronise with the host tempo (musical grid). */
    struct TempoSync{};

    /** @brief Enables a parameter to link the delay timings of both stereo channels together. */
    struct Link{};

    /** @brief Enables a overriding of the default min, max, default, and centre values for delay time. */
    template<size_t minMS = 1, size_t maxMS = 10000,  size_t defaultMS = 500, size_t skew = 1000>
    struct TimeValues
    {
        static constexpr float minTimeMS_       = minMS;
        static constexpr float maxTimeMS_       = maxMS;
        static constexpr float defaultTimeMS_   = defaultMS;
        static constexpr float skewForCentre_   = skew;
    };
}

/**
 * @brief A highly flexible, compile-time optimized stereo/mono delay line processor.
 *
 * The `Delay` class utilizes modern template metaprogramming techniques to parse features
 * directly out of its variadic parameter pack. It accepts structural configuration flags from the
 * `delay_config` namespace alongside fully instantiated processing engines.
 *
 * ### Advanced Ecosystem Component Resolution:
 *
 * 1. **Dynamic LFO Modulation Engine Detection:**
 *    Instead of relying on a simplified boolean flag layout, this class introspects its template
 *    parameters using `has_any_instantiation`. If any configured instance of **sjf::dsp::oscillators::lfo::LFO**
 *    is found inside the template parameters, `hasModulation` evaluates to `true`. The delay line will then
 *    extract the precise type mapping automatically to instantiate the underlying LFO engine. This engine
 *    modulates the fractional delay time pointers on a per-sample basis for chorus, flanging, or vibrato effects.
 *
 * 2. **Dynamic Feedback Loop Saturation Detection:**
 *    The class searches the parameter pack for an instantiation of the **sjf::dsp::waveshaper::WaveshaperTypeProvider**
 *    container. If discovered, `hasSaturation` evaluates to `true` and the type map is drawn out. This integrates the
 *    waveshaper provider's multi-algorithmic clipping/saturation layers directly into the delay lines feedback accumulation path.
 *    If missing, it safe-defaults to a clean `WaveshaperTypeProvider<sjf::dsp::waveshaper::None>` profile.
 *
 * ### Example Configurations:
 * @code
 * // Example 1: High-performance, clean stereo digital delay (No LFO or Saturation)
 * using DigitalDelay = sjf::dsp::Delay<
 *     sjf::dsp::delay_config::Feedback,
 *     sjf::dsp::delay_config::Filter
 * >;
 *
 * // Example 2: Vintage modulated tape echo featuring internal LFO and warm saturation choices
 * using CustomLFO = sjf::dsp::oscillators::lfo::LFO<
 *     sjf::dsp::oscillators::lfo::DefaultWaveformProvider,
 *     sjf::dsp::oscillators::lfo::lfo_config::PhaseOffset
 * >;
 * using CustomSaturator = sjf::dsp::waveshaper::WaveshaperTypeProvider<Tape, SoftClip>;
 *
 * using ModulatedTapeEcho = sjf::dsp::Delay<
 *     sjf::dsp::delay_config::Feedback,
 *     sjf::dsp::delay_config::PingPong,
 *     CustomLFO,       // Instantiated LFO config type activates modulation
 *     CustomSaturator  // Instantiated Waveshaper type activates saturation options
 * >;
 * @endcode
 *
 * @tparam Configurations A variadic parameter pack containing `delay_config` tags, an optional
 *                        `sjf::dsp::oscillators::lfo::LFO` template type instantiation, and an optional
 *                        `sjf::dsp::waveshaper::WaveshaperTypeProvider` template type instantiation.
 */
template<typename ... Configurations>
class Delay
{
public:
    static constexpr auto MAX_DELAY_MS = 10000.0f;


    static constexpr auto hasMono = sjf::helpers::functions::utilities::configurationAvailable<delay_config::Mono, Configurations...>;
    static constexpr auto NUM_CHANNELS = hasMono ? 1 : 2;



    static constexpr auto hasOffset = sjf::helpers::functions::utilities::configurationAvailable<delay_config::Offset, Configurations...>;
    static constexpr auto hasFeedback = sjf::helpers::functions::utilities::configurationAvailable<delay_config::Feedback, Configurations...>;
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

    using DefaultTimes = delay_config::TimeValues<>;
    using DelayTimes = sjf::helpers::functions::utilities::find_value_instantiation_of_t<
                                                                                             delay_config::TimeValues,
                                                                                             DefaultTimes,
                                                                                             Configurations...
                                                                                         >;

    struct Parameters : public helpers::AudioParametersBase
    {
        using Duration = helpers::SyncedDurationParameter<>;
        std::array<FloatState, NUM_CHANNELS> detunes;
        std::array<FloatState, NUM_CHANNELS> offsets;
        FloatState feedback, drive;
        std::array<Duration, NUM_CHANNELS> delayTimes;


        BoolState link, pingPong, saturation;
        ChoiceState saturationType;



        Parameters() :
        delayTimes(sjf::helpers::functions::utilities::makeFilledArray<Duration, NUM_CHANNELS>(DelayTimes::minTimeMS_, DelayTimes::maxTimeMS_, DelayTimes::defaultTimeMS_, DelayTimes::skewForCentre_))
        {}

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
                auto createTrackedOffsetParameter = [&](helpers::ParameterFactory& factoryToUse, size_t channel){
                    auto offsetRange = NormalisableRange<float>{-33.0f, 33.0f, 0.001f};
                    offsetRange.setSkewForCentre(0.0f);
                    auto offsetMapping = [&](const float x) {
                        return std::pow(2.0f, x * 0.01f);
                    };
                    const auto offsetAttributes = juce::AudioParameterFloatAttributes{}.withLabel("%");
                    createTrackedParameter(factoryToUse, offsets[channel], "Offset", "Offset", offsetRange, 0.0f, offsetMapping, offsetAttributes);
                };

                for (auto i = 0ul; i < NUM_CHANNELS; i++)
                {
                    auto& delayTime = delayTimes[i];
                    const auto& str = delayTimeStrings[i];

                    if constexpr (hasTempoSync)
                    {
                        auto delayFactory_ = delayTime.createParameters("Time" + str.substring(0, 1), "Time " + str);
                        auto& delayFactory = *delayFactory_;
                        factory->addChild(std::move(delayFactory_));

                        if constexpr (hasOffset)
                            createTrackedOffsetParameter(delayFactory, i);

                        addTrackedChildParameters(delayTime);
                    }
                    else
                    {
                        const auto mapping = [this](const float x) { return  x * spec.sampleRate * 0.001f;};
                        createTrackedParameter(*factory, delayTime.time, "Time" + str.substring(0, 1),  "Time " + str, delayTime.getDurationRange(), delayTime.defaultTimeMS, mapping, Duration::getDurationAttributes());
                        if constexpr (hasOffset)
                            createTrackedOffsetParameter(*factory, i);
                    }

                    // Detune
                    if constexpr (hasDetune)
                    {
                        auto& detune = detunes[i];
                        auto range = juce::NormalisableRange<float>{ -100.0f, 100.0f, 0.01f };
                        const auto attributes = juce::AudioParameterFloatAttributes{}   .withLabel("cents");
                        const auto mapping = [&](const float x){ return x * 0.01f;};
                        createTrackedParameter  (*factory, detune, "Detune" + str.substring(0, 1),  "Detune "+ str +" (cents)",  range, 0.0f, mapping, attributes);
                    }
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
            if constexpr (hasFeedback)
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

                createTrackedParameter(*factory, saturationType, "SaturationType", "Saturation Type", Saturation::getNames(), 0);
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
    : inputChannelPointers({})
    , outputChannelPointers({})
    {
        static_assert(NUM_CHANNELS > 1 || !hasPingPong, "Can't have PingPong with a mono delay");
        static_assert(NUM_CHANNELS > 1 || !hasLink, "Can't Link timings with a mono delay");
        static_assert((NUM_CHANNELS > 1 && hasLink) || hasTempoSync || !hasOffset, "Having offset when you only have time based values and one channel/no link doesn't make sense");
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
            lastSaturationType = static_cast<size_t>(parameters.saturationType.currentValue);
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
        (void)((targetIndex == static_cast<size_t>(Indices) ? (processInternal<Indices, SaturationActive>(context), true) : false) || ...);
    }

    template <size_t SaturationIndex, bool SaturationActive, typename ProcessContext>
    void processInternal (const ProcessContext& context) noexcept
    {
        const auto& inputBlock = context.getInputBlock();
        auto& outputBlock      = context.getOutputBlock();
        const auto numSamples  = outputBlock.getNumSamples();

        jassert (inputBlock.getNumChannels() == NUM_CHANNELS);
        jassert (outputBlock.getNumChannels() == NUM_CHANNELS);

        AudioBuffer<float> oneSampleBuffer(NUM_CHANNELS, 1);
        const auto wptrs = oneSampleBuffer.getArrayOfWritePointers();
        juce::dsp::AudioBlock<float> oneSampleBlock(oneSampleBuffer);
        const juce::dsp::ProcessContextReplacing<float> oneSampleContext(oneSampleBlock);


        for (auto channel = 0ul; channel < NUM_CHANNELS; ++channel)
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
            parameters.tickSmoothers();

            const auto feedback = getFeedback();
            const auto drive = getDrive<SaturationActive>();
            const auto norm = drive > 0.0f ? 1.0f / saturation.template processSample<SaturationIndex, SaturationActive>(drive) : 1.0f;

            for (auto channel = 0ul; channel < NUM_CHANNELS; ++channel)
            {
                auto& sample = wptrs[channel][0];
                auto delayTime = link ? parameters.delayTimes[0].time.currentValue : parameters.delayTimes[channel].time.currentValue;
                delayTime *= getDelayTimeOffset(channel);
                delayTime += delayTime * getLFOSample(static_cast<int>(channel), static_cast<int>(i), lfoBlock);
                if constexpr (hasDetune)
                {
                    const auto detune = parameters.detunes[channel].currentValue;
                    delayLine[channel].setPitchShift(detune);
                }
                sample = delayLine[channel].template readSample<sjf::interpolation::InterpolatorTypes::cubic>(delayTime);
                sample = saturation.template processSample<SaturationIndex, SaturationActive>(sample * drive) * norm;
            }

            if constexpr (hasFilter)
                filter.process(oneSampleContext);

            for (auto channel = 0ul; channel < NUM_CHANNELS; ++channel)
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

    float getDelayTimeOffset(const size_t channel)
    {
        if constexpr (hasOffset)
            return parameters.offsets[channel].currentValue;
        else
            return 1.0f;
    }

    float getFeedback()
    {
        if constexpr (hasFeedback)
            return parameters.feedback.currentValue;
        else
            return 0.0f;
    }

    juce::dsp::ProcessSpec spec{};
    std::array<DelayLine, NUM_CHANNELS> delayLine;
    [[maybe_unused]] LFO lfo;
    sjf::helpers::ProcessorDuplicator<helpers::DCBlocker<false>> dcBlocker;
    [[maybe_unused]] Filter filter;
    [[maybe_unused]] Saturation saturation;
    std::array<const float*, NUM_CHANNELS> inputChannelPointers;
    std::array<float*, NUM_CHANNELS> outputChannelPointers;
    size_t lastSaturationType = 0;
};
}
