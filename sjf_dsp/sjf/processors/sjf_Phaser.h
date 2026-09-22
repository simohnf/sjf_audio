/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 21/09/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_ParameterFactory.h>
#include <sjf/oscillators/LFO/sjf_LFO.h>
#include <sjf/helpers/sjf_Waveshapers.h>

namespace sjf::dsp{
template<size_t MaxOrder, typename ConfiguredLFO>
class Phaser
{
public:
	static constexpr auto NumFilters = MaxOrder * 2;
	using LFO = ConfiguredLFO;
    struct Parameters : public helpers::AudioParametersBase
    {
        FloatState  centreF, feedback;
    	IntState order;

        std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) override
        {
            auto factory = helpers::ParameterFactory::create (factoryID, factoryName, true, false);
        	createTrackedFrequencyParameter(*factory, centreF, "CentreF", "Centre Frequency", 20.0f, 20000.0f, 2000.0f, 2000.0f, [&](const float x){ return x; });
        	createTrackedPercentParameter(*factory, feedback, "FB", "Feedback", -100.0f, 100.0f, 0.0f, 0.0f, [](const float x){ return sjf::helpers::Waveshapers::Clippers::tanh(x*0.01f); });

        	static_assert(MaxOrder >= 1);
        	if (MaxOrder > 1)
				createTrackedParameter(*factory, order, "Order", "Order", 1, MaxOrder, juce::jmin(6ul, MaxOrder), [](const int x){ return x * 2;});
            return factory;
        }
    } parameters;


    void prepare (const juce::dsp::ProcessSpec& spec_)
    {
        spec = spec_;
        parameters.prepare(spec);
    	lfo.prepare(spec);

    	for (auto & filt : filters)
    	{
    		filt.setType(juce::dsp::FirstOrderTPTFilterType::allpass);
    		filt.prepare(spec);
    	}

    	lastSample.setSize(static_cast<int>(spec.numChannels), 1);
        reset();
    }

    void reset()
    {
        parameters.reset();
    	lfo.reset();
    	for (auto & filt : filters)
    	{
    		filt.setCutoffFrequency(juce::jmin(parameters.centreF.currentValue, static_cast<float>(spec.sampleRate * 0.4999)));
    		filt.reset();
    	}

    	lastSample.clear();

    	frequencyRange.start = 20.0f;
    	frequencyRange.end   = juce::jmin(static_cast<float>(spec.sampleRate * 0.4999), 20000.0f);
    	frequencyRange.setSkewForCentre(calculateSkewForCentreFrequency());
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

    	lfo.process(context);

        if (parameters.checkForStateChange())
        {
            processSmoothedState(context);

        	for (auto f = static_cast<size_t>(parameters.order.currentValue); f < filters.size(); ++f)
        	{
        		filters[f].reset();
        	}
        }
        else
        {
            processStaticState(context);
        }


		#if JUCE_DSP_ENABLE_SNAP_TO_ZERO
    	for (auto & filt : filters)
			filt.snapToZero();
		#endif
    }

    std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName)
    {
        auto factory = parameters.createParameters (factoryID, factoryName);
    	factory->addChildFactory(lfo.createParameters (factoryID + "LFO", factoryName + " LFO"));
    	return factory;
    }

private:
	float calculateSkewForCentreFrequency()
	{
		return juce::jmin(static_cast<float>(spec.sampleRate * 0.4),parameters.centreF.currentValue);
	}
    template <typename ProcessContext>
    void processStaticState (const ProcessContext& context) noexcept
    {
        const auto& inputBlock = context.getInputBlock();
        auto& outputBlock      = context.getOutputBlock();
        const auto numChannels = outputBlock.getNumChannels();
        const auto numSamples  = outputBlock.getNumSamples();

        jassert (inputBlock.getNumChannels() == numChannels);
        jassert (inputBlock.getNumSamples() == numSamples);

    	auto lfoBlock = lfo.getLfoOutput();
    	lfoBlock.add(1.0f);
    	lfoBlock.multiplyBy(0.5f);
    	frequencyRange.setSkewForCentre(calculateSkewForCentreFrequency());
    	juce::dsp::AudioBlock<float>::process(lfoBlock, lfoBlock, [&](const float x){ return frequencyRange.convertFrom0to1(x);});

    	const auto order = static_cast<size_t>(parameters.order.currentValue);
    	const auto fb = parameters.feedback.currentValue;
        for (size_t channel = 0; channel < numChannels; ++channel)
        {
            auto* inputSamples  = inputBlock.getChannelPointer (channel);
            auto* outputSamples = outputBlock.getChannelPointer (channel);
        	auto* lfo_ = lfoBlock.getChannelPointer (channel);
        	auto* last = lastSample.getWritePointer(static_cast<int>(channel));
            for (size_t i = 0; i < numSamples; ++i)
            {
            	outputSamples[i] = inputSamples[i] + last[0] * fb;
	            for (auto f = 0ul; f < order; ++f)
	            {
	            	auto& filt = filters[f];
		            filt.setCutoffFrequency(lfo_[i]);
	            	outputSamples[i] = filt.processSample(static_cast<int>(channel), outputSamples[i]);
	            }
            	last[0] = outputSamples[i];
            }
        }
    }

    template <typename ProcessContext>
    void processSmoothedState (const ProcessContext& context) noexcept
    {
        const auto& inputBlock = context.getInputBlock();
        auto& outputBlock      = context.getOutputBlock();
        const auto numChannels = outputBlock.getNumChannels();
        const auto numSamples  = outputBlock.getNumSamples();

        jassert (inputBlock.getNumChannels() == numChannels);
        jassert (inputBlock.getNumSamples() == numSamples);

    	auto lfoBlock = lfo.getLfoOutput();
    	lfoBlock.add(1.0f);
    	lfoBlock.multiplyBy(0.5f);

    	// lfoBlock.process(lfoBlock, lfoBlock, [](float x){ return mapToLog10(x, 0.0f, 1.0f);});
    	const auto order = static_cast<size_t>(parameters.order.currentValue);
    	for (size_t i = 0; i < numSamples; ++i)
    	{
    		parameters.tickSmoothers();
    		frequencyRange.setSkewForCentre(calculateSkewForCentreFrequency());
    		for (size_t channel = 0; channel < numChannels; ++channel)
    		{
    			auto* inputSamples  = inputBlock.getChannelPointer (channel);
    			auto* outputSamples = outputBlock.getChannelPointer (channel);
    			auto* lfo_ = lfoBlock.getChannelPointer (channel);
    			auto* last = lastSample.getWritePointer(static_cast<int>(channel));
    			outputSamples[i] = inputSamples[i] + last[0] * parameters.feedback.currentValue;
    			for (auto f = 0ul; f < order; ++f)
    			{
    				auto & filt = filters[f];
		            filt.setCutoffFrequency(frequencyRange.convertFrom0to1(lfo_[i]));
    				outputSamples[i] = filt.processSample(static_cast<int>(channel), outputSamples[i]);
    			}
    			last[0] = outputSamples[i];
    		}
    	}
    }

	std::array<juce::dsp::FirstOrderTPTFilter<float>, NumFilters> filters;
	LFO lfo;
	juce::AudioBuffer<float> lastSample;
	juce::NormalisableRange<float> frequencyRange;

    juce::dsp::ProcessSpec spec{};
};


namespace modulation_effects
{
	using BasicPhaserLFO =	sjf::dsp::oscillators::lfo::LFO<dsp::oscillators::lfo::DefaultWaveformProvider,
															dsp::oscillators::lfo::lfo_config::TempoSync,
															dsp::oscillators::lfo::lfo_config::Invert,
															dsp::oscillators::lfo::lfo_config::PhaseOffset,
															dsp::oscillators::lfo::lfo_config::Smooth,
															dsp::oscillators::lfo::lfo_config::Depth>;
	using BasicPhaser = dsp::Phaser<16, BasicPhaserLFO>;
}

}


