/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 29/07/2026.
//
#pragma once
#include <JuceHeader.h>

#include <sjf/helpers/sjf_ParameterFactory.h>
#include <sjf/helpers/sjf_ProcessorSequenceConfigs.h>

#include <sjf/helpers/sjf_HelperFunctions.h>
#include <sjf/helpers/sjf_OptionalCalls.h>

namespace sjf::helpers
{

/**
 * @brief A compile-time variadic container that holds multiple DSP modules, but executes
 *        only ONE active processor at a time based on a runtime index.
 *
 * Like `ProcessorSequence`, all child modules are stored layout-contiguous in a `std::tuple`
 * with zero runtime allocation overhead. Lifecycle functions (`prepare`, `reset`, `createParameters`,
 * `setPositionInfo`) act across all internal processors, while `process()` and `getLatencySamples()`
 * evaluate dynamically against the currently active index.
 *
 * @tparam Processors Variadic list of class types to compose into the selector pool.
 */
template <typename... Processors>
class ProcessorSelector
{
public:
    static_assert (sizeof...(Processors) > 0, "ProcessorSelector must be instantiated with at least one Processor type!");

	struct Parameters : AudioParametersBase
	{
		ChoiceState selectedProcessor;



		std::unique_ptr<ParameterFactory> createParameters ( const juce::String& factoryID, const juce::String& factoryName, const StringArray& names)
		{
			auto factory = createParameters (factoryID, factoryName);
			createTrackedParameter(*factory, selectedProcessor, "Type", "Type", names, 0);
			return factory;
		}

	private:
		std::unique_ptr<ParameterFactory> createParameters ( const juce::String& factoryID, const juce::String& factoryName)
		{
			return ParameterFactory::create (factoryID, factoryName);
		}

	} parameters;
    /**
        Creates a parent ParameterFactory and recursively builds nested sub-factories
        for each processor in the selector pool.
    */
    template <typename... Configs>
    std::unique_ptr<ParameterFactory> createParameters (
        const juce::String& factoryID,
        const juce::String& factoryName,
        Configs&&... subConfigs)
    {
        static_assert (sizeof...(Configs) == sizeof...(Processors),
            "The number of configuration parameters must match the number of processors!");

        auto mainFactory = parameters.createParameters(factoryID, factoryName, getProcessorNames(subConfigs...));

        if (mainFactory == nullptr)
        {
            jassertfalse;
            return nullptr;
        }

        auto configTuple = std::forward_as_tuple (std::forward<Configs> (subConfigs)...);

        [&]<std::size_t... Is>(std::index_sequence<Is...>)
        {
            (processor_sequence::invokeCreateParameters (
                std::get<Is> (processors),
                mainFactory.get(),
                std::forward<std::tuple_element_t<Is, decltype(configTuple)>> (std::get<Is> (configTuple))
             ), ...);
        }(std::make_index_sequence<sizeof...(Processors)>{});

        return mainFactory;
    }

    //==============================================================================
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
    	processorChangedBuffer.setSize(static_cast<int>(spec.numChannels), static_cast<int>(spec.maximumBlockSize));
    	fadeInSmoother.reset(spec.sampleRate, 0.05f);
    	fadeOutSmoother.reset(spec.sampleRate, 0.05f);
	    sjf::helpers::functions::utilities::forEach (processors, [&spec](auto& proc) { proc.prepare (spec); });
    	for (auto& dl : delayLines)
    	{
    		dl.prepare (spec);
    		dl.setMaximumDelayInSamples(jmax(static_cast<int>(spec.sampleRate), getLatencySamples() * 10));
    	}
    	reset();
    }

    void reset()
    {
    	parameters.reset();
    	fadeInSmoother .setCurrentAndTargetValue(1.0f);
    	fadeOutSmoother.setCurrentAndTargetValue(0.0f);
    	lastActiveIndex = static_cast<size_t>(parameters.selectedProcessor.currentValue);
    	mainDelay = 0;
    	for (auto& dl : delayLines)
    		dl.reset();
    	delayLines[mainDelay].setDelay(getLatencySamples() - getProcessorLatencySamples(static_cast<size_t>(parameters.selectedProcessor.currentValue)));
	    sjf::helpers::functions::utilities::forEach (processors, [](auto& proc) { proc.reset(); });
    }

    /** Executes only the processor corresponding to activeIndex. */
    template <typename ProcessContextType>
    void process (const ProcessContextType& context)
    {
	    {
			const auto latency = getLatencySamples();
	    	for (auto& dl : delayLines)
	    	{
	    		if (dl.getMaximumDelayInSamples() < latency * 10)
	    			dl.setMaximumDelayInSamples(latency * 10);
	    	}
	    }

    	if (!fadeInSmoother.isSmoothing())
    	{

    		const auto previousIndex = parameters.selectedProcessor.currentValue;

    		if (parameters.checkForStateChange() && parameters.selectedProcessor.currentValue != previousIndex)
    		{
    			lastActiveIndex = static_cast<size_t>(previousIndex);
    			mainDelay = (mainDelay + 1) & 1;
    			delayLines[mainDelay].setDelay(getLatencySamples() - getProcessorLatencySamples(static_cast<size_t>(parameters.selectedProcessor.currentValue)));
    			delayLines[mainDelay].reset();
    			fadeInSmoother .setCurrentAndTargetValue(0.0f);
    			fadeOutSmoother.setCurrentAndTargetValue(1.0f);
    			fadeInSmoother .setTargetValue(1.0f);
    			fadeOutSmoother.setTargetValue(0.0f);
    		}
    		else
    		{
    			fadeInSmoother .setCurrentAndTargetValue(1.0f);
    			fadeOutSmoother.setCurrentAndTargetValue(0.0f);
    		}
    	}

    	if (fadeInSmoother.isSmoothing())
    	{
    		jassert(fadeOutSmoother.isSmoothing());
    		auto fadeInBlock = juce::dsp::AudioBlock<float>(processorChangedBuffer).getSubBlock(0, context.getInputBlock().getNumSamples());
    		fadeInBlock.copyFrom(context.getInputBlock());
    		const auto fadeInContext = juce::dsp::ProcessContextReplacing<float>(fadeInBlock);
    		dispatchAndDelay (fadeInContext, static_cast<size_t>(parameters.selectedProcessor.currentValue), mainDelay);
    		fadeInContext.getOutputBlock().multiplyBy(fadeInSmoother);


    		// fade out
    		dispatchAndDelay (context, lastActiveIndex, (mainDelay + 1) & 1);
    		context.getOutputBlock().multiplyBy(fadeOutSmoother);

    		// sum
    		context.getOutputBlock().add(fadeInContext.getOutputBlock());
    	}
    	else
    	{
    		dispatchAndDelay (context, static_cast<size_t>(parameters.selectedProcessor.currentValue), mainDelay);
    	}
    }

