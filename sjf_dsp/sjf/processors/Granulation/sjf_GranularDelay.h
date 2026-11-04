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
#include <sjf/processors/Granulation/sjf_GranulationEngine.h>

namespace sjf::dsp{
class GranularDelay
{
public:
    struct Parameters : public helpers::AudioParametersBase
    {
        struct ParameterWithJitter
        {
            FloatState jitter, parameter;
            juce::RangedAudioParameter* param{nullptr};
            juce::Random& rng;

            explicit ParameterWithJitter(Random& rng_) : rng(rng_){}

            void createParameters ( Parameters& parent,
                                    helpers::ParameterFactory& factory,
                                    const juce::String& paramID,
                                    const juce::String& paramName,
                                    const juce::NormalisableRange<float>& range,
                                    const float defaultValue,
                                    const FloatMapping& mapping,
                                    const juce::AudioParameterFloatAttributes& attributes)
            {
                parent.createTrackedPercentParameter   (factory, jitter, paramID+"J", paramName + " Jitter", 0, 100, 50, 0);

                auto jitterMapping = helpers::ParameterHelpers::getPercentMapping();
                auto m = [mapping, this, jitterMapping](float x){
                    if (param)
                    {
                        x = param->convertTo0to1(x);

                        const auto sign = (rng.nextBool() ? 1.0f : -1.0f);
                        auto j = jitterMapping(jitter.getParameterValue()) * rng.nextFloat();
                        j = sign < 0.0f ? (-1.0f * j * x) : (j * (1.0f - x));

                        x = juce::jlimit(0.0f, 1.0f, x+j);

                        x = param->convertFrom0to1(x);
                    }
                    return mapping ? mapping(x) : x;
                };
                param = parent.createTrackedParameter          (factory, parameter, paramID, paramName, range, defaultValue, m, attributes);

            }
        };

        struct JitteredParameter : helpers::AudioParametersBase
        {
            FloatState jitter;

            std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) override
            {
                auto factory = helpers::ParameterFactory::create (factoryID, factoryName, false, false);
                createTrackedPercentParameter(*factory, jitter, "Jitter", "Jitter", 0.0f, 100.0f, 50.0f, 0.0f);
                return factory;
            }

        } delayJitter;


        FloatState delayT, feedback;

        ParameterWithJitter rate, length, gain, pan, density, pitch;

        ChoiceState delaySyncedNumerator, delaySyncedDenominator;
        BoolState delaySynced;

        sjf::helpers::SyncedDurationParameter<> delay;

        Parameters(juce::Random& rng_)
        : rate(rng_), length(rng_), gain(rng_), pan(rng_), density(rng_), pitch(rng_)
        , delay (delayT, delaySynced, delaySyncedNumerator, delaySyncedDenominator, {1.0f, 10000.0f, 0.01f}, 100.0f)
        {}

        std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) override
        {
            auto factory = helpers::ParameterFactory::create (factoryID, factoryName);

            auto delayFactory = delay.createParameters(factoryID+"Delay", factoryName + " Delay");
            delayFactory->addChildFactory(delayJitter.createParameters(factoryID+"Delay"+"J", factoryName+" Delay"+" "));
            factory->addChildFactory(std::move(delayFactory));
            addTrackedChildParameters(delayJitter);
            addTrackedChildParameters(delay);

            using att = juce::AudioParameterFloatAttributes;
            auto rateRange = juce::NormalisableRange<float>{1.0f, 500.0f, 0.01f};
            rateRange.setSkewForCentre(100.0f);
            rate.createParameters(*this, *factory, "GRate", "Grate Rate", rateRange, 20.0f, {}, att{}.withLabel("Hz"));
            length.createParameters(*this, *factory, "GLen", "Grain Length", {0.0f, 100.0f, 0.01f}, 100.0f, [](const float x){return juce::jmap(x*0.01f, 0.01f, 2.0f);}, att{}.withLabel("%"));
            gain.createParameters(*this, *factory, "GGain", "Grain Gain", {-80.0f, 6.0f, 0.01f}, 0.0f, helpers::ParameterHelpers::getDecibelsMapping(), att{}.withLabel("dB"));
            pan.createParameters(*this, *factory, "GPan", "Grain Pan", {-1.0f, 1.0f, 0.01f}, 0.0f, [](const float x){return (x+1.0f)*0.5f;}, att{}.withLabel(""));
            density.createParameters(*this, *factory, "GDen", "Grain Density", {0.0f, 100.0f, 0.01f}, 100.0f, [](const float x){return (x*0.01f);}, att{}.withLabel("%"));
            pitch.createParameters(*this, *factory, "GPit", "Grain Pitch", {-12.0f, 12.0f, 0.01f}, 0.0f, [](const float x){return std::pow(2.0f, x/12.0f);}, att{}.withLabel("st"));
            createTrackedPercentParameter(*factory, feedback, "FB", "Feedback", 0.0f, 100.0f, 50.0f, 0.0f);

            return factory;
        }

