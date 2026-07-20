/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */

//
// Created by Simon Fay on 16/07/2026.
//
#pragma once
#include <JuceHeader.h>

#include "sjf/helpers/sjf_HelperFunctions.h"

namespace sjf::dsp::oscillators::lfo
{
// =================================================================
// Example 1: Minimal Waveform (Only required methods)
// =================================================================
struct Sine
{
    static const juce::String& getName()
    {
        static const juce::String name = "Sine";
        return name;
    }

    float processSample (const float phase)
    {
        return sjf::helpers::functions::waveforms::getSin(phase);
    }
};

struct Triangle
{
    static const juce::String& getName()
    {
        static const juce::String name = "Triangle";
        return name;
    }

    float processSample (const float phase)
    {
        return sjf::helpers::functions::waveforms::getTriangle(phase);
    }
};

struct Sawtooth
{
    static const juce::String& getName()
    {
        static const juce::String name = "Sawtooth";
        return name;
    }

    float processSample (const float phase)
    {
        return sjf::helpers::functions::waveforms::getSaw(phase);
    }
};

struct Square
{
    static const juce::String& getName()
    {
        static const juce::String name = "Square";
        return name;
    }

    float processSample (const float phase)
    {
        return sjf::helpers::functions::waveforms::getSquare(phase);
    }
};

// =================================================================
// Example 2: Fully-Featured Waveform (Has all optional methods)
// =================================================================
struct ComplexSine
{
    float feedbackState = 0.0f;
    juce::dsp::ProcessSpec spec;

    static const juce::String& getName()
    {
        static const juce::String name = "Complex Sine";
        return name;
    }

    float processSample (const float phase)
    {
        float output = sjf::helpers::functions::waveforms::getSin(phase) + (feedbackState * 0.1f);
        feedbackState = output; // updating state
        return output;
    }

    // Optional Method 1
    void prepare (const juce::dsp::ProcessSpec& spec_)
    {
        spec = spec_;
        feedbackState = 0.0f;
    }

    // Optional Method 2
    void reset()
    {
        feedbackState = 0.0f;
    }
};

/**
 * @brief A compile-time collection manager and dispatcher for low-frequency oscillator waveforms.
 *
 * This template aggregates a custom set of LFO waveform generators into a static tuple.
 * It provides a clean, unified interface to query their user-facing display names,
 * prepare or reset stateful generators, and process phase signals through the selected index.
 *
 * Using C++20 constraints (`requires` clauses), the class automatically identifies and
 * invokes initialisation or reset sequences only on the underlying waveform structs
 * that implement them. Unused sequences are completely compiled out, yielding excellent
 * runtime performance.
 *
 * ### Example Usage:
 * @code
 * // Configure an LFO provider offering simple Sine and advanced ComplexSine shapes
 * using MyLfoShapes = sjf::dsp::oscillators::lfo::LFOWaveformProvider<
 *     sjf::dsp::oscillators::lfo::Sine,
 *     sjf::dsp::oscillators::lfo::ComplexSine
 * >;
 *
 * MyLfoShapes provider;
 *
 * // Retrieve the name list: {"Sine", "Complex Sine"}
 * const auto& names = MyLfoShapes::getNames();
 *
 * // Prepare all internal stateful shapes
 * provider.prepare(spec);
 *
 * // Evaluate the Sine shape (Index 0) at the current phase
 * float lfoVal = provider.processSample<0>(0.25f);
 * @endcode
 *
 * @tparam Waveforms A parameter pack containing the waveform classes to compile into
 *                   this provider instance.
 */
template <typename... Waveforms>
struct LFOWaveformProvider
{
    static constexpr std::size_t numWaveforms = sizeof...(Waveforms);
    static_assert(numWaveforms > 0, "LFOWaveformProvider requires at least one waveform type!");

    using WaveformTypes = std::tuple<Waveforms...>;
    WaveformTypes waveforms;



    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        sjf::helpers::functions::utilities::forEach(waveforms, [&](auto& w) {
            if constexpr (requires { w.prepare(spec); }) {
                w.prepare(spec);
            }
        });
    }

    void reset()
    {
        sjf::helpers::functions::utilities::forEach(waveforms, [&](auto& w) {
            if constexpr (requires { w.reset(); }) {
                w.reset();
            }
        });
    }

    static const juce::StringArray& getNames()
    {
        static const juce::StringArray names = [] {
            juce::StringArray arr;
            (arr.add(Waveforms::getName()), ...);
            return arr;
        }();
        return names;
    }

    template <std::size_t Index>
    float processSample(const float phase)
    {
        static_assert(Index < numWaveforms, "Waveform index out of bounds!");
        return std::get<Index>(waveforms).processSample(phase);
    }

    template <std::size_t Index>
    float processSample(const float phase) const
    {
        static_assert(Index < numWaveforms, "Waveform index out of bounds!");
        return std::get<Index>(waveforms).processSample(phase);
    }
};

using DefaultWaveformProvider = LFOWaveformProvider<Sine, Triangle, Sawtooth, Square>;

}
