/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 03/09/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_ParameterFactory.h>

namespace sjf::dsp{
class CrossoverFilter
{
public:
    struct Parameters : public helpers::AudioParametersBase
    {
        FloatState  frequency;

        std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) override
        {
            auto factory = helpers::ParameterFactory::create (factoryID, factoryName);
        	auto mapping = [this](const float x){
        		return jmin(static_cast<float>(spec.sampleRate) * 0.49f, x);
        	};
        	createTrackedFrequencyParameter(*factory, frequency, "Freq", "Frequency", 20.0f, 20000.0f, 1000.0f, 1000.0f, mapping);

            return factory;
        }
    } parameters;


    void prepare (const juce::dsp::ProcessSpec& spec_)
    {
        spec = spec_;
        parameters.prepare(spec);
    	filter.prepare(spec);
    	filter.setCutoffFrequency(parameters.frequency.currentValue);
        reset();
    }

    void reset()
    {
        parameters.reset();

    	filter.setCutoffFrequency(parameters.frequency.currentValue);
    	filter.reset();
    }


	void process(const juce::dsp::AudioBlock<float>& inputBlock, juce::dsp::AudioBlock<float>& lowBlock, juce::dsp::AudioBlock<float>& highBlock)
    {
    	if (parameters.checkForStateChange())
    	{
    		processSmoothedState(inputBlock, lowBlock, highBlock);
    	}
    	else
    	{
    		processStaticState(inputBlock, lowBlock, highBlock);
    	}

		#if JUCE_DSP_ENABLE_SNAP_TO_ZERO
		filter.snapToZero();
		#endif
    }

    std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName)
    {
        return parameters.createParameters (factoryID, factoryName);
    }

private:
    void processStaticState (const juce::dsp::AudioBlock<float>& inputBlock, juce::dsp::AudioBlock<float>& lowBlock, juce::dsp::AudioBlock<float>& highBlock) noexcept
    {
        const auto numChannels = inputBlock.getNumChannels();
        const auto numSamples  = inputBlock.getNumSamples();

        jassert (inputBlock.getNumChannels() == lowBlock.getNumChannels() && inputBlock.getNumSamples() == highBlock.getNumSamples());
        jassert (inputBlock.getNumSamples() == lowBlock.getNumSamples() && inputBlock.getNumSamples() == highBlock.getNumSamples());

        for (size_t channel = 0; channel < numChannels; ++channel)
        {
			const auto* inputSamples  = inputBlock.getChannelPointer (channel);
            auto* lowSamples = lowBlock.getChannelPointer (channel);
        	auto* highSamples = highBlock.getChannelPointer (channel);

            for (size_t i = 0; i < numSamples; ++i)
                filter.processSample(static_cast<int>(channel), inputSamples[i], lowSamples[i], highSamples[i]);
        }
    }

    void processSmoothedState (const juce::dsp::AudioBlock<float>& inputBlock, juce::dsp::AudioBlock<float>& lowBlock, juce::dsp::AudioBlock<float>& highBlock) noexcept
    {
    	const auto numChannels = inputBlock.getNumChannels();
    	const auto numSamples  = inputBlock.getNumSamples();

    	jassert (inputBlock.getNumChannels() == lowBlock.getNumChannels() && inputBlock.getNumSamples() == highBlock.getNumSamples());
    	jassert (inputBlock.getNumSamples() == lowBlock.getNumSamples() && inputBlock.getNumSamples() == highBlock.getNumSamples());
        for (size_t i = 0; i < numSamples; ++i)
        {
            parameters.tickSmoothers();

        	filter.setCutoffFrequency(parameters.frequency.currentValue);

        	for (size_t channel = 0; channel < numChannels; ++channel)
        	{
        		const auto* inputSamples  = inputBlock.getChannelPointer (channel);
        		auto* lowSamples = lowBlock.getChannelPointer (channel);
        		auto* highSamples = highBlock.getChannelPointer (channel);

        		filter.processSample(static_cast<int>(channel), inputSamples[i], lowSamples[i], highSamples[i]);
        	}
        }
    }



	juce::dsp::LinkwitzRileyFilter<float> filter;
    juce::dsp::ProcessSpec spec{};
};

}


