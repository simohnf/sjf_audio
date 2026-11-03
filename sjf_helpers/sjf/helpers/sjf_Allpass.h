/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 03/11/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_ParameterFactory.h>

namespace sjf::helpers{
    struct Allpass
    {
        void prepare (const juce::dsp::ProcessSpec& spec_)
        {
            spec = spec_;
            y1.resize(spec.numChannels);
            x1.resize(spec.numChannels);
            reset();
        }

        void reset()
        {
            for ( auto i = 0ul; i < spec.numChannels; ++i)
                y1[i] = x1[i] = 0.0f;
        }

        void setCutoffFrequency( const float f)
        {
            const auto x = jlimit(0.0f, 1.0f, juce::MathConstants<float>::pi*f/static_cast<float>(spec.sampleRate));

            g = -1.0f * (1.0f-x)/(1.0f+x);
        }

        float processSample(const float x, const size_t channel)
        {
            y1[channel] = g*x + x1[channel] - g*y1[channel];
            x1[channel] = x;
            return y1[channel];
        }

        void process(const juce::dsp::ProcessContextReplacing<float>& context)
        {
            juce::ScopedNoDenormals noDenormals;

            [[maybe_unused]] const auto& inputBlock = context.getInputBlock();
            [[maybe_unused]] auto& outputBlock = context.getOutputBlock();
            [[maybe_unused]] const auto numChannels = outputBlock.getNumChannels();
            [[maybe_unused]] const auto numSamples = outputBlock.getNumSamples();


            for (auto c = 0ul; c < numChannels; ++c)
            {
                const auto in_ = inputBlock.getChannelPointer(c);
                const auto out_ = outputBlock.getChannelPointer(c);
                for ( auto i = 0ul; i < numSamples; ++i)
                {
                    out_[i] = processSample(in_[i], c);
                }
            }
        }

    private:
        juce::dsp::ProcessSpec spec{};
        std::vector<float> y1{}, x1{};
        float g{};
    };
}


