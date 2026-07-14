//
// Created by Simon Fay on 13/07/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_Interpolators.h>

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

        void writeSample(const float x)
        {
            delayLine[writeIndex] = x;
            writeIndex &= MASK;
        }

        void readSample(const float delayTimeSamps)
        {

        }

        template<sjf::interpolation::InterpolatorTypes InterpType>
        float readSample(const float delayTimeSamps, const InterpType interpType)
        {
            jassert(static_cast<size_t>(delayTimeSamps + 0.5f) < delayLine.size() - 2);

            auto find1 = (delayLine.size() + writeIndex - delayTimeSamps);
            jassert (find1 < delayLine.size() && find1 >= 0);
            auto ind1 = static_cast<size_t>(find1);
            const auto mu = find1 - static_cast<float>(ind1);
            ind1 &= MASK;
            const auto ind2 = (ind1 + 1) & MASK;

            if constexpr (InterpType == sjf::interpolation::InterpolatorTypes::none)
                return readSample(static_cast<size_t>(std::round(delayTimeSamps)));
            if constexpr (InterpType == sjf::interpolation::InterpolatorTypes::linear)
                return sjf::interpolation::linearInterpolate(mu, ind1, ind2);

            const auto ind0 = (ind1 + delayLine.size() - 1) & MASK;
            const auto ind3 = (ind2 + 1) & MASK;

            if constexpr (InterpType == sjf::interpolation::InterpolatorTypes::cubic)
                return sjf::interpolation::cubicInterpolate(mu, ind0, ind1, ind2, ind3);
            if constexpr (InterpType == sjf::interpolation::InterpolatorTypes::pureData)
                return sjf::interpolation::fourPointInterpolatePD(mu, ind0, ind1, ind2, ind3);
            if constexpr (InterpType == sjf::interpolation::InterpolatorTypes::fourthOrder)
                return sjf::interpolation::fourPointFourthOrderOptimal(mu, ind0, ind1, ind2, ind3);
            if constexpr (InterpType == sjf::interpolation::InterpolatorTypes::godot)
                return sjf::interpolation::cubicInterpolateGodot(mu, ind0, ind1, ind2, ind3);
            if constexpr (InterpType == sjf::interpolation::InterpolatorTypes::hermite)
                return sjf::interpolation::cubicInterpolateHermite2(mu, ind0, ind1, ind2, ind3);

            return delayLine[ind];
        }

        float readSample(const size_t delayTimeSamps)
        {
            jassert(delayTimeSamps < delayLine.size() - 2);
            const auto readIndex = (delayLine.size() + writeIndex - delayTimeSamps) & MASK;
            return delayLine[readIndex];
        }

    private:
        std::vector<float> delayLine;
        juce::dsp::ProcessSpec spec;
        size_t writeIndex{}, MASK{};


    };
}
