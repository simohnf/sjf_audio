//
// Created by Simon Fay on 14/07/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_ParameterFactory.h>

namespace sjf::dsp
{
/// just a wrapper around the juce StateVariableFilter to allow it to be easily dropped into custom processors
template<bool FixedType = false, bool NoResonance = false>
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
            if constexpr (!NoResonance)
            {
                const auto range = NormalisableRange<float>{ 0.1f,  18.0f, 0.01f};
                const auto attributes = AudioParameterFloatAttributes();
                createTrackedParameter  (*factory, resonance, "Q",  "Q",  range, 0.7071f, {}, attributes);
            }
            else
            {
                resonance.currentValue = 0.7071f;
            }

            if (!FixedType)
            {
                createTrackedParameter(*factory, type, "Type", "Type", {"LP", "BP", "HP"}, 0);
            }

            return factory;
        }
    } parameters;

    template <bool B = FixedType, typename = std::enable_if_t<B>>
    void setType (const juce::dsp::StateVariableFilter::StateVariableFilterType& filterType)
    {
        parameters.type.currentValue = static_cast<int>(filterType);
    }



    void prepare (const juce::dsp::ProcessSpec& spec_)
    {
        spec = spec_;
        parameters.prepare(spec);
        filter.resize(spec.numChannels);
        auto monoSpec = spec;
        monoSpec.numChannels = 1;
        for (auto& c : filter)
            c.prepare(monoSpec);
        reset();
    }

    void reset()
    {
        parameters.reset();
        for (auto& c : filter)
        {
            c.parameters->type = static_cast<juce::dsp::StateVariableFilter::StateVariableFilterType>(parameters.type.currentValue);
            c.parameters->setCutOffFrequency(spec.sampleRate, parameters.cutoff.currentValue, parameters.resonance.currentValue);
            c.reset();
        }
    }

    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
        const auto& inputBlock = context.getInputBlock();
        auto& outputBlock      = context.getOutputBlock();
        const auto numChannels = outputBlock.getNumChannels();
        const auto numSamples  = outputBlock.getNumSamples();

        jassert (inputBlock.getNumChannels() == numChannels);
        jassert (inputBlock.getNumSamples() == numSamples);



        if (parameters.checkForStateChange())
        {
            for ( auto channel = 0ul; channel < numChannels; channel++)
            {
                auto inBlock = inputBlock.getSingleChannelBlock(channel);
                auto outBlock = outputBlock.getSingleChannelBlock(channel);
                auto monoContext = [&]()
                {
                    if constexpr (ProcessContext::usesSeparateInputAndOutputBlocks())
                        return ProcessContext(inBlock, outBlock);
                    else
                        return ProcessContext(outBlock);
                }();

                processSmoothedState(monoContext, channel);
            }
        }
        else
        {
            for (auto channel = 0ul; channel < numChannels; channel++)
            {
                auto inBlock = inputBlock.getSingleChannelBlock(channel);
                auto outBlock = outputBlock.getSingleChannelBlock(channel);
                auto monoContext = [&]()
                {
                    if constexpr (ProcessContext::usesSeparateInputAndOutputBlocks())
                        return ProcessContext(inBlock, outBlock);
                    else
                        return ProcessContext(outBlock);
                }();

                processStaticState(monoContext, channel);
            }
        }
    }

    template <typename ProcessContext>
    void process (const ProcessContext& context, const size_t channel) noexcept
    {
        const auto& inputBlock = context.getInputBlock();
        auto& outputBlock      = context.getOutputBlock();
        // const auto numChannels = outputBlock.getNumChannels();
        // const auto numSamples  = outputBlock.getNumSamples();

        jassert (inputBlock.getNumChannels() == 1);
        jassert (outputBlock.getNumSamples() == 1);

        if (parameters.checkForStateChange())
        {
            processSmoothedState(context, channel);
        }
        else
        {
            processStaticState(context, channel);
        }
    }

    float processSample(const bool shouldTickSmoohters, const size_t channel, const float input)
    {
        auto& params = *filter[channel].parameters;
        if (shouldTickSmoohters)
        {
            const auto f = parameters.cutoff.currentValue;
            const auto r = parameters.resonance.currentValue;
            const auto t = parameters.type.currentValue;
            parameters.tickSmoothers();
            if ( !approximatelyEqual(f, parameters.cutoff.currentValue) || !approximatelyEqual(r, parameters.resonance.currentValue) || t != parameters.type.currentValue )
            {
                params.type = static_cast<juce::dsp::StateVariableFilter::StateVariableFilterType>(parameters.type.currentValue);
                params.setCutOffFrequency(spec.sampleRate, parameters.cutoff.currentValue, parameters.resonance.currentValue);
            }
        }

        return filter[channel].processSample(input);
    }

    std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName)
    {
        return parameters.createParameters (factoryID, factoryName);
    }

private:
    template <typename ProcessContext>
    void processStaticState (const ProcessContext& context, const size_t channel) noexcept
    {
        const auto& inputBlock = context.getInputBlock();
        auto& outputBlock      = context.getOutputBlock();
        // const auto numChannels = outputBlock.getNumChannels();

        jassert (inputBlock.getNumChannels() == 1);
        jassert (outputBlock.getNumSamples() == 1);

        auto& params = *filter[channel].parameters;
        params.type = static_cast<juce::dsp::StateVariableFilter::StateVariableFilterType>(parameters.type.currentValue);
        params.setCutOffFrequency(spec.sampleRate, parameters.cutoff.currentValue, parameters.resonance.currentValue);
        filter[channel].process(context);

    }

    template <typename ProcessContext>
    void processSmoothedState (const ProcessContext& context, const size_t channel) noexcept
    {
        const auto& inputBlock = context.getInputBlock();
        auto& outputBlock      = context.getOutputBlock();
        const auto numChannels = outputBlock.getNumChannels();
        const auto numSamples  = outputBlock.getNumSamples();

        jassert (inputBlock.getNumChannels() == numChannels);
        jassert (inputBlock.getNumSamples() == numSamples);


        const auto shouldTickParameters = channel == 0;
        for (size_t i = 0; i < numSamples; ++i)
        {
            // Remember to tick the smoothers!!! for every sample
            if (shouldTickParameters)
                parameters.tickSmoothers();

            auto& params = *filter[channel].parameters;
            params.type = static_cast<juce::dsp::StateVariableFilter::StateVariableFilterType>(parameters.type.currentValue);
            params.setCutOffFrequency(spec.sampleRate, parameters.cutoff.currentValue, parameters.resonance.currentValue);
            outputBlock.getChannelPointer (0)[i] = filter[channel].processSample(inputBlock.getChannelPointer (0)[i]);
        }

    }

    std::vector<juce::dsp::StateVariableFilter::Filter<float>> filter;
    juce::dsp::ProcessSpec spec{};

};
}