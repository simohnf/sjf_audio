/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */

//
// Created by Simon Fay on 15/07/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_ParameterFactory.h>

namespace sjf::helpers
{
/**
 * @brief A high-pass filter module designed to remove low-frequency DC offset transients from an audio signal.
 *
 *
 * @tparam FIRST_ORDER Configures the filter order. Defaults to `true` (first-order / 6 dB/octave high-pass).
 */
template<bool FIRST_ORDER = true> // second order if false
class DCBlocker
{
public:
    struct Parameters : public helpers::AudioParametersBase
    {
        /// NOTE: You need to ensure all TrackedState objects of a given type are declared consecutively!!!

        std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) override
        {
            auto factory = helpers::ParameterFactory::create (factoryID, factoryName);

            return factory;
        }
    } parameters;



    void prepare (const juce::dsp::ProcessSpec& spec_)
    {
        spec = spec_;
        parameters.prepare(spec);
        dcBlocker.prepare(spec);
        if  constexpr (FIRST_ORDER)
            dcBlocker.coefficients = juce::dsp::IIR::Coefficients<float>::makeFirstOrderHighPass(spec.sampleRate, 15.0f);
        else
            dcBlocker.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(spec.sampleRate, 15.0f);
        reset();
    }

    void reset()
    {
        parameters.reset();
        dcBlocker.reset();
    }

    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
        const auto& inputBlock = context.getInputBlock();
        auto& outputBlock      = context.getOutputBlock();
        const auto numChannels = outputBlock.getNumChannels();
        const auto numSamples  = outputBlock.getNumSamples();
        ignoreUnused(numSamples);
        ignoreUnused(numChannels);
        ignoreUnused(inputBlock);

        jassert (inputBlock.getNumChannels() == numChannels);
        jassert (inputBlock.getNumSamples() == numSamples);

        dcBlocker.process(context);
    }

    float processSample( const float x )
    {
        return dcBlocker.processSample(x);
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

    juce::dsp::IIR::Filter<float> dcBlocker;
    juce::dsp::ProcessSpec spec{};
};

}