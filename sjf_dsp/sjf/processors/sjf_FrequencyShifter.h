/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 27/10/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_ParameterFactory.h>

#include <sjf/helpers/sjf_Hilbert.h>

#include <sjf/oscillators/sjf_SinCos.h>

namespace sjf::dsp{


class FrequencyShifter
{
public:
    struct Parameters : public helpers::AudioParametersBase
    {
        FloatState  shift, spread;

        std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) override
        {
            auto factory = helpers::ParameterFactory::create (factoryID, factoryName);
            const auto att = juce::AudioParameterFloatAttributes{}.withLabel("Hz");

            auto range = juce::NormalisableRange<float>{-10000.0f, 10000.0f, 0.001f};
            range.skew = 0.25f;
            range.symmetricSkew = true;
            createTrackedParameter          (*factory, shift, "Shift",  "Frequency Shift",  range, 0.0f, {}, att);
            createTrackedDegreeParameter    (*factory, spread, "Spread", "Spread", -180.0f, 180.0f, 0.0f, 0.0f, {});

            return factory;
        }
    } parameters;


    void prepare (const juce::dsp::ProcessSpec& spec_)
    {
        spec = spec_;
        parameters.prepare(spec);
        hilbert.prepare(spec);
        modulator.prepare(spec);
        modulatorOutput.setSize(2, static_cast<int>(spec.maximumBlockSize));

        shiftedModulatorOutputs.resize(spec.numChannels);
        for (auto& out : shiftedModulatorOutputs)
            out.setSize(2, static_cast<int>(spec.maximumBlockSize));

        tempBuffer.setSize(1, static_cast<int>(spec.maximumBlockSize));

        reset();
    }

    void reset()
    {
        parameters.reset();
        hilbert.reset();
        modulator.reset();
        modulatorOutput.clear();
    }

    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
        [[maybe_unused]] const auto& inputBlock = context.getInputBlock();
        [[maybe_unused]] auto& outputBlock      = context.getOutputBlock();
        [[maybe_unused]] const auto numChannels = outputBlock.getNumChannels();
        [[maybe_unused]] const auto numSamples  = outputBlock.getNumSamples();

        jassert (inputBlock.getNumChannels() == numChannels);
        jassert (inputBlock.getNumSamples() == numSamples);

        hilbert.process(context);

        auto sigRealBlock = hilbert.getRealBlock().getSubBlock(0, numSamples);
        auto sigImaginaryBlock = hilbert.getImaginaryBlock().getSubBlock(0, numSamples);



        if (JUCE_LIVE_CONSTANT(false))
        {
            outputBlock.getSingleChannelBlock(0).copyFrom(sigRealBlock.getSingleChannelBlock(0));
            outputBlock.getSingleChannelBlock(1).copyFrom(sigImaginaryBlock.getSingleChannelBlock(0));
            return;
        }

        const auto spreadBefore = parameters.spread.currentValue;

        if (parameters.checkForStateChange())
        {
            for (auto i = 0; i < static_cast<int>(numSamples); i++)
            {
                parameters.tickSmoothers();

                modulator.setFrequency(parameters.shift.currentValue);

                const auto [cosOut, sinOut] = modulator();
                modulatorOutput.setSample(0, i, cosOut);
                modulatorOutput.setSample(1, i, sinOut);
            }
        }
        else
        {
            modulator.setFrequency(parameters.shift.currentValue);
            for (auto i = 0; i < static_cast<int>(numSamples); i++)
            {
                const auto [cosOut, sinOut] = modulator();
                modulatorOutput.setSample(0, i, cosOut);
                modulatorOutput.setSample(1, i, sinOut);
            }
        }

        const auto spreadAfter = parameters.spread.currentValue;

        auto modBlock = juce::dsp::AudioBlock<float>{modulatorOutput}.getSubBlock(0, numSamples);
        auto modRealBlock =  modBlock.getSingleChannelBlock(0);
        auto modImaginaryBlock = modBlock.getSingleChannelBlock(1);

        for (auto i = 0ul; i < numChannels; ++i)
        {
            auto getAngVals = [numChannels, i](const float spread) {
                auto v = numChannels <= 1 ? 0.0f : static_cast<float>(i) / static_cast<float>(numChannels - 1);
                auto angle = juce::jmap(v, -spread, spread);
                angle /= 360.0f;
                angle *= juce::MathConstants<float>::pi;
                return std::pair{ std::cos(angle), std::sin(angle) };
            };

            const auto before = getAngVals(spreadBefore);
            const auto after  = getAngVals(spreadAfter);

            auto modChannelBlock = juce::dsp::AudioBlock<float>{ shiftedModulatorOutputs[i] }.getSubBlock(0, numSamples);
            auto modChannelRealBlock = modChannelBlock.getSingleChannelBlock(0);
            auto modChannelImaginaryBlock = modChannelBlock.getSingleChannelBlock(1);

            auto tempBlock = juce::dsp::AudioBlock<float>{ tempBuffer }.getSubBlock(0, numSamples);

            // --- 1. Real Channel: cos(theta)*Real - sin(theta)*Imag ---
            modChannelRealBlock.copyFrom(modRealBlock);
            tempBuffer.applyGainRamp(0, static_cast<int>(numSamples), before.first, after.first); // Ramp Cosine

            tempBlock.copyFrom(modImaginaryBlock);
            tempBuffer.applyGainRamp(0, static_cast<int>(numSamples), -before.second, -after.second); // Ramp -Sine
            modChannelRealBlock.add(tempBlock);

            // --- 2. Imaginary Channel: cos(theta)*Imag + sin(theta)*Real ---
            modChannelImaginaryBlock.copyFrom(modImaginaryBlock);
            tempBuffer.applyGainRamp(0, static_cast<int>(numSamples), before.first, after.first); // Ramp Cosine

            tempBlock.copyFrom(modRealBlock);
            tempBuffer.applyGainRamp(0, static_cast<int>(numSamples), before.second, after.second); // Ramp +Sine
            modChannelImaginaryBlock.add(tempBlock);

            sigRealBlock.getSingleChannelBlock(i).multiplyBy(modChannelRealBlock);
            sigImaginaryBlock.getSingleChannelBlock(i).multiplyBy(modChannelImaginaryBlock);
        }

        outputBlock.copyFrom(sigRealBlock);
        outputBlock.subtract(sigImaginaryBlock);
    }

    std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName)
    {
        return parameters.createParameters (factoryID, factoryName);
    }

private:

    juce::dsp::ProcessSpec spec{};
    sjf::helpers::Hilbert hilbert;
    sjf::dsp::oscillators::SinCos modulator;
    juce::AudioBuffer<float> modulatorOutput;
    std::vector<juce::AudioBuffer<float>> shiftedModulatorOutputs;

    juce::AudioBuffer<float> tempBuffer;

};

}


