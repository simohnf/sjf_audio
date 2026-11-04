
/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 02/11/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_ParameterFactory.h>
#include <sjf/processors/Granulation/sjf_GranulationEngine.h>
#include <sjf/helpers/sjf_DelayLine.h>

namespace sjf::dsp
{
    class Stretch
    {
        struct SyncRatesProvider
        {
            struct Numerator
            {
                const static std::vector<int>& getValues()
                {
                    static const auto values = []()
                    {
                        const std::vector<int> vals {1, 2, 3, 4, 5, 6, 7};
                        std::vector<int> ret{};
                        for (auto i = 0; i < 5; ++i)
                        {
                            const auto p = static_cast<int>(pow(2, i));
                            for (const auto d : vals)
                            {
                                const auto next = p*d;
                                if (std::find(ret.begin(), ret.end(), next) == ret.end())
                                    ret.push_back(next);
                            }
                        }
                        return ret;
                    }();

                    return values;
                }

                const static juce::StringArray& getStrings()
                {
                    const static juce::StringArray strings =[]()
                    {
                        auto arr = juce::StringArray();
                        for (auto& d : getValues())
                            arr.add(juce::String(d));
                        return arr;
                    }();
                    return strings;
                }

                static int getDefault()
                {
                    static const auto defaultValue = getStrings().indexOf("1");
                    return defaultValue;
                }
            };

            struct Denominator
            {
                const static std::vector<int>& getValues()
                {
                    const static std::vector<int> values {1, 2, 3, 4, 5, 6, 7, 8};
                    return values;
                }

                const static juce::StringArray& getStrings()
                {
                    const static juce::StringArray strings =[]()
                    {
                        auto arr = juce::StringArray();
                        for (const auto i : getValues())
                            arr.add(juce::String(i));
                        return arr;
                    }();

                    return strings;
                }

                static int getDefault()
                {
                    static const auto defaultValue = getStrings().indexOf("8");
                    return defaultValue;
                }
            };
        };

    public:
        struct Parameters : public helpers::AudioParametersBase
        {

            FloatState delayT, stretch;
            ChoiceState delaySyncedNumerator, delaySyncedDenominator;
            BoolState delaySynced;



            sjf::helpers::SyncedDurationParameter<SyncRatesProvider> delay;

            Parameters()
            : delay (delayT, delaySynced, delaySyncedNumerator, delaySyncedDenominator, {100.0f, 10000.0f, 0.01f}, 500.0f)
            {}

            std::unique_ptr<helpers::ParameterFactory> createParameters(const juce::String& factoryID, const juce::String& factoryName) override
            {
                auto factory = helpers::ParameterFactory::create(factoryID, factoryName);

                factory->addChildFactory(delay.createParameters(factoryID+"Win", factoryName + " Window"));
                addTrackedChildParameters(delay);


                {
                    auto range = juce::NormalisableRange<float>{1.0f, 100.0f, 0.01f};
                    range.setSkewForCentre(10.0f);
                    const auto att = juce::AudioParameterFloatAttributes{}.withLabel("X");
                    createTrackedParameter(*factory, stretch, "Stretch", "Stretch", range, 1.0f, {}, att);
                }
                return factory;
            }

            void setPositionInfo(const juce::AudioPlayHead::PositionInfo& positionInfo_)
            {
                delay.setPositionInfo(positionInfo_);
            }
        } parameters;


        void prepare(const juce::dsp::ProcessSpec& spec_)
        {
            spec = spec_;
            parameters.prepare(spec);
            phasor.prepare(spec);
            engine.prepare(spec);
            for (auto& delayLine : delayLines)
            {
                delayLine.prepare(spec);
                delayLine.setMaxDelayTimeMS(10000.0f);
            }

            reset();
        }

        void reset()
        {
            parameters.reset();
            phasor.reset();
            engine.reset();
            for (auto& delayLine : delayLines)
                delayLine.reset();

            windowCounter = lastPosition = lastPhase = 0.0f;
            spawnNewGrain();
        }

        template <typename ProcessContext>
        void process(const ProcessContext& context) noexcept
        {
            [[maybe_unused]] const auto& inputBlock = context.getInputBlock();
            [[maybe_unused]] auto& outputBlock = context.getOutputBlock();
            [[maybe_unused]] const auto numChannels = outputBlock.getNumChannels();
            [[maybe_unused]] const auto numSamples = outputBlock.getNumSamples();

            jassert(inputBlock.getNumChannels() == numChannels);
            jassert(inputBlock.getNumChannels() == 2);
            jassert(inputBlock.getNumSamples() == numSamples);

            if (parameters.checkForStateChange())
            {
                parameters.reset();
            }

            const auto ppq_ =  positionInfo.getPpqPosition();
            const auto bpm_ = positionInfo.getBpm();

            if (ppq_.hasValue() && bpm_.hasValue())
            {
                const auto bpm = (*positionInfo.getBpm());
                const auto ppq = (*ppq_);

                if (!juce::approximatelyEqual(static_cast<float>(ppq), lastPosition))
                {
                    lastPosition = static_cast<float>(ppq);

                    if (parameters.delaySynced.currentValue)
                    {
                        if (bpm <= 0.0)
                            return;

                        const double samplesPerPPQ = (spec.sampleRate * 60.0) / bpm;
                        const double phasePerPPQ = static_cast<double>(phasor.m_increment) * samplesPerPPQ;

                        const double currentPhase = std::fmod(ppq * phasePerPPQ, 1.0);

                        phasor.m_phase = static_cast<float>(currentPhase < 0.0 ? currentPhase + 1.0 : currentPhase);
                    }
                }
            }


            if (phasor.willWrap(numSamples) || phasor.m_phase < lastPhase*0.5f)
            {
                processInternal<true>(context);
            }
            else
            {
                processInternal<false>(context);
            }
        }

        std::unique_ptr<helpers::ParameterFactory> createParameters(const juce::String& factoryID, const juce::String& factoryName)
        {
            return parameters.createParameters(factoryID, factoryName);
        }

        void setPositionInfo(const juce::AudioPlayHead::PositionInfo& positionInfo_)
        {
            parameters.setPositionInfo(positionInfo_);
            positionInfo = positionInfo_;
        }

    private:
        void spawnNewGrain()
        {
            const auto f = juce::jlimit(20.0f, 100.0f, static_cast<float>(spec.sampleRate) * parameters.stretch.currentValue / parameters.delayT.currentValue);

            phasor.setFrequency(f);

            const auto l = 2.0f * static_cast<float>(spec.sampleRate) / f; // grain length
            constexpr auto r = 1.0f; // playback rate

            const auto speed = 1.0f - (1.0f / parameters.stretch.currentValue);

            windowCounter = windowCounter < parameters.delayT.currentValue ? windowCounter : 0;
            const auto dt = jmax(windowCounter * speed, 0.0f);

            constexpr auto g = juce::MathConstants<float>::sqrt2; // gain
            constexpr auto p = 0.5f;  // pan

            engine.spawn(dt, l, r, g, p );
        }

        template<bool WillWrap, typename Context>
        void processInternal(const Context& context)
        {
            [[maybe_unused]] const auto& inputBlock = context.getInputBlock();
            [[maybe_unused]] auto& outputBlock = context.getOutputBlock();
            [[maybe_unused]] const auto numChannels = outputBlock.getNumChannels();
            [[maybe_unused]] const auto numSamples = outputBlock.getNumSamples();

            for (auto i = 0ul; i < numSamples; i++)
            {
                phasor.process();

                if constexpr (WillWrap)
                {
                    if (phasor.m_phase < lastPhase)
                    {
                        spawnNewGrain();
                    }
                }

                lastPhase = phasor.m_phase;

                for (auto c = 0ul; c < static_cast<size_t>(numChannels); c++)
                {
                    delayLines[c].writeSample(inputBlock.getChannelPointer (c)[i]);
                }


                const std::array<float, 2> outputs = engine.processSample(delayLines);

                for (auto c = 0ul; c < static_cast<size_t>(numChannels); c++)
                    outputBlock.getChannelPointer (c)[i] = outputs[c];

                windowCounter += 1.0f;
            }
        }

        juce::dsp::ProcessSpec spec{};
        granular::GranulationEngine<16> engine;
        std::array<sjf::helpers::DelayLine, 2> delayLines;
        sjf::dsp::oscillators::Phasor phasor;
        juce::AudioPlayHead::PositionInfo positionInfo{};
        float windowCounter{}, lastPosition{}, lastPhase{};
    };
}


