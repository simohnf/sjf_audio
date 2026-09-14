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

#include "sjf/helpers/sjf_MultiCrossoverWrapper.h"

namespace sjf::dsp{
class StereoSpread
{
public:
	static constexpr auto maxOrder = 12;

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

    	for (auto& filt : crossoverFilters)
    		filt.prepare(spec);

    	for ( auto& band : compensationFilters)
    		for (auto& filt : band)
    			filt.prepare(spec);

    	for (auto & panner : panners)
    		panner.prepare(spec);

    	inputBuffer.setSize(static_cast<int>(spec.numChannels), static_cast<int>(spec.maximumBlockSize));
    	lowBuffer.setSize(static_cast<int>(spec.numChannels), static_cast<int>(spec.maximumBlockSize));
    	highBuffer.setSize(static_cast<int>(spec.numChannels), static_cast<int>(spec.maximumBlockSize));
        reset();
    }

    void reset()
    {
        parameters.reset();

    	updateFilterParameters();

    	for (auto& filt : crossoverFilters)
    		filt.reset();

    	for ( auto& band : compensationFilters)
    		for (auto& filt : band)
    			filt.reset();

    	for (auto & panner : panners)
    	{
    		panner.setRule(juce::dsp::Panner<float>::Rule::squareRoot3dB);
    		panner.reset();

    	}

    	inputBuffer.clear();
    	lowBuffer.clear();
    	highBuffer.clear();


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
    		parameters.reset();
    		updateFilterParameters();
    	}

    	auto inBlock = juce::dsp::AudioBlock<float>(inputBuffer).getSubBlock(0, inputBlock.getNumSamples());
    	inBlock.copyFrom(inputBlock);

        {
        	constexpr auto sqrtPoint5 = 1.0f / juce::MathConstants<float>::sqrt2;
        	inBlock.getSingleChannelBlock(0).add(inBlock.getSingleChannelBlock(1));
        	inBlock.getSingleChannelBlock(0).multiplyBy(0.5f);
        	inBlock.getSingleChannelBlock(1).copyFrom(inBlock.getSingleChannelBlock(0));
        }

    	outputBlock.clear();

    	auto lowBlock = juce::dsp::AudioBlock<float>(lowBuffer).getSubBlock(0, inputBlock.getNumSamples());
    	auto highBlock = juce::dsp::AudioBlock<float>(highBuffer).getSubBlock(0, inputBlock.getNumSamples());

    	const auto numBands = static_cast<size_t>(parameters.order.currentValue);


    	for (auto i = 0ul; i < numBands; i++)
    	{
    		auto& filter = crossoverFilters[i];
    		auto& panner = panners[i];

    		filter.process(inBlock, lowBlock, highBlock);
    		juce::dsp::ProcessContextReplacing<float> processorContext{lowBlock};

    		panner.process(processorContext);

    		for ( auto j = i+1ul; j < numBands; j++)
    			compensationFilters[i][j].process(processorContext);

    		outputBlock.add(lowBlock);

    		inBlock.copyFrom(highBlock);
    	}

	    {
        	// last block is centred
        	outputBlock.add(highBlock);
	    }

    	for (auto i = numBands; i < crossoverFilters.size(); ++i)
    	{
    		crossoverFilters[i].reset();
    		for ( auto j = i + 1; j < compensationFilters[i].size(); j++)
    			compensationFilters[i][j].reset();
    	}

    	for (auto i = numBands; i < panners.size(); ++i)
    		panners[i].reset();

    }

    std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName)
    {
    	crossoverFiltersParams = helpers::ParameterFactory::create("Filters", "Filters");

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
		for ( auto i= 0ul; i < nXOvers; ++i)
		{
			for ( auto j = i+1ul; j < nXOvers; j++)
			{
				compensationFilters[i][j].setFrequency(crossoverFilters[j].getTargetFrequency());
			}
		}

		auto getPan = [low = parameters.lowAmount.currentValue, high = parameters.highAmount.currentValue, nXOvers](const size_t index){
			if (index == 0 || index >= nXOvers)
				return 0.0f;

			const auto pol = (index - 1) % 2 ? 1.0f : -1.0f;
			const auto end = static_cast<float>(index - 1) / static_cast<float>(nXOvers - 2);
			const auto start = 1.0f - end;


			return pol * ((start *low) + (end*high));
		};


		for ( auto i = 0ul; i < panners.size(); i++)
			panners[i].setPan(getPan(i));
    }

	std::array<helpers::crossover::Filter<>, maxOrder + 1> crossoverFilters;
	std::array<std::array<helpers::crossover::Filter<true>, maxOrder + 1>, maxOrder + 1> compensationFilters;
	std::array<juce::dsp::Panner<float>, maxOrder + 1> panners;


    juce::dsp::ProcessSpec spec{};
	std::unique_ptr<juce::AudioProcessorParameterGroup> crossoverFiltersParams {nullptr};
	juce::AudioBuffer<float> inputBuffer, lowBuffer, highBuffer;


	};

}


