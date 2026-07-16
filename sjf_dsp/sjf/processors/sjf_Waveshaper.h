//
// Created by Simon Fay on 16/07/2026.
#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_Waveshapers.h>

#include "sjf/helpers/sjf_HelperFunctions.h"

namespace sjf::dsp::waveshapers
{
    struct None
    {
        static const juce::String& getName()
        {
            static const juce::String name = "None";
            return name;
        }

        float processSample (const float x)
        {
            return x;
        }

        // Optional Method 1
        // void prepare (const juce::dsp::ProcessSpec& spec_){}

        // Optional Method 2
        // void reset(){}
    };

    struct SoftClip
    {
        static const juce::String& getName()
        {
            static const juce::String name = "Soft";
            return name;
        }

        float processSample (const float x)
        {
            return helpers::Waveshapers::Clippers::soft(x);
        }

        // Optional Method 1
        // void prepare (const juce::dsp::ProcessSpec& spec_){}

        // Optional Method 2
        // void reset(){}
    };

    struct HardClip
    {
        static const juce::String& getName()
        {
            static const juce::String name = "Hard";
            return name;
        }

        float processSample (const float x)
        {
            return helpers::Waveshapers::Clippers::hard(x);
        }
    };

    struct Overdrive
    {
        static const juce::String& getName()
        {
            static const juce::String name = "Overdrive";
            return name;
        }

        float processSample (const float x)
        {
            return helpers::Waveshapers::Clippers::tanh(x);
        }
    };

    struct Tape
    {
        static const juce::String& getName()
        {
            static const juce::String name = "Tape";
            return name;
        }

        float processSample (const float x)
        {
            return helpers::Waveshapers::Sigmoids::xOverOnePlusAbsX(x);
        }
    };

    struct BucketBrigade
    {
        static const juce::String& getName()
        {
            static const juce::String name = "BucketBrigade";
            return name;
        }

        float processSample (const float x)
        {
            return helpers::Waveshapers::Misc::bucketBrigade(x);
        }
    };



    template<typename ... SaturatorTypes>
    struct Waveshaper
    {
        static constexpr std::size_t numSaturators = sizeof...(SaturatorTypes);
        static_assert(numSaturators > 0, "Waveshaper requires at least one saturation type!");

        using Saturators = std::tuple<SaturatorTypes...>;
        Saturators saturators;

        void prepare(const juce::dsp::ProcessSpec& spec)
        {
            sjf::helpers::functions::utilities::forEach(saturators, [&](auto& w) {
                if constexpr (requires { w.prepare(spec); }) {
                    w.prepare(spec);
                }
            });
        }

        void reset()
        {
            sjf::helpers::functions::utilities::forEach(saturators, [&](auto& w) {
                if constexpr (requires { w.reset(); }) {
                    w.reset();
                }
            });
        }

        static const juce::StringArray& getNames()
        {
            static const juce::StringArray names = [] {
                juce::StringArray arr;
                (arr.add(SaturatorTypes::getName()), ...);
                return arr;
            }();
            return names;
        }

        template <std::size_t Index>
        float processSample(const float x)
        {
            static_assert(Index < numSaturators, "Saturation type index out of bounds!");
            return std::get<Index>(saturators).processSample(x);
        }

        template <std::size_t Index>
        float processSample(const float x) const
        {
            static_assert(Index < numSaturators, "Saturation type index out of bounds!");
            return std::get<Index>(saturators).processSample(x);
        }

    };
}
