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

            const auto ind1 = (delayLine.size() + writeIndex - delayTimeSamps) & MASK;
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
