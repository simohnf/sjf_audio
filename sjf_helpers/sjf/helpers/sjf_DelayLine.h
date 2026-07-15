//
// Created by Simon Fay on 13/07/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_Interpolators.h>

#include "sjf_HelperFunctions.h"

namespace sjf::helpers
{
    class DelayLine
    {
    public:
        void reset() noexcept
        {
            std::fill(delayLine.begin(), delayLine.end(), 0);

            writeIndex = 0;
        }

        void prepare(const juce::dsp::ProcessSpec& spec_)
        {
            spec = spec_;
        }

        void setMaxDelayTimeMS(const float ms)
        {
            setMaxDelayTimeSamps(static_cast<int>(spec.sampleRate * 0.001 * ms));
        }

        void setMaxDelayTimeSamps(const int samps)
        {
            delayLine.resize(static_cast<size_t>(juce::nextPowerOfTwo(samps + 2)));
            MASK = delayLine.size() - 1;
        }

        size_t getMaxDelayTimeSamps() const
        {
            return delayLine.size() - 2;
        }

        void writeSample(const float x)
        {
            delayLine[writeIndex++] = x;
            writeIndex &= MASK;
        }
        template<sjf::interpolation::InterpolatorTypes InterpType>
        [[nodiscard]] float readSample(const float delayTimeSamps) const
        {
            jassert(delayLine.size() > 2);
            jassert(static_cast<size_t>(lround(delayTimeSamps)) < delayLine.size() - 2);

            const auto find1 = (static_cast<float>(delayLine.size() + writeIndex) - delayTimeSamps);
            jassert ( find1 >= 0);
            auto ind1 = static_cast<size_t>(find1);
            const auto mu = find1 - static_cast<float>(ind1);
            ind1 &= MASK;
            const auto ind2 = (ind1 + 1) & MASK;

            const auto x1 = delayLine[ind1];
            const auto x2 = delayLine[ind2];

            if constexpr (InterpType == sjf::interpolation::InterpolatorTypes::none)
                return readSample(static_cast<size_t>(std::round(delayTimeSamps)));
            else if constexpr (InterpType == sjf::interpolation::InterpolatorTypes::linear)
                return sjf::interpolation::linearInterpolate(mu, x1, x2);


            const auto ind0 = (ind1 + delayLine.size() - 1) & MASK;
            const auto ind3 = (ind2 + 1) & MASK;

            const auto x0 = delayLine[ind0];
            const auto x3 = delayLine[ind3];

            if constexpr (InterpType == sjf::interpolation::InterpolatorTypes::cubic)
                return sjf::interpolation::cubicInterpolate(mu, x0, x1, x2, x3);
            else if constexpr (InterpType == sjf::interpolation::InterpolatorTypes::pureData)
                return sjf::interpolation::fourPointInterpolatePD(mu, x0, x1, x2, x3);
            else if constexpr (InterpType == sjf::interpolation::InterpolatorTypes::fourthOrder)
                return sjf::interpolation::fourPointFourthOrderOptimal(mu, x0, x1, x2, x3);
            else if constexpr (InterpType == sjf::interpolation::InterpolatorTypes::godot)
                return sjf::interpolation::cubicInterpolateGodot(mu, x0, x1, x2, x3);
            else if constexpr (InterpType == sjf::interpolation::InterpolatorTypes::hermite)
                return sjf::interpolation::cubicInterpolateHermite2(mu, x0, x1, x2, x3);

            return 0.0f;

        }

        [[nodiscard]] float readSample(const size_t delayTimeSamps) const
        {
            jassert(delayTimeSamps < delayLine.size() - 2);
            const auto readIndex = (delayLine.size() + writeIndex - delayTimeSamps) & MASK;
            return delayLine[readIndex];
        }

    private:
        std::vector<float> delayLine;
        juce::dsp::ProcessSpec spec{};
        size_t writeIndex{}, MASK{};


    };
}

#if JUCE_MODULE_AVAILABLE_sjf_dsp
    //====================//====================//====================//====================//====================
    //====================//====================//====================//====================//====================
    //====================//====================//====================//====================//====================
    //====================//====================//====================//====================//====================

#include <sjf/oscillators/sjf_Phasor.h>
namespace sjf::helpers
{
    template <size_t NUM_HEADS = 3, bool EQUAL_POWER = true> // uses amplitude scaling if false
    class PitchShiftDelayLine
    {
    public:
        void prepare(const juce::dsp::ProcessSpec& spec_) noexcept
        {
            spec = spec_;
            delayLine.prepare(spec);
            phasor.prepare(spec);
            setWindowSizeMS(windowSizeMS);
        }

        void reset() noexcept
        {
            phasor.reset();
            delayLine.reset();
        }

        void setMaxDelayTimeMS(const float ms)
        {
            delayLine.setMaxDelayTimeMS(ms);
        }

        void setMaxDelayTimeSamps(const int samps)
        {
            delayLine.setMaxDelayTimeSamps(samps);
        }

        void writeSample(const float x)
        {
            delayLine.writeSample(x);
        }

        void setPitchShift(const float pitchShiftInSemiTones)
        {
            setPlayBackRate(std::pow(2.0f, pitchShiftInSemiTones/12.0f));
        }

        void setPlayBackRate(const float rate)
        {
            phasor.setFrequency(-1.0f*(rate - 1.0f)*invWindowSizeS);
        }

        void setWindowSizeMS(const float windowSizeMS_)
        {
            windowSizeMS = windowSizeMS_;
            invWindowSizeS = 1.0f / (windowSizeMS_ / 1000.0f);
            windowSizeSamps = windowSizeMS_ * static_cast<float>(spec.sampleRate) * 0.001f;
        }

        template<sjf::interpolation::InterpolatorTypes InterpType>
        [[nodiscard]] float readSample(const float delayTimeSamps)
        {
            jassert(roundToIntAccurate(windowSizeMS * 0.001f * spec.sampleRate + delayTimeSamps) + 2 < static_cast<int>(delayLine.getMaxDelayTimeSamps()));
            auto sum = 0.0f;
            auto t = phasor.process();


            for ( auto head = 0ul; head < NUM_HEADS; ++head)
            {
                const auto dt = t*windowSizeSamps + delayTimeSamps + 2.0f;
                const auto e = helpers::Functions::getHannWindow(t + 0.5f);
                sum += delayLine.readSample<InterpType>(dt) * e;
                t = helpers::Functions::wrapPhase(t, headOffset);
            }
            return sum * outputScale;
        }



    private:
        static float calculatedOutputScaling()
        {
            auto sum = 0.0f;
            for ( auto head = 0ul; head < NUM_HEADS; ++head)
                sum += helpers::Functions::getHannWindow(static_cast<float>(head)*headOffset);
            if (EQUAL_POWER)
                return 1.0f / sqrt(sum);
            return 1.0f/sum;
        }

        sjf::helpers::DelayLine delayLine;
        sjf::dsp::oscillators::Phasor<float> phasor{};
        juce::dsp::ProcessSpec spec{};

        float windowSizeMS{20}, windowSizeSamps{}, invWindowSizeS{}, outputScale{calculatedOutputScaling()};
        static constexpr float headOffset{1.0f/static_cast<float>(NUM_HEADS)};
    };

#endif
}
