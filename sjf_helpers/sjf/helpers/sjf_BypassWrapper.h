/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */

//
// Created by Simon Fay on 13/07/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_ParameterFactory.h>
#include "sjf_OptionalCalls.h"

namespace sjf::helpers
{
/**
 * @brief Configuration tags used to customise the behaviour and parameter set of sjf::helpers::BypassWrapper at compile time.
 *
 * Passing these tags into the BypassWrapper template parameter pack selectively enables parameters
 * and internal signal calculation logic (such as smooth dry/wet mixing, soft bypassing, or total muting)
 * with zero runtime overhead.
 */
namespace bypass_wrapper_config
{
    /** @brief Configuration tag enabling a soft-bypass control parameter with a click-free 50ms ramp transition. */
    struct Bypass{};

    /** @brief Configuration tag enabling an adjustable Dry/Wet Mix parameter using constant-power crossfading curves. */
    struct Mix{};

    /** @brief Configuration tag enabling a hard mute switch parameter with a 50ms gain ramp down. */
    struct Mute{};
}

/**
 * @brief A compile-time configurable wrapper that adds smooth bypassing, mixing, and muting capabilities to any DSP processor.
 *
 * The `BypassWrapper` wraps a raw mono or multi-channel DSP processor and overlays mixing controls
 * without modifying the wrapped class's core DSP code. By evaluating the template configurations,
 * it conditionally instantiates and updates target parameters (`Bypass`, `Mix`, `Mute`).
 *
 *
 * @tparam Processor The underlying DSP class to wrap (which must implement standard `prepare()`, `reset()`,
 *                   `process()`, and `createParameters()` interfaces).
 * @tparam Configs A variadic parameter pack consisting of configuration tags from `bypass_wrapper_config`.
 */
template <typename Processor, typename... Configs>
class BypassWrapper
{
    static constexpr auto hasBypass = helpers::functions::utilities::configurationAvailable<bypass_wrapper_config::Bypass, Configs...>;
    static constexpr auto hasMix = helpers::functions::utilities::configurationAvailable<bypass_wrapper_config::Mix, Configs...>;
    static constexpr auto hasMute = helpers::functions::utilities::configurationAvailable<bypass_wrapper_config::Mute, Configs...>;
public:
    BypassWrapper() = default;
    ~BypassWrapper() = default;

    /**
        Nested Parameters class matching the DummyProcessor structural layout.
    */
    struct Parameters : public helpers::AudioParametersBase
    {
        FloatState mix;
        BoolState bypass, mute;

        std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String&, const juce::String&) override
        {
            using namespace bypass_wrapper_config;
            if (targetFactory)
            {
                /// we insert the parameters into the processors parameter tree,
                /// BUT The BypassWrapper::Parameters struct handles smoothing/reset etc the

                if constexpr (hasMix)
                {
                    const auto range = juce::NormalisableRange<float>{ 0.0f, 100.0f, 0.01f };
                    const auto attributes = juce::AudioParameterFloatAttributes{}
                                                                                    .withLabel("%");
                    const auto mapping = [&](const float x){ return x * 0.01f;};
                    createTrackedParameter (*targetFactory, mix, "Mix", "Mix", range, 100.0f, mapping, attributes);
                }
                else
                {
                    mix.currentValue = 1.0f;
                }

                if constexpr (hasBypass)
                {
                    createTrackedParameter (*targetFactory, bypass, "Bypass", "Bypass", false);
                }
                else
                {
                    bypass.currentValue = false;
                }

                if constexpr (hasMute)
                {
                    createTrackedParameter (*targetFactory, mute, "Mute", "Mute", false);
                }
                else
                {
                    mute.currentValue = false;
                }
            }
            else
            {
                jassertfalse;
            }

            targetFactory = nullptr;

            return std::unique_ptr<helpers::ParameterFactory>{nullptr};
        }

        void setParameterFactory(ParameterFactory* factoryToUse)
        {
            targetFactory = factoryToUse;
        }

    private:
        ParameterFactory* targetFactory{nullptr};
    } parameters;

    //==============================================================================
    void prepare (const juce::dsp::ProcessSpec& spec_)
    {
        spec = spec_;
        processor.prepare (spec);
        parameters.prepare (spec);
        wetRamp.reset(spec.sampleRate, 0.05); // 50ms
        dryRamp.reset(spec.sampleRate, 0.05); // 50ms

        inputBuffer.setSize(static_cast<int>(spec.numChannels), static_cast<int>(spec.maximumBlockSize));
        reset();
    }

    void reset()
    {
        processor.reset();
        parameters.reset();
        dryRamp.setCurrentAndTargetValue(getDryTargetLevel());
        wetRamp.setCurrentAndTargetValue(getWetTargetLevel());
    }

    //==============================================================================
    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
        const auto& inputBlock = context.getInputBlock();
        auto& outputBlock      = context.getOutputBlock();

        if (context.isBypassed)
        {
            jassertfalse; // This should never happen! Top level processor needs to handle host setting bypass
        }

        if (!wetRamp.isSmoothing() && !dryRamp.isSmoothing())
        {
            parameters.reset(); // we only have bool parameter so we can snap and do our thing
            wetRamp.setTargetValue(getWetTargetLevel());
            dryRamp.setTargetValue(getDryTargetLevel());
        }

        if (wetRamp.getCurrentValue() == 0.0f)
        {
            outputBlock.copyFrom(inputBlock);
            dryRamp.skip(static_cast<int>(inputBlock.getNumSamples()));
            wetRamp.skip(static_cast<int>(inputBlock.getNumSamples()));
        }
        else
        {
            juce::dsp::AudioBlock<float> dryBlock(inputBuffer);
            dryBlock.copyFrom(inputBlock);
            processor.process (context);
            dryBlock.multiplyBy(dryRamp);
            outputBlock.multiplyBy(wetRamp);
            outputBlock.add(dryBlock);
        }
    }

    //==============================================================================
    /**
        Generates the internal processor's parameter factory, hands it to our local
        parameters object to append the bypass state, and cleans up the transient pointer.
    */
    template <typename... Args>
    std::unique_ptr<helpers::ParameterFactory> createParameters (
        const juce::String& factoryID,
        const juce::String& factoryName,
        Args&&... configArgs)
    {
        // 1. Ask the wrapped processor to generate its parameter factory layout first
        auto factory = processor.createParameters (factoryID, factoryName, std::forward<Args> (configArgs)...);

        if (factory == nullptr)
        {
            jassertfalse;
            return nullptr;
        }

        parameters.setParameterFactory (factory.get());

        parameters.createParameters (factoryID, factoryName);


        return factory;
    }

    //==============================================================================
    [[nodiscard]] Processor& getProcessor() noexcept { return processor; }
    [[nodiscard]] const Processor& getProcessor() const noexcept { return processor; }

    void setPositionInfo(const juce::AudioPlayHead::PositionInfo& positionInfo)
    {
        sjf::optional_calls::setPositionInfo(processor, positionInfo);
    }

	int getLatencySamples()
    {
    	return sjf::optional_calls::getLatencySamples(processor);
    }

private:

    float getWetTargetLevel()
    {
        if constexpr (hasMute)
            if (parameters.mute.currentValue)
                return 0.0f;

        if constexpr (hasBypass)
            if (parameters.bypass.currentValue)
                return 0.0f; // Bypassed = no wet signal

        if constexpr (hasMix)
            return std::sqrt (parameters.mix.currentValue);

        return 1.0f;
    }

    float getDryTargetLevel()
    {
        if constexpr (hasMute)
            if (parameters.mute.currentValue)
                return 0.0f;

        if constexpr (hasBypass)
            if (parameters.bypass.currentValue)
                return 1.0f;

        if constexpr (hasMix)
            return std::sqrt (1.0f - parameters.mix.currentValue);

        return 0.0f;
    }


    juce::dsp::ProcessSpec spec{};
    Processor processor;

    juce::AudioBuffer<float> inputBuffer;
    juce::LinearSmoothedValue<float> wetRamp, dryRamp;
};

} // namespace sjf::helpers