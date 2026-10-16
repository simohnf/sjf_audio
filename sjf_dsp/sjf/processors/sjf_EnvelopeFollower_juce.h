/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 15/10/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_ParameterFactory.h>

namespace sjf::dsp{
class EnvelopeFollower
{
public:
    struct Parameters : public helpers::AudioParametersBase
    {
        FloatState  attack, release;
        ChoiceState type;

        std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) override
        {
            auto factory = helpers::ParameterFactory::create (factoryID, factoryName);
            createTrackedTimeParameter(*factory, attack, "Att", "Attack", 0.0f, 200.0f, 100.0f, 10.0f, {});
            createTrackedTimeParameter(*factory, release, "Rel", "Release", 0.0f, 5000.0f, 100.0f, 10.0f, {});
            createTrackedParameter(*factory, type, "Type", "Calculation Type", {"Peak", "RMS"}, 0);
            return factory;
        }
    } parameters;


    void prepare (const juce::dsp::ProcessSpec& spec_)
    {
        spec = spec_;
        parameters.prepare(spec);
        ballistics.prepare(spec);
        reset();
    }

    void reset()
    {
        parameters.reset();
        updateBallisticsParameters();
        ballistics.reset();
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
            for (size_t i = 0; i < numSamples; ++i)
            {
                parameters.tickSmoothers();
                updateBallisticsParameters();

                for (auto chan = 0ul; chan < numChannels; ++chan)
                {
                    const auto sample = ballistics.processSample(static_cast<int>(chan), inputBlock.getSample(static_cast<int>(chan), static_cast<int>(i)));
                    outputBlock.setSample(static_cast<int>(chan), static_cast<int>(i), sample);
                }
            }
        }
        else
        {
            ballistics.process(context);
        }
    }

    std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String factoryID, const juce::String factoryName)
    {
        return parameters.createParameters (factoryID, factoryName);
    }

private:
    void updateBallisticsParameters()
    {
        ballistics.setAttackTime(parameters.attack.currentValue);
        ballistics.setReleaseTime(parameters.release.currentValue);
        if (lastCalculationType != static_cast<juce::dsp::BallisticsFilter<float>::LevelCalculationType>(parameters.type.currentValue))
        {
            lastCalculationType = static_cast<juce::dsp::BallisticsFilter<float>::LevelCalculationType>(parameters.type.currentValue);
            ballistics.setLevelCalculationType(lastCalculationType);
        }

    }

    juce::dsp::BallisticsFilter<float> ballistics;
    juce::dsp::ProcessSpec spec{};
    juce::dsp::BallisticsFilter<float>::LevelCalculationType lastCalculationType{juce::dsp::BallisticsFilter<float>::LevelCalculationType::peak};
};

}


