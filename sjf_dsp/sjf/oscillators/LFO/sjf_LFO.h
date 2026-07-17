//
// Created by Simon Fay on 15/07/2026.
//
#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_ParameterFactory.h>

#include "sjf/oscillators/sjf_Phasor.h"
#include <sjf/oscillators/LFO/sjf_LFOWaveformProvider.h>

namespace sjf::dsp::oscillators::lfo
{

/**
 * @brief Configuration tags used to customise the features of sjf::dsp::oscillators::lfo::LFO at compile time.
 *
 * Passing these tags into the LFO template parameter pack selectively enables parameters,
 * smoothing filters, phase offsets, and channel routing options with zero runtime overhead.
 */
namespace lfo_config
{
    /** @brief Configuration tag enabling a Phase Offset parameter (-180º to 180º) between LFO channels. */
    struct PhaseOffset{};

    /** @brief Configuration tag enabling an Invert choice parameter to invert the output phase of the left or right channel. */
    struct Invert{};

    /** @brief Configuration tag enabling an internal low-pass filter to smooth out sudden jumps or discontinuities in the LFO signal. */
    struct Smooth{};

    /** @brief Configuration tag enabling an adjustable Depth parameter (0% to 100%) to scale the LFO output amplitude. */
    struct Depth{};

    /** @brief Configuration tag enabling DAW tempo synchronization features for the LFO frequency. */
    struct TempoSync{};
}

/**
 * @brief A highly configurable, compile-time optimized Low Frequency Oscillator (LFO) for audio processing.
 *
 * The `LFO` class utilizes C++ variadic templates to compile only the requested feature sets, parameters,
 * and internal variables specified via `lfo_config` tags. Unconfigured capabilities are completely
 * scrubbed from runtime execution.
 *
 * ### LFOWaveformProvider Integration
 * To functionalize, the `LFO` requires an instance of an `LFOWaveformProvider` (such as `DefaultWaveformProvider`)
 * inside its template arguments. The class automatically discovers this type using meta-programming.
 * * If only a single waveform is included in the provider, the selection UI parameter is compiled out entirely.
 * * If multiple waveforms are provided, an automated runtime selection parameter is exposed, executing optimized
 *   compile-time dispatch loops via `processSample<Index>()`.
 *
 * ### Example Configuration:
 * @code
 * // Configure a stereo LFO featuring variable depth and phase offsetting
 * using MyLFO = sjf::dsp::oscillators::lfo::LFO<
 *     sjf::dsp::oscillators::lfo::DefaultWaveformProvider,
 *     sjf::dsp::oscillators::lfo::lfo_config::Depth,
 *     sjf::dsp::oscillators::lfo::lfo_config::PhaseOffset
 * >;
 *
 * MyLFO lfo;
 * @endcode
 *
 * @tparam Configurations A variadic pack consisting of `lfo_config` tags and a single `LFOWaveformProvider` type.
 */
template<typename... Configurations>
class LFO
{
public:
    using WaveformProvider = sjf::helpers::functions::utilities::find_instantiation_of_t<
                                                                                sjf::dsp::oscillators::lfo::LFOWaveformProvider,
                                                                                helpers::functions::utilities::DummyStruct,
                                                                                Configurations...
                                                                            >;

    static constexpr auto hasDepth = helpers::functions::utilities::configurationAvailable<lfo_config::Depth, Configurations...>;
    static constexpr auto hasPhaseOffset = helpers::functions::utilities::configurationAvailable<lfo_config::PhaseOffset, Configurations...>;
    static constexpr auto hasInvert = helpers::functions::utilities::configurationAvailable<lfo_config::Invert, Configurations...>;
    static constexpr auto hasSmooth = helpers::functions::utilities::configurationAvailable<lfo_config::Smooth, Configurations...>;
    static constexpr auto hasTempoSync = helpers::functions::utilities::configurationAvailable<lfo_config::TempoSync, Configurations...>;
    static constexpr auto numChannels = hasPhaseOffset || hasInvert ? 2 : 1;
    static constexpr auto numWaveforms = WaveformProvider::numWaveforms;
    static constexpr auto hasWaveformChoice = numWaveforms > 1;

    using Filters = std::array<juce::dsp::IIR::Filter<float>, numChannels>;

    struct Parameters : public helpers::AudioParametersBase
    {

        using FrequencyParam = helpers::SyncedFrequencyParameter<>;
        FloatState freq;
        [[maybe_unused]] FloatState depth, phaseOffset;

        BoolState sync;

        [[maybe_unused]] ChoiceState waveform, invert;
        [[maybe_unused]] ChoiceState syncedNumerator, syncedDenominator;

