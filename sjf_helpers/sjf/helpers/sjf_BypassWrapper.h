//
// Created by Simon Fay on 13/07/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_ParameterFactory.h>

namespace sjf::helpers
{

template <typename Processor>
class BypassWrapper
{
public:
    BypassWrapper() = default;
    ~BypassWrapper() = default;

    /**
        Nested Parameters class matching the DummyProcessor structural layout.
    */
    struct Parameters : public helpers::AudioParametersBase
    {
        FloatState mix;
        BoolState bypass;

        std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String&, const juce::String&) override
        {
            if (targetFactory)
            {
                /// we insert the parameters into the processors parameter tree,
                /// BUT The BypassWrapper::Parameters struct handles smoothing/reset etc the

                {
                    const auto range = juce::NormalisableRange<float>{ 0.0f, 100.0f, 0.01f };
                    const auto attributes = juce::AudioParameterFloatAttributes{}
                                                                                    .withLabel("%");
                    const auto mapping = [&](const float x){ return x * 0.01f;};
                    createTrackedParameter (*targetFactory, mix, "Mix", "Mix", range, 100.0f, mapping, attributes);
                }
                createTrackedParameter (*targetFactory, bypass, "Bypass", "Bypass", false);
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
    };

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
        if (parameters.bypass.currentValue)
        {
            dryRamp.setCurrentAndTargetValue(1.0f);
            wetRamp.setCurrentAndTargetValue(0.0f);
        }
        else
        {
            auto dry = sqrt(1.0f - parameters.mix.currentValue);
            auto wet = sqrt(parameters.mix.currentValue);
            dryRamp.setCurrentAndTargetValue(dry);
            wetRamp.setCurrentAndTargetValue(wet);
        }
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
            if (parameters.bypass.currentValue)
            {
                wetRamp.setTargetValue(0.0f);
                dryRamp.setTargetValue(1.0f);
            }
            else
            {
                wetRamp.setTargetValue(sqrt(parameters.mix.currentValue));
                dryRamp.setTargetValue(sqrt(1.0f - parameters.mix.currentValue));
            }
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

private:
    juce::dsp::ProcessSpec spec{};
    Processor processor;
    Parameters parameters;

    juce::AudioBuffer<float> inputBuffer;
    juce::LinearSmoothedValue<float> wetRamp, dryRamp;
};

} // namespace sjf::helpers