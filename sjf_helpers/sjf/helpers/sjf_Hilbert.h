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
#include <sjf/helpers/sjf_Allpass.h>

namespace sjf::helpers
{

    class Hilbert
    {
    public:
        static constexpr size_t Order = 6;

        void prepare (const juce::dsp::ProcessSpec& spec_)
        {
            spec = spec_;

            // The mathematically exact, verified wideband cutoff frequencies for Order = 6 at 48 kHz
            constexpr auto frequencies = std::array<std::array<float, Order>, 2>{{
                { 18.786f, 83.5065f, 335.134f, 1344.41f, 5471.87f, 41551.7f }, // Branch 0 (Real Path)
                { 5.4135f, 41.118f, 167.359f, 671.372f, 2694.36f, 11976.9f }  // Branch 1 (Imaginary Path)
            }};
            for (auto i = 0ul; i < 2; ++i)
            {
                for (auto f = 0ul; f < Order; ++f)
                {
                    auto& filter = filters[i][f];
                    filter.prepare(spec);
                    filter.setCutoffFrequency(frequencies[i][f]);
                }
            }

            real.setSize(static_cast<int>(spec.numChannels), static_cast<int>(spec.maximumBlockSize));
            imaginary.setSize(static_cast<int>(spec.numChannels), static_cast<int>(spec.maximumBlockSize));

            reset();
        }

        void reset()
        {
            for (auto& bank : filters)
            {
                for (auto& filter : bank)
                {
                    filter.reset();
                }
            }
        }

        template <typename ProcessContext>
        void process(const ProcessContext& context) noexcept
        {
            [[maybe_unused]] const auto& inputBlock = context.getInputBlock();
            [[maybe_unused]] auto& outputBlock = context.getOutputBlock();
            [[maybe_unused]] const auto numChannels = outputBlock.getNumChannels();
            [[maybe_unused]] const auto numSamples = outputBlock.getNumSamples();

            jassert(inputBlock.getNumChannels() == numChannels);
            jassert(inputBlock.getNumSamples() == numSamples);

            auto realBlock = juce::dsp::AudioBlock<float>{real}.getSubBlock(0, numSamples);
            auto imaginaryBlock = juce::dsp::AudioBlock<float>{imaginary}.getSubBlock(0, numSamples);

            realBlock.copyFrom(inputBlock);
            imaginaryBlock.copyFrom(inputBlock);

            auto realContext = juce::dsp::ProcessContextReplacing<float>{realBlock};
            auto imaginaryContext = juce::dsp::ProcessContextReplacing<float>{imaginaryBlock};


            for (auto j = 0ul; j < Order; ++j)
            {
                filters[0][j].process(realContext);
                filters[1][j].process(imaginaryContext);
            }
        }

        juce::dsp::AudioBlock<float> getRealBlock()
        {
            return {real};
        }

        juce::dsp::AudioBlock<float> getImaginaryBlock()
        {
            return {imaginary};
        }


    private:
        juce::dsp::ProcessSpec spec{};
        juce::AudioBuffer<float> real, imaginary;
        std::array<std::array<Allpass, Order>, 2> filters;
    };
}