    //==============================================================================
    /** Access an individual processor by index at compile-time */
    template <size_t Index> [[nodiscard]] decltype(auto) get() noexcept       { return std::get<Index> (processors); }
    template <size_t Index> [[nodiscard]] decltype(auto) get() const noexcept { return std::get<Index> (processors); }

    void setPositionInfo (const juce::AudioPlayHead::PositionInfo& positionInfo)
    {
        sjf::helpers::functions::utilities::forEach (processors, [&](auto& proc){ sjf::optional_calls::setPositionInfo (proc, positionInfo); });
    }

    /** The maximum latency of all the processors.
     *		Other processors are delayed to align with the maximum latency
     */
    int getLatencySamples() const
    {
        auto maxLatencySamples = 0;

    	sjf::helpers::functions::utilities::forEach (processors, [&](auto& proc){
    		maxLatencySamples = jmax(maxLatencySamples, sjf::optional_calls::getLatencySamples (proc));
    	});



        return maxLatencySamples;
    }


	void attachToState (juce::ValueTree& parentTree)
    {
    	sjf::helpers::functions::utilities::forEach (processors, [&](auto& proc){
    		sjf::optional_calls::attachToState(proc, parentTree);
    	});
    }

private:
	template<typename ProcessContext>
	void dispatchAndDelay(const ProcessContext& context, const size_t index, const size_t delayLineIndex)
	{
		// Unroll compile-time fold to dispatch call to the active index
		[&]<std::size_t... Is>(std::index_sequence<Is...>)
		{
			(void)((Is == index ? (std::get<Is> (processors).process (context), true) : false) || ...);
		}(std::make_index_sequence<sizeof...(Processors)>{});

		delayLines[delayLineIndex].process(context);
	}

	/**
	 * @brief Parses the variadic configuration arguments to generate a juce::StringArray
	 *        containing the names of all child processors.
	 *
	 * Extremely useful for initializing a juce::AudioParameterChoice or juce::ComboBox.
	 */
	template <typename... Configs>
	static juce::StringArray getProcessorNames(const Configs&... subConfigs)
    {
    	static_assert (sizeof...(Configs) == sizeof...(Processors),
			"The number of configuration parameters must match the number of processors!");

    	juce::StringArray names;

    	// C++17 Fold expression to evaluate extractProcessorName for every config in the pack
    	(names.add (extractProcessorName (subConfigs)), ...);

    	return names;
    }

	//==============================================================================
	// Case 1: Extract name from a flat SubFactoryConfig
	template <typename ConfigType>
	requires std::is_same_v<std::decay_t<ConfigType>, processor_sequence::SubFactoryConfig>
	static juce::String extractProcessorName (const ConfigType& config)
    {
    	return config.name;
    }

	// Case 2: Extract name from a NestedConfig token
	template <typename... Args>
	static juce::String extractProcessorName (const processor_sequence::NestedConfig<Args...>& nestedPackage)
    {
    	// Assuming standard convention: (factoryID, factoryName, childConfigs...)
    	// We use if constexpr to safely check the tuple at compile-time to prevent hard errors.
    	if constexpr (sizeof...(Args) >= 2 && std::is_constructible_v<juce::String, std::tuple_element_t<1, std::tuple<Args...>>>)
    	{
    		return juce::String (std::get<1> (nestedPackage.args));
    	}
    	// Fallback: If only 1 argument exists and it's a string, use it
    	else if constexpr (sizeof...(Args) >= 1 && std::is_constructible_v<juce::String, std::tuple_element_t<0, std::tuple<Args...>>>)
    	{
    		return juce::String (std::get<0> (nestedPackage.args));
    	}
    	// Fallback: If we can't deduce the name from the tuple
    	else
    	{
    		return "Nested Processor";
    	}
    }


	int getProcessorLatencySamples(size_t index) const
	{
		int latency = 0;

		[&]<std::size_t... Is>(std::index_sequence<Is...>)
		{
			((Is == index ? void(latency = sjf::optional_calls::getLatencySamples (std::get<Is> (processors))) : void()), ...);
		}(std::make_index_sequence<sizeof...(Processors)>{});

		return latency;

	}

    //==============================================================================
    std::tuple<Processors...> processors;
    size_t lastActiveIndex = 0;
	juce::AudioBuffer<float> processorChangedBuffer;
	juce::LinearSmoothedValue<float> fadeInSmoother, fadeOutSmoother;
	std::array<juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None>, 2> delayLines;
	size_t mainDelay = 0;

};

} // namespace sjf::helpers