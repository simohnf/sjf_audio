/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 28/10/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_ParameterFactory.h>

namespace sjf::dsp::granular{

enum class EnvelopeType
{
    hann, tri, sine, sinc, box
};

struct Grain
{
    float grainLengthSamps  {0.0f};
    float currentDelaySamps {0.0f};
    float playbackRate      {1.0f};
    float gain              {1.0f};
    float left              {0.707f};
    float right             {0.707f};
    float frameDelta        {0.0f};
    float normPos           {0.0f};
    bool active             {false};

    void spawn(const float initialDelaySamps,
               const float lengthSamps,
               const float rate = 1.0f,
               const float grainGain = 1.0f,
               const float grainPan = 0.5f) noexcept
    {
        currentDelaySamps = initialDelaySamps;
        playbackRate = 1.0f - rate;

        grainLengthSamps = std::max(1.0f, lengthSamps);

        gain = grainGain;
        left = std::sqrt(1.0f - grainPan);
        right = std::sqrt(grainPan);

        frameDelta = 1.0f / grainLengthSamps;
        normPos = 0.0f;

        active = true;
    }

    template<sjf::interpolation::InterpolatorTypes InterpType = sjf::interpolation::InterpolatorTypes::cubic, typename DelayLineType>
    [[nodiscard]] forcedinline float processSample(const DelayLineType& delayLine) noexcept
    {
        if (!active)
            return 0.0f;

        const auto windowGain = sjf::helpers::functions::waveforms::getHannWindow(normPos);

        const auto rawSample = delayLine.template readSample<InterpType>(currentDelaySamps);
        currentDelaySamps += playbackRate;

        const auto maxSafeDelay = static_cast<float>(delayLine.getMaxDelayTimeSamps());
        currentDelaySamps = juce::jlimit(1.0f, maxSafeDelay, currentDelaySamps);

        normPos += frameDelta;
        if (normPos >= 1.0f)
            active = false;

        return rawSample * windowGain * gain;
    }

    template<sjf::interpolation::InterpolatorTypes InterpType = sjf::interpolation::InterpolatorTypes::cubic, typename DelayLineType>
    [[nodiscard]] forcedinline std::pair<float, float> processSample(const std::array<DelayLineType, 2>& delayLines) noexcept
    {
        if (!active)
            return {0.0f, 0.0f};

        const auto windowGain = sjf::helpers::functions::waveforms::getHannWindow(normPos);

        const auto rawSampleL = delayLines[0].template readSample<InterpType>(currentDelaySamps);
        const auto rawSampleR = delayLines[1].template readSample<InterpType>(currentDelaySamps);
        currentDelaySamps += playbackRate;

        const auto maxSafeDelay = static_cast<float>(delayLines[0].getMaxDelayTimeSamps());
        currentDelaySamps = juce::jlimit(1.0f, maxSafeDelay, currentDelaySamps);

        normPos += frameDelta;
        if (normPos >= 1.0f)
            active = false;

        const auto g = windowGain * gain;
        return {rawSampleL*g, rawSampleR*g};
    }

    void reset()
    {
        active = false; // everything else gets reset when spawned
    }
};


template<size_t MaxNumGrains = 64>
class GranulationEngine
{
public:
    void prepare(const juce::dsp::ProcessSpec&)
    {

    }

    void reset()
    {
        for (auto & g : grains)
            g.reset();
    }

    template<sjf::interpolation::InterpolatorTypes InterpType = sjf::interpolation::InterpolatorTypes::cubic, typename DelayLineType>
    [[nodiscard]] forcedinline std::pair<float, float> processSample(const DelayLineType& delayLine) noexcept
    {
        auto ret = std::pair<float, float>{0.0f, 0.0f};
        for (auto& g : grains)
        {
            auto output = g.template processSample<InterpType>(delayLine);
            ret.first += output * g.left;
            ret.second += output * g.right;
        }

        return ret;
    }


    template<sjf::interpolation::InterpolatorTypes InterpType = sjf::interpolation::InterpolatorTypes::cubic, typename DelayLineType>
    [[nodiscard]] forcedinline std::array<float, 2> processSample(const std::array<DelayLineType, 2>& delayLines) noexcept
    {
        auto ret = std::array<float, 2>{0.0f, 0.0f};
        for (auto& g : grains)
        {
            auto [outL, outR] = g.template processSample<InterpType>(delayLines);
            ret[0] += outL * g.left;
            ret[1] += outR * g.right;
        }

        return ret;
    }


    void spawn(const float initialDelaySamps,
           const float lengthSamps,
           const float rate = 1.0f,
           const float grainGain = 1.0f,
           const float grainPan = 0.5f) noexcept
    {
        for (auto& g : grains)
        {
            if (!g.active)
            {
                g.spawn(initialDelaySamps, lengthSamps, rate, grainGain, grainPan);
                return;
            }
        }
    }

private:
    std::array<Grain, MaxNumGrains> grains;
};

}


