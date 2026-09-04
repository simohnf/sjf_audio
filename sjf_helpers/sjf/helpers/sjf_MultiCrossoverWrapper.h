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
#include <sjf/helpers/sjf_OptionalCalls.h>

#include "sjf/processors/sjf_CrossoverFilter.h"
#include "sjf_Passthrough.h"

namespace sjf::helpers
{

template <typename Processor, size_t NumBands, bool FixedFrequencies = false, bool AddBandSolo = false>
class MultiCrossoverWrapper
{
	static constexpr auto NumFilters = NumBands - 1;
public:
	static_assert(NumBands >= 2, "You need too have at least two bands! Otherwise whats the point");

    struct Parameters : public helpers::AudioParametersBase
    {
    	std::array<FloatState, NumFilters> filters;
    	[[maybe_unused]] std::array<BoolState, NumBands> solos;

        std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) override
        {
        	auto factory = ParameterFactory::create(factoryID, factoryName, true, false);

        	ParameterFactory* factoryToUse = factory.get();


        	constexpr auto defaultMinF = 100.0f;
        	constexpr auto defaultMaxF = 10000.0f;

            const auto nOctaves = std::log2f(defaultMaxF/defaultMinF);
            const auto inc = nOctaves/ static_cast<float>(NumFilters-1);
            for (auto i = 0ul; i < NumFilters; i++)
            {
            	auto defaultF = defaultMinF * std::pow(2.0f, static_cast<float>(i) *inc);
            	if constexpr (FixedFrequencies)
            	{
            		filters[i].currentValue = defaultF;
            	}
            	else
            	{
            		auto mapping = [&, i](const float x){
            			if (i == 0 || x > filters[i-1].getParameterValue())
            				return x;
            			return jmin(static_cast<float>(spec.sampleRate) * 0.5f, filters[i].getParameterValue() + 1.0f);
            		};
            		createTrackedFrequencyParameter(*factoryToUse, filters[i], "XOver" + juce::String{i+1}, "XOver " + juce::String{i+1}, 20.0f, 20000.0f, 2000.0f, defaultF, mapping);
            	}
            }

        	if constexpr(AddBandSolo)
        	{
        		soloSet.setRange(0, NumBands, false);
        		lastSoloSet = soloSet;
        	}

            return factory;
        }


    	void createSoloParameter(helpers::ParameterFactory& factory, const size_t index, std::array<RangedAudioParameter*, NumBands>& soloParams_)
        {
        	auto mapping = [this, index](const bool x){
        		soloSet.setBit(static_cast<int>(index), x);
        		return x;
        	};
	        soloParams_[index] = createTrackedParameter(factory, solos[index], "Solo", "Solo", false, mapping);
        }

    	[[maybe_unused]] juce::BigInteger soloSet, lastSoloSet;
    } parameters;

    //==============================================================================
    void prepare (const juce::dsp::ProcessSpec& spec_)
    {
        spec = spec_;
        parameters.prepare (spec);

    	for (auto & processor : processors)
    		processor.prepare (spec);

    	for (auto& muter : muters)
    		muter.prepare (spec);

    	for (auto& filter : filters)
    		filter.prepare (spec);

    	inputBuffer.setSize(static_cast<int>(spec.numChannels), static_cast<int>(spec.maximumBlockSize));
    	lowBuffer.setSize(static_cast<int>(spec.numChannels), static_cast<int>(spec.maximumBlockSize));
    	highBuffer.setSize(static_cast<int>(spec.numChannels), static_cast<int>(spec.maximumBlockSize));

        reset();
    }

    void reset()
    {
    	parameters.reset();
    	for (auto i = 0ul; i < NumFilters; i++)
    		filters[i].setFrequency(parameters.filters[i].currentValue);

    	for (auto & processor : processors)
    		processor.reset();

    	if constexpr(AddBandSolo)
    	{
    		setMutes();
    	}

    	for (auto& muter : muters)
    		muter.reset();

    	for (auto& filter : filters)
    		filter.reset();


    }

    //==============================================================================
    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
    	const auto inputBlock = context.getInputBlock();
    	auto outputBlock = context.getOutputBlock();

    	if (parameters.checkForStateChange())
    	{
    		parameters.reset();
    		for (auto i = 0ul; i < NumFilters; i++)
    			filters[i].setFrequency(parameters.filters[i].currentValue);

    		if constexpr(AddBandSolo)
    		{
    			if (parameters.soloSet != parameters.lastSoloSet)
    				setMutes();
    		}
    	}


    	auto inBlock = juce::dsp::AudioBlock<float>(inputBuffer).getSubBlock(0, inputBlock.getNumSamples());
    	inBlock.copyFrom(inputBlock);

