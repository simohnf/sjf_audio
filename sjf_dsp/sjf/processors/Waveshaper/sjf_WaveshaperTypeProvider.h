//
// Created by Simon Fay on 16/07/2026.
#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_Waveshapers.h>

#include "sjf/helpers/sjf_HelperFunctions.h"

namespace sjf::dsp::waveshaper
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


    /**
     * @brief A compile-time collection manager and dispatcher for different waveshaper algorithms.
     *
     * This class aggregates multiple waveshaper types into a static tuple at compile time.
     * It provides a unified interface to query their display names, initialise/reset those
     * that require state tracking, and dispatch sample processing dynamically or statically
     * via index template parameters.
     *
     * By using compile-time dispatching and template constraints (via C++20 `requires` clauses),
     * unused initialisation and reset functions are completely compiled out, generating
     * highly optimised machine code.
     *
     * ### Example Usage:
     * @code
     * // Define the selection of saturators available to the system
     * using MySaturator = sjf::dsp::waveshaper::WaveshaperTypeProvider<
     *     sjf::dsp::waveshaper::None,
     *     sjf::dsp::waveshaper::SoftClip,
     *     sjf::dsp::waveshaper::Tape
     * >;
     *
     * MySaturator saturator;
     *
     * // Retrieve the list of names: {"None", "Soft", "Tape"}
     * const auto& names = MySaturator::getNames();
     *
     * // Process a sample through the "SoftClip" stage (Index 1)
     * float saturatedSample = saturator.processSample<1>(inputSample);
     * @endcode
     *
     * @tparam SaturatorTypes A parameter pack containing the waveshaping structures to compile
     *                        into this provider instance.
     */
    template<typename ... SaturatorTypes>
    struct WaveshaperTypeProvider
    {
        static constexpr std::size_t numSaturators = sizeof...(SaturatorTypes);
        static_assert(numSaturators > 0, "WaveshaperTypeProvider requires at least one saturation type!");

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

        template <std::size_t Index, bool On = true>
        float processSample(const float x)
        {
            static_assert(Index < numSaturators, "Saturation type index out of bounds!");
            if constexpr (On)
                return std::get<Index>(saturators).processSample(x);
            else
                return x;
        }

        template <std::size_t Index, bool On = true>
        float processSample(const float x) const
        {
            static_assert(Index < numSaturators, "Saturation type index out of bounds!");
            if constexpr (On)
                return std::get<Index>(saturators).processSample(x);
            else
                return x;
        }

    };
}
