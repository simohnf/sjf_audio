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

#include <sjf/processors/sjf_CrossoverFilter.h>

namespace sjf::dsp{
class StereoSpread
{
	struct PannedCrossoverFilter
	{
		explicit PannedCrossoverFilter(CrossoverFilter& filt) : crossoverFilter(filt) {}

		void prepare (const juce::dsp::ProcessSpec& spec_)
		{
			for (auto& p : pan)
				p.reset(spec_.sampleRate, 0.05f);
		}

		void reset()
		{
			pan[0].setCurrentAndTargetValue(std::sqrt(1.0f - panning));
			pan[1].setCurrentAndTargetValue(std::sqrt(panning));
		}

		void process(const juce::dsp::AudioBlock<float>& inputBlock, juce::dsp::AudioBlock<float>& lowBlock, juce::dsp::AudioBlock<float>& highBlock)
		{
			if (!(pan[0].isSmoothing() || pan[1].isSmoothing()))
			{
				pan[0].setTargetValue(std::sqrt(1.0f - panning));
				pan[1].setTargetValue(std::sqrt(panning));
			}

			auto lowMono = lowBlock.getSingleChannelBlock(0);
			auto highMono = highBlock.getSingleChannelBlock(0);
			crossoverFilter.process(inputBlock, lowMono, highMono);

			lowBlock.getSingleChannelBlock(1).copyFrom(lowMono);
			highBlock.getSingleChannelBlock(1).copyFrom(highMono);


			lowBlock.getSingleChannelBlock(0).multiplyBy(pan[0]);
			lowBlock.getSingleChannelBlock(1).multiplyBy(pan[1]);
		}

		void setPanning(const float targetPan)
		{
			panning = targetPan;
		}
	private:
		float panning = 0.5f;
		std::array<juce::LinearSmoothedValue<float>, 2> pan;
		CrossoverFilter& crossoverFilter;
	};

public:
	static constexpr auto maxOrder = 12;

	StereoSpread()
	{
		for (auto i = 0ul; i < crossoverFilters.size(); ++i)
			pannedCrossoverFilters[i] = std::make_unique<PannedCrossoverFilter>(crossoverFilters[i]);
	}

    struct Parameters : public helpers::AudioParametersBase
    {
        FloatState  lowAmount, highAmount, lowFreq, highFreq;
    	IntState	order;

        std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) override
        {
            auto factory = helpers::ParameterFactory::create (factoryID, factoryName);
        	createTrackedPercentParameter(*factory, lowAmount, "Low", "Low Amount", -100.0f, 100.0f, 0.0f, 100.0f);
        	createTrackedPercentParameter(*factory, highAmount, "High", "High Amount", -100.0f, 100.0f, 0.0f, 100.0f);

        	createTrackedFrequencyParameter(*factory, lowFreq, "LowF", "Low Frequency", 20.0f, 20000.0f, 1000.0f, 20.0f, {});
        	createTrackedFrequencyParameter(*factory, highFreq, "HighF", "High Frequency", 20.0f, 20000.0f, 1000.0f, 20000.0f, {});

        	createTrackedParameter(*factory, order, "Order", "Order", 2, maxOrder, maxOrder);



            return factory;
        }


    } parameters;


    void prepare (const juce::dsp::ProcessSpec& spec_)
    {
    	jassert(spec_.numChannels == 2);
        spec = spec_;
        parameters.prepare(spec);

    	auto monoSpec = spec;
    	monoSpec.numChannels = 1;
    	for (auto& filt : crossoverFilters)
    		filt.prepare(monoSpec);

    	for (const auto & panned : pannedCrossoverFilters)
    		panned->prepare(spec);

    	inputBuffer.setSize(static_cast<int>(1), static_cast<int>(spec.maximumBlockSize));
    	filteredBuffer.setSize(static_cast<int>(spec.numChannels), static_cast<int>(spec.maximumBlockSize));
    	filteredBuffer2.setSize(static_cast<int>(spec.numChannels), static_cast<int>(spec.maximumBlockSize));
        reset();
    }

    void reset()
    {
        parameters.reset();

    	updateFilterParameters();

    	for (auto& filt : crossoverFilters)
    		filt.reset();

    	for (const auto & panned : pannedCrossoverFilters)
    		panned->reset();

    	inputBuffer.clear();
    	filteredBuffer.clear();
    	filteredBuffer2.clear();


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

    	auto inBlock = juce::dsp::AudioBlock<float> (inputBuffer).getSubBlock(0, numSamples);
    	inBlock.copyFrom(inputBlock.getSingleChannelBlock(0));
    	inBlock.add(inputBlock.getSingleChannelBlock(1));
    	inBlock.multiplyBy(0.7071f);

    	outputBlock.clear();

    	auto filteredBlock = juce::dsp::AudioBlock<float> (filteredBuffer).getSubBlock(0, numSamples);
    	auto filteredBlock2 = juce::dsp::AudioBlock<float> (filteredBuffer2).getSubBlock(0, numSamples);

        if (parameters.checkForStateChange())
        {
        	parameters.reset();
            updateFilterParameters();
        }


    	for (auto i = 0ul; i < static_cast<size_t>(parameters.order.currentValue) +1; i++)
    	{
    		pannedCrossoverFilters[i]->process(inBlock, filteredBlock, filteredBlock2);
    		outputBlock.add(filteredBlock);
    		inBlock.copyFrom(filteredBlock2);
    	}

    	for (auto i = static_cast<size_t>(parameters.order.currentValue) + 1ul; i < pannedCrossoverFilters.size(); i++)
    	{
    		pannedCrossoverFilters[i]->reset();
    		crossoverFilters[i].reset();
    	}

    	outputBlock.add(filteredBlock2);

    }

    std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName)
    {
    	crossoverFiltersParams = helpers::ParameterFactory::create("Filters", "Filters");
    	for (auto i = 0ul; i < crossoverFilters.size(); ++i)
    		crossoverFiltersParams->addChild(crossoverFilters[i].createParameters(juce::String{i}, " " + juce::String{i}));
        return parameters.createParameters (factoryID, factoryName);
    }


private:

	void updateFilterParameters()
    {
		const auto minF = juce::jmin(parameters.lowFreq.currentValue, parameters.highFreq.currentValue);
		const auto maxF = juce::jmax(parameters.highFreq.currentValue, parameters.lowFreq.currentValue);
		const auto nXOvers = static_cast<size_t>(parameters.order.currentValue) +1;

		const auto nOctaves = std::log2f(maxF/minF);
		const auto inc = nOctaves/ static_cast<float>(nXOvers-1);

		for (auto i = 0ul; i < nXOvers; i++)
		{
			const auto f = minF * std::pow(2.0f, static_cast<float>(i) *inc);
			crossoverFilters[i].setFrequency(f);
		}

		for (auto i = nXOvers; i < crossoverFilters.size(); i++)
		{
			crossoverFilters[i].setFrequency(maxF);
		}

		auto getPan = [low = parameters.lowAmount.currentValue, high = parameters.highAmount.currentValue, nXOvers](const size_t index){
			if (index == 0 || index >= nXOvers)
				return 0.5f;

			const auto pol = (index - 1) % 2 ? 1.0f : -1.0f;
			const auto end = static_cast<float>(index - 1) / static_cast<float>(nXOvers - 2);
			const auto start = 1.0f - end;


			return 0.5f* (1.0f + pol * ((start *low) + (end*high)));
		};


		for ( auto i = 0ul; i < pannedCrossoverFilters.size(); i++)
			pannedCrossoverFilters[i]->setPanning(getPan(i));
    }

	std::array<CrossoverFilter, maxOrder + 1> crossoverFilters;
	std::array<std::unique_ptr<PannedCrossoverFilter>, maxOrder + 1> pannedCrossoverFilters;

    juce::dsp::ProcessSpec spec{};
	std::unique_ptr<juce::AudioProcessorParameterGroup> crossoverFiltersParams {nullptr};
	juce::AudioBuffer<float> inputBuffer, filteredBuffer, filteredBuffer2;
};

}