    	outputBlock.clear();

    	auto lowBlock = juce::dsp::AudioBlock<float>(lowBuffer).getSubBlock(0, inputBlock.getNumSamples());
    	auto highBlock = juce::dsp::AudioBlock<float>(highBuffer).getSubBlock(0, inputBlock.getNumSamples());

    	for (auto i = 0ul; i < NumFilters; i++)
    	{
    		auto& filter = filters[i];
    		auto& processor = processors[i];

    		filter.process(inBlock, lowBlock, highBlock);
    		juce::dsp::ProcessContextReplacing<float> processorContext{lowBlock};
    		processor.process(processorContext);

    		if constexpr(AddBandSolo)
    			muters[i].process(processorContext);

    		outputBlock.add(lowBlock);

    		inBlock.copyFrom(highBlock);
    	}

	    {
    		juce::dsp::ProcessContextReplacing<float> processorContext{highBlock};
		    processors[NumFilters].process(processorContext);

    		if constexpr(AddBandSolo)
    			muters[NumFilters].process(processorContext);

    		outputBlock.add(highBlock);
	    }
    }

    //==============================================================================
    /**
        Generates the internal processor's parameter factory, hands it to our local
        parameters object to append the bypass state, and cleans up the transient pointer.
    */
    template <typename... Args>
    std::unique_ptr<helpers::ParameterFactory> createParameters (
        const juce::String& factoryID,
        const juce::String& factoryName)
    {

        // 1. Ask the wrapped processor to generate its parameter factory layout first
        auto factory = parameters.createParameters (factoryID, factoryName);

        if (factory == nullptr)
        {
            jassertfalse;
            return nullptr;
        }

    	for (auto i = 0ul; i < processors.size(); i++)
    	{
    		auto& processor = processors[i];
    		auto processorFactory = processor.createParameters(factoryID + "B" + juce::String{i+1}, factoryName + " Band " + juce::String{i+1});

    		if constexpr (AddBandSolo)
    		{
    			muteParams.addChild(muters[i].createParameters("Mute"+juce::String(i), "Mute"+juce::String(i)));
    			parameters.createSoloParameter(*processorFactory, i, soloParams);
    		}

    		factory->addChildFactory (std::move(processorFactory));
    	}

    	filterParams = dummy.createParameters ("Filters", "Filters");

    	for (auto i = 0ul; i < filters.size(); i++)
    	{
    		auto& filter = filters[i];
    		filterParams->addChildFactory (filter.createParameters("Filters" + juce::String{i+1}, "Filters " + juce::String{i+1}));
    	}


        return factory;
    }

    void setPositionInfo(const juce::AudioPlayHead::PositionInfo& positionInfo)
    {
    	for (auto & processor : processors)
    		sjf::optional_calls::setPositionInfo(processor, positionInfo);

    	for (auto& filter : filters)
    		sjf::optional_calls::setPositionInfo(filter, positionInfo);

    }

	int getLatencySamples()
    {
    	return sjf::optional_calls::getLatencySamples(processors[0]);
    }

	void attachToState (juce::ValueTree& parentTree)
    {
    	for (auto & processor : processors)
			sjf::optional_calls::attachToState(processor, parentTree);
    }
private:
	void setMutes()
	{
		auto diff = parameters.soloSet>0 ? parameters.soloSet ^ parameters.lastSoloSet : 0;
		auto index = diff.getHighestBit();
		const auto mutes = muteParams.getParameters(true);
		jassert(mutes.size() == NumBands);
		for (auto i = 0ul; i < NumBands; i++)
		{
			jassert(soloParams[i]);
			const auto solo = index >= 0 && index == static_cast<int>(i);
			const auto mute = index >= 0 && index != static_cast<int>(i);
			soloParams[i]->setValueNotifyingHost(solo);
			mutes[static_cast<int>(i)]->setValueNotifyingHost(mute);
		}

		parameters.lastSoloSet = parameters.soloSet;
	}

	std::array<Processor, NumBands> processors;
	std::array<sjf::dsp::CrossoverFilter, NumFilters> filters;
	std::unique_ptr<sjf::helpers::ParameterFactory> filterParams;
    juce::dsp::ProcessSpec spec{};
	juce::AudioProcessorParameterGroup muteParams;
	[[maybe_unused]] std::array<juce::RangedAudioParameter*, NumBands> soloParams;
	[[maybe_unused]] std::array<helpers::BypassWrapper<helpers::Passthrough, bypass_wrapper_config::Mute>, NumBands> muters;
	sjf::helpers::Passthrough dummy;
	juce::AudioBuffer<float> inputBuffer, lowBuffer, highBuffer;
};

}
