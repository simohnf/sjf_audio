/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */

//
// Created by Simon Fay on 14/07/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_ParameterFactory.h>

namespace sjf::dsp
{
/// just a wrapper around the juce StateVariableFilter to allow it to be easily dropped into custom processors
enum class FixedFilterType {Variable, LowPass, BandPass, HighPass};
template<FixedFilterType FixedType = FixedFilterType::Variable, bool NoResonance = false>
class SVF
{
public:
    struct Parameters : public helpers::AudioParametersBase
    {
        /// NOTE: You need to ensure all TrackedState objects of a given type are declared consecutively!!!
        FloatState  cutoff, resonance;
        ChoiceState type;


        std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) override
        {
            auto factory = helpers::ParameterFactory::create (factoryID, factoryName);
            {
                auto range = NormalisableRange<float>{ 20.0f, 20000.0f, 0.1f };
                range.setSkewForCentre(2000.0f);
                const auto attributes = AudioParameterFloatAttributes().withLabel("Hz");
                createTrackedParameter  (*factory, cutoff, "Cutoff",  "Cutoff Frequency",  range, 1000.0f, {}, attributes);
            }

            if constexpr (NoResonance)
            {
                resonance.currentValue = 0.7071f;
            }
            else
            {
                auto range = NormalisableRange<float>{ 0.1f,  18.0f, 0.001f};
            	range.setSkewForCentre(1.0f);
                const auto attributes = AudioParameterFloatAttributes();
                createTrackedParameter  (*factory, resonance, "Q",  "Q",  range, 0.707f, {}, attributes);
            }

            if constexpr (FixedType != FixedFilterType::Variable)
            {
                type.currentValue = static_cast<int>(FixedType) - 1;
            }
            else
            {
                createTrackedParameter(*factory, type, "Type", "Type", {"LP", "BP", "HP"}, 0);
            }

            return factory;
        }
    } parameters;

    template <bool B = FixedType != FixedFilterType::Variable, typename = std::enable_if_t<B>>
    void setType (const juce::dsp::StateVariableFilter::StateVariableFilterType& filterType)
    {
        parameters.type.currentValue = static_cast<int>(filterType);
    }



    void prepare (const juce::dsp::ProcessSpec& spec_)
    {
        spec = spec_;
        parameters.prepare(spec);
        filter.prepare(spec);
        inputChannelPointers.resize(spec.numChannels);
        outputChannelPointers.resize(spec.numChannels);
        reset();
    }

    void reset()
    {
        parameters.reset();
        updateFilterParameters();
        filter.reset();
    }

    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
        const auto& inputBlock = context.getInputBlock();
        auto& outputBlock      = context.getOutputBlock();
        const auto numSamples  = outputBlock.getNumSamples();
        const auto numChannels = inputBlock.getNumChannels();


        for (auto channel = 0ul; channel < numChannels; ++channel)
        {
            inputChannelPointers[channel] = inputBlock.getChannelPointer (channel);
            outputChannelPointers[channel] = outputBlock.getChannelPointer (channel);
        }

        if (parameters.checkForStateChange())
        {
            for (auto i = 0ul; i < numSamples; ++i)
            {
                parameters.tickSmoothers();
                updateFilterParameters();
                for ( auto c = 0ul; c < numChannels; c++)
                    outputChannelPointers[c][i] = filter.processSample(static_cast<int>(c), inputChannelPointers[c][i]);
            }
        }
        else
        {
            updateFilterParameters();
            filter.process(context);
        }
    }

    float processSample(const int channel, const float input)
    {
        return filter.processSample(channel, input);
    }

    std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName)
    {
        return parameters.createParameters (factoryID, factoryName);
    }

    void updateFilterParameters()
    {
        filter.setType(static_cast<juce::dsp::StateVariableTPTFilterType>(parameters.type.currentValue));
        filter.setCutoffFrequency(jmin(static_cast<float>(spec.sampleRate) * 0.499f, parameters.cutoff.currentValue));
        filter.setResonance(parameters.resonance.currentValue);
    }

private:
    juce::dsp::StateVariableTPTFilter<float> filter;
    juce::dsp::ProcessSpec spec{};

    std::vector<const float*> inputChannelPointers;
    std::vector<float*> outputChannelPointers;
};
}