        FrequencyParam frequency;



        explicit Parameters(const float minFrequency_ = 0.001f, const float maxFrequency_ = 20.0f, const float defaultFrequency_ = 1.0f)
        : frequency(freq, sync, syncedNumerator, syncedDenominator, FrequencyParam::makeFrequencyRange(minFrequency_, maxFrequency_), defaultFrequency_)
        {}

        std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) override
        {
            static_assert(numWaveforms > 0, "There must be at least one waveform!!!");

            auto factory = helpers::ParameterFactory::create (factoryID, factoryName);

            if constexpr (hasTempoSync)
            {
                factory->addChild(frequency.createParameters("Frequency" , "Frequency"));
                addTrackedChildParameters(frequency);
            }
            else
            {
                createTrackedParameter(*factory, frequency.frequency, "Frequency", "Frequency", frequency.frequencyRange, frequency.defaultFrequency, nullptr, FrequencyParam::getFrequencyAttributes());
            }

            if constexpr (hasWaveformChoice)
                createTrackedParameter(*factory, waveform, "Waveform", "Waveform", WaveformProvider::getNames(), 0);

            if constexpr (hasDepth)
            {
                auto range = juce::NormalisableRange<float>(0.0f, 100.0f, 0.01f);
                auto mapping = [](const float x){
                    return x * 0.01f;
                };
                const auto attributes = AudioParameterFloatAttributes().withLabel("%");
                createTrackedParameter(*factory, depth, "Depth", "Depth", range, 0.0f, mapping, attributes);
            }
            if constexpr (hasPhaseOffset)
            {
                static const auto degreeString = juce::CharPointer_UTF8 ("\xc2\xba");
                auto range = juce::NormalisableRange<float>(-180.0f, 180.0f, 0.01f);
                range.setSkewForCentre(0.0f);
                auto mapping = [](const float x){
                    using namespace sjf::helpers::functions::waveforms;
                    return wrapPhase( 1.0f + x/360.0f);
                };
                const auto attributes = AudioParameterFloatAttributes().withLabel(degreeString);
                createTrackedParameter(*factory, phaseOffset, "PhaseOffset", "Phase Offset", range, 0.0f, mapping, attributes);
            }
            if constexpr (hasInvert)
            {
                createTrackedParameter(*factory, invert, "InvertRight", "Invert Right", {"None", "Left", "Right"}, 0);
            }

            return factory;
        }
    } parameters;


    explicit LFO(const float minFrequency_ = 0.001f, const float maxFrequency_ = 20.0f, const float defaultFrequency_ = 1.0f)
    : parameters(minFrequency_, maxFrequency_, defaultFrequency_)
    {
        static_assert(!std::is_same_v<helpers::functions::utilities::DummyStruct, LFO>, "You have not included an instance of LFOWaveformProvider in the template arguments");
    }

    void prepare (const juce::dsp::ProcessSpec& spec_)
    {
        spec = spec_;
        spec.numChannels = static_cast<size_t>(numChannels);
        parameters.prepare(spec);
        waveformProvider.prepare(spec);
        phasor.prepare(spec);
        lfoOutput.setSize(numChannels, static_cast<int>(spec.maximumBlockSize));
        if constexpr (hasSmooth)
        {
            for (auto& filter : smoothingFilter)
            {
                filter.prepare(spec);
                filter.coefficients = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(spec.sampleRate, jmin(static_cast<float>(spec.sampleRate * 0.4999), parameters.frequency.frequencyRange.end * 2.5f));
            }
        }
        reset();
    }

    void reset()
    {
        parameters.reset();
        waveformProvider.reset();
        lastWaveform = static_cast<size_t>(parameters.waveform.currentValue);
        phasor.setFrequency(parameters.frequency.frequency.currentValue);
        phasor.reset();
        if constexpr (hasSmooth)
            for (auto& filter : smoothingFilter)
                filter.reset();
        lfoOutput.clear();
    }

    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
        if constexpr (hasWaveformChoice)
        {
            const auto currentWaveform = static_cast<size_t>(parameters.waveform.currentValue);
            if (currentWaveform != lastWaveform)
                waveformProvider.reset();
            lastWaveform = currentWaveform;

            dispatch(currentWaveform, std::make_index_sequence<numWaveforms>{}, context);
        }
        else
        {
            dispatch(0, std::make_index_sequence<numWaveforms>{}, context);
        }

        if constexpr (hasInvert)
        {
            if (parameters.invert.currentValue > 0)
                lfoOutput.applyGain(parameters.invert.currentValue-1, 0, static_cast<int>(context.getOutputBlock().getNumSamples()), -1.0f);
        }

        if constexpr(hasSmooth)
        {
            for ( auto i = 0ul; i < smoothingFilter.size(); ++i)
            {
                auto& filter = smoothingFilter[i];
                auto channelBlock = getLfoOutput().getSingleChannelBlock(i);
                const juce::dsp::ProcessContextReplacing<float> filterContext{channelBlock};
                filter.process(filterContext);
            }
        }
    }

    juce::dsp::AudioBlock<float> getLfoOutput()
    {
        return juce::dsp::AudioBlock<float>{lfoOutput};
    }

    std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName)
    {
        return parameters.createParameters (factoryID, factoryName);
    }

private:
    forcedinline float getPhaseOffsetValue() const noexcept
    {
        if constexpr (hasPhaseOffset)
            return parameters.phaseOffset.currentValue;
        else
            return 0.0f;
    }

    forcedinline float getDepthValue() const noexcept
    {
        if constexpr (hasDepth)
            return parameters.depth.currentValue;
        else
            return 1.0f;
    }



    template <std::size_t... Indices, typename ProcessContext>
    void dispatch (const size_t targetIndex, std::index_sequence<Indices...>, const ProcessContext& context) noexcept
    {
        (void)((targetIndex == static_cast<size_t>(Indices) ? (executeProcessWithIndex<Indices>(context), true) : false) || ...);
    }

    template<size_t WaveformIndex, typename ProcessContext>
    void executeProcessWithIndex(const ProcessContext& context) noexcept
    {
        if (parameters.checkForStateChange())
        {
            processSmoothedState<WaveformIndex>(context);
        }
        else
        {
            processStaticState<WaveformIndex>(context);
        }
    }

    template <size_t WaveformIndex, typename ProcessContext>
    void processStaticState (const ProcessContext& context) noexcept
    {
        const auto& inputBlock = context.getInputBlock();
        auto& outputBlock      = context.getOutputBlock();
        const auto numSamples  = outputBlock.getNumSamples();

        jassert (inputBlock.getNumSamples() == numSamples);

        auto wptrs = lfoOutput.getArrayOfWritePointers();

        auto getOffsetPhase = [phaseOffset = getPhaseOffsetValue()](const size_t channel, const float phase){
            if constexpr (hasPhaseOffset)
                return sjf::helpers::functions::waveforms::wrapPhase(phase, static_cast<float>(channel) * phaseOffset );
            else
                return phase;
        };

        phasor.setFrequency(parameters.frequency.frequency.currentValue);


        for (size_t i = 0; i < numSamples; ++i)
        {
            const auto phase = phasor.process();
            for ( auto channel = 0ul; channel < numChannels; ++channel)
            {
                // per channel changes if necessary
                const auto offsetPhase = getOffsetPhase(channel, phase);
                wptrs[channel][i] = waveformProvider.template processSample<WaveformIndex>(offsetPhase);
            }
        }

        if constexpr(hasDepth)
            lfoOutput.applyGain(getDepthValue());

    }

    template <size_t WaveformIndex, typename ProcessContext>
    void processSmoothedState (const ProcessContext& context) noexcept
    {

        const auto& inputBlock = context.getInputBlock();
        auto& outputBlock      = context.getOutputBlock();
        const auto numSamples  = outputBlock.getNumSamples();

        jassert (inputBlock.getNumSamples() == numSamples);

        auto wptrs = lfoOutput.getArrayOfWritePointers();

        auto getOffsetPhase = [](const size_t channel, const float phase, const float phaseOffset){
            if constexpr (hasPhaseOffset)
                return sjf::helpers::functions::waveforms::wrapPhase(phase, static_cast<float>(channel) * phaseOffset );
            else
                return phase;
        };

        for (size_t i = 0; i < numSamples; ++i)
        {
            // Remember to tick the smoothers!!! for every sample
            parameters.tickSmoothers();

            phasor.setFrequency(parameters.frequency.frequency.currentValue);
            const auto phase = phasor.process();
            const auto phaseOffset = getPhaseOffsetValue();
            const auto depth = getDepthValue();
            for ( auto channel = 0ul; channel < numChannels; ++channel)
            {
                // per channel changes if necessary
                const auto offsetPhase = getOffsetPhase(channel, phase, phaseOffset);
                wptrs[channel][i] = depth * waveformProvider.template processSample<WaveformIndex>(offsetPhase);
            }
        }
    }

    juce::AudioBuffer<float> lfoOutput;

    juce::dsp::ProcessSpec spec{};
    WaveformProvider waveformProvider;
    sjf::dsp::oscillators::Phasor phasor;
    Filters smoothingFilter;
    size_t lastWaveform = 0;
};

}