        void setPositionInfo(const juce::AudioPlayHead::PositionInfo& positionInfo_)
        {
            delay.setPositionInfo(positionInfo_);
        }
    } ;


    void prepare (const juce::dsp::ProcessSpec& spec_)
    {
        spec = spec_;
        parameters.prepare(spec);
        engine.prepare(spec);
        phasor.prepare(spec);

        for (auto& delayLine : delayLines)
        {
            delayLine.prepare(spec);
            delayLine.setMaxDelayTimeMS(10000.0f);
        }

        auto monoSpec = spec_;
        monoSpec.numChannels = 1;
        dcBlock.prepare(monoSpec);
        dcBlock.coefficients = juce::dsp::IIR::Coefficients<float>::makeFirstOrderHighPass(spec.sampleRate, 15.0f);
        reset();
    }

    void reset()
    {
        parameters.reset();
        engine.reset();
        phasor.setFrequency(parameters.rate.parameter.currentValue);
        phasor.reset();

        for (auto& delayLine : delayLines)
            delayLine.reset();

        dcBlock.reset();
    }

    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
        [[maybe_unused]] const auto& inputBlock = context.getInputBlock();
        [[maybe_unused]] auto& outputBlock      = context.getOutputBlock();
        [[maybe_unused]] const auto numChannels = outputBlock.getNumChannels();
        [[maybe_unused]] const auto numSamples  = outputBlock.getNumSamples();

        jassert (inputBlock.getNumChannels() == numChannels);
        jassert (inputBlock.getNumSamples() == numSamples);


        if (parameters.checkForStateChange())
        {
            if (phasor.willWrap(numSamples))
            {
                processInternal<true, true>(context);
            }
            else
            {
                processInternal<true, false>(context);
            }
        }
        else
        {
            if (phasor.willWrap(numSamples))
            {
                processInternal<false, true>(context);
            }
            else
            {
                processInternal<false, false>(context);
            }
        }

        dcBlock.snapToZero();
    }

    std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName)
    {
        return parameters.createParameters (factoryID, factoryName);
    }

    void setPositionInfo(const juce::AudioPlayHead::PositionInfo& positionInfo_)
    {
        parameters.setPositionInfo(positionInfo_);
    }

private:
    void spawnNewGrain()
    {
        const auto f = parameters.rate.parameter.currentValue;
        phasor.setFrequency(f);

        auto d = parameters.density.parameter.currentValue;
        if (d >= jitterEngine.nextFloat())
        {
            const auto l = juce::jmax(static_cast<float>(spec.sampleRate)*0.001f*0.25f, parameters.length.parameter.currentValue * (1.0f / f) * static_cast<float>(spec.sampleRate));
            const auto r = parameters.pitch.parameter.currentValue;

            float minSafeDelay = 1.0f; //static_cast<float>(spec.maximumBlockSize);
            const float maxSafeDelay = static_cast<float>(delayLines[0].getMaxDelayTimeSamps()) - 1.0f;

            if (r > 1.0f)
                minSafeDelay += l * (r - 1.0f);

            auto dt = parameters.delay.time.currentValue;

            auto j = parameters.delayJitter.jitter.currentValue * jitterEngine.nextFloat();
            const auto desiredJitter = j * (parameters.delay.timeRange.end - parameters.delay.timeRange.start) * 0.001f * static_cast<float>(spec.sampleRate);

            const float sign = jitterEngine.nextBool() ? 1.0f : -1.0f;

            const float availableHeadroom = (sign < 0.0f) ? std::max(0.0f, dt - minSafeDelay) : std::max(0.0f, maxSafeDelay - dt);

            j = sign * std::min(desiredJitter, j*availableHeadroom);

            dt += j;
            dt = juce::jlimit(minSafeDelay, maxSafeDelay, dt);

            const auto g = parameters.gain.parameter.currentValue;
            const auto p = juce::jlimit(0.0f, 1.0f, parameters.pan.parameter.currentValue);

            engine.spawn(dt, l, r, g, p );
        }
    }

    template<bool TickSmoothers, bool WillWrap, typename ProcessContext>
    void processInternal(const ProcessContext& context)
    {
        [[maybe_unused]] const auto& inputBlock = context.getInputBlock();
        [[maybe_unused]] auto& outputBlock      = context.getOutputBlock();
        [[maybe_unused]] const auto numChannels = outputBlock.getNumChannels();
        [[maybe_unused]] const auto numSamples  = outputBlock.getNumSamples();

        jassert (inputBlock.getNumChannels() == numChannels);
        jassert (numChannels == 2);
        jassert (inputBlock.getNumSamples() == numSamples);

        constexpr auto scale = 1.0f / juce::MathConstants<float>::sqrt2;
        std::array<float, 2> inputs{0.0f, 0.0f};

        [[maybe_unused]] auto lastPhase = phasor.m_phase;
        for (auto i = 0ul; i < numSamples; ++i)
        {
            if constexpr (TickSmoothers)
            {
                parameters.tickSmoothers();
            }

            phasor.process();
            if constexpr (WillWrap)
            {
                if (phasor.m_phase < lastPhase)
                    spawnNewGrain();
            }
            lastPhase = phasor.m_phase;

            inputs[0] = inputBlock.getChannelPointer (0)[i];
            inputs[1] = inputBlock.getChannelPointer(1)[i];

            const auto outputs = engine.processSample(delayLines);

            outputBlock.setSample(0, static_cast<int>(i), outputs[0]);
            outputBlock.setSample(1, static_cast<int>(i), outputs[1]);

            const auto fbSamples = parameters.feedback.currentValue*scale*(dcBlock.processSample(outputs[0] + outputs[1])*scale);
            for (auto c = 0ul; c < numChannels; ++c)
                delayLines[c].writeSample((sjf::helpers::Waveshapers::Clippers::hard(inputs[c] + fbSamples, 5.0f)));
        }
    }

    granular::GranulationEngine<16> engine;
    std::array<sjf::helpers::DelayLine, 2> delayLines;
    juce::Random jitterEngine;
    Parameters parameters{jitterEngine};
    sjf::dsp::oscillators::Phasor phasor;
    juce::dsp::ProcessSpec spec{};
    juce::dsp::IIR::Filter<float> dcBlock;
};

}


