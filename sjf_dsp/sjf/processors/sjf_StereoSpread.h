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

/**
 * @brief A multi-band stereo spreading DSP class that pans intermediate frequency bands across the sound field while leaving the lowest and highest bands centered.
 *
 * This class splits the stereo input signal into multiple frequency bands using a serial chain of Linkwitz-Riley (LR4) crossover filters.
 * To maintain a flat magnitude response (0 dB summation) upon reconstruction, lower bands pass through dedicated downstream All-Pass filter
 * instances to preserve phase alignment with higher bands.
 *
 * Key features and design rules:
 * 1. **Band Layout**: For a user-specified `order` (\f$N \ge 2\f$), the processor constructs \f$N + 1\f$ crossover filters and \f$N + 2\f$ output bands.
 * 2. **Centered Boundaries**: The lowest band (index 0) and the highest band (index \f$N + 1\f$) are strictly locked to center (Pan = 0.0).
 * 3. **Interleaved Stereo Width**: Intermediate bands (\f$1 \dots N\f$) alternate pan polarity (Left/Right) while interpolating pan position smoothly between `lowAmount` and `highAmount`.
 * 4. **State-Isolated All-Pass Compensation**: Downstream All-Pass filters maintain dedicated state buffers per band and per crossover index, avoiding memory state corruption across bands during buffer processing.
 *
 * @note Input signals must be stereo (`numChannels == 2`). Mono summing is applied internally before the multiband split.
 */
class StereoSpread
{
public:
	static constexpr auto maxOrder = 12;
	static constexpr auto maxNumFilters = maxOrder + 1;
	static constexpr auto maxNumBands = maxNumFilters + 1;

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
        	inBlock.getSingleChannelBlock(0).add(inBlock.getSingleChannelBlock(1));
        	inBlock.getSingleChannelBlock(0).multiplyBy(0.5f);
        	inBlock.getSingleChannelBlock(1).copyFrom(inBlock.getSingleChannelBlock(0));
        }

    	outputBlock.clear();

    	auto lowBlock = juce::dsp::AudioBlock<float>(lowBuffer).getSubBlock(0, inputBlock.getNumSamples());
    	auto highBlock = juce::dsp::AudioBlock<float>(highBuffer).getSubBlock(0, inputBlock.getNumSamples());

    	const auto numXOvers = static_cast<size_t>(parameters.order.currentValue) + 1;
    	const auto numBands  = numXOvers + 1;


    	for (auto i = 0ul; i < numXOvers; i++)
    	{
    		auto& filter = crossoverFilters[i];
    		auto& panner = panners[i];

    		filter.process(inBlock, lowBlock, highBlock);
    		const juce::dsp::ProcessContextReplacing<float> processorContext{lowBlock};

    		panner.process(processorContext);

    		for ( auto j = i+1ul; j < numXOvers; j++)
    			compensationFilters[i][j].process(processorContext);

    		outputBlock.add(lowBlock);

    		inBlock.copyFrom(highBlock);
    	}

	    {
        	// last block is centred
			const juce::dsp::ProcessContextReplacing<float> processorContext{highBlock};
        	panners[numBands-1].process(processorContext);
        	outputBlock.add(highBlock);
	    }

    	for (auto i = numXOvers; i < crossoverFilters.size(); ++i)
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
		const auto nXOvers = static_cast<size_t>(parameters.order.currentValue) + 1;
		const auto nBands = nXOvers + 1;

		const auto nOctaves = std::log2f(maxF/minF);
		const auto inc = nOctaves/ static_cast<float>(nXOvers-1);

		for (auto i = 0ul; i < nXOvers; i++)
		{
			const auto f = minF * std::pow(2.0f, static_cast<float>(i) *inc);
			crossoverFilters[i].setFrequency(f);
		}

		for (auto i = nXOvers; i < maxNumFilters; i++)
		{
			crossoverFilters[i].setFrequency(maxF);
		}

		for ( auto i= 0ul; i < maxNumFilters; ++i)
		{
			for ( auto j = i+1ul; j < maxNumFilters; j++)
			{
				compensationFilters[i][j].setFrequency(crossoverFilters[j].getTargetFrequency());
			}
		}

		auto getPan = [low = parameters.lowAmount.currentValue, high = parameters.highAmount.currentValue, nXOvers, nBands](const size_t index){
			if (index == 0 || index >= nBands-1)
				return 0.0f;

			const auto pol = (index - 1) % 2 ? 1.0f : -1.0f;
			const auto end = static_cast<float>(index - 1) / static_cast<float>(nXOvers - 2);
			const auto start = 1.0f - end;


			return pol * ((start *low) + (end*high));
		};


		for ( auto i = 0ul; i < panners.size(); i++)
			panners[i].setPan(getPan(i));
    }

	std::array<helpers::crossover::Filter<>, maxNumFilters> crossoverFilters;
	std::array<std::array<helpers::crossover::Filter<true>, maxNumFilters>, maxNumFilters> compensationFilters;
	std::array<juce::dsp::Panner<float>, maxNumBands> panners;


    juce::dsp::ProcessSpec spec{};
	std::unique_ptr<juce::AudioProcessorParameterGroup> crossoverFiltersParams {nullptr};
	juce::AudioBuffer<float> inputBuffer, lowBuffer, highBuffer;


	};

}


