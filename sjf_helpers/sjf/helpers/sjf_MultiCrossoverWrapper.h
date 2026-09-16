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
#include <sjf/helpers/sjf_Passthrough.h>


namespace sjf::helpers
{
namespace crossover
{
	/**
	 * @brief Encapsulates a Linkwitz-Riley crossover or phase-compensation filter stage with parameter smoothing.
	 *
	 * This wrapper manages a 4th-order Linkwitz-Riley filter (`juce::dsp::LinkwitzRileyFilter`), supporting both
	 * dual-output band splits (low-pass / high-pass) and single-input/output all-pass phase alignment stages.
	 * Cutoff frequencies are smoothed per-sample using `juce::LinearSmoothedValue` to eliminate parameter zippering.
	 *
	 * @tparam Compensation If `true`, configures the internal filter as an `allpass` type for phase compensation.
	 *                      If `false`, operates as a main crossover splitting stage.
	 */
	template< bool Compensation = false>
	class Filter
	{
	public:
		void prepare (const juce::dsp::ProcessSpec& spec_)
		{
			spec = spec_;
			if constexpr (Compensation)
				filter.setType(juce::dsp::LinkwitzRileyFilterType::allpass);

			filter.prepare(spec);
			frequency.reset(spec.sampleRate, 0.1f);
			reset();
		}

		void reset()
		{
			frequency.setCurrentAndTargetValue(frequency.getTargetValue());

			filter.setCutoffFrequency(juce::jmin(frequency.getCurrentValue(), static_cast<float>(spec.sampleRate*0.499)));
			filter.reset();
		}


		void process(const juce::dsp::AudioBlock<float>& inputBlock, juce::dsp::AudioBlock<float>& lowBlock, juce::dsp::AudioBlock<float>& highBlock)
		{
			static_assert(!Compensation, "This method is only used for main crossover filters");

			if (frequency.isSmoothing())
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

		template<typename ProcessContext>
		void process(const ProcessContext& context)
		{
			static_assert(Compensation, "This method is only used for the compensation filters");
			if (frequency.isSmoothing())
			{
				processSmoothedState(context);
			}
			else
			{
				filter.process(context);
			}

			#if JUCE_DSP_ENABLE_SNAP_TO_ZERO
			filter.snapToZero();
			#endif
		}

		void setFrequency(const float frequency_)
		{
			frequency.setTargetValue(frequency_);
		}

		float getTargetFrequency() const
		{
			return frequency.getTargetValue();
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
				filter.setCutoffFrequency(juce::jmin(frequency.getNextValue(), static_cast<float>(spec.sampleRate * 0.4999)));

				for (size_t channel = 0; channel < numChannels; ++channel)
				{
					const auto* inputSamples  = inputBlock.getChannelPointer (channel);
					auto* lowSamples = lowBlock.getChannelPointer (channel);
					auto* highSamples = highBlock.getChannelPointer (channel);

					filter.processSample(static_cast<int>(channel), inputSamples[i], lowSamples[i], highSamples[i]);
				}
			}
		}

		template<typename ProcessContext>
		void processSmoothedState (const ProcessContext& context) noexcept
		{
			const auto inputBlock = context.getInputBlock();
			auto outputBlock = context.getOutputBlock();
			const auto numChannels = inputBlock.getNumChannels();
			const auto numSamples  = inputBlock.getNumSamples();


			for (size_t i = 0; i < numSamples; ++i)
			{
				filter.setCutoffFrequency(juce::jmin(frequency.getNextValue(), static_cast<float>(spec.sampleRate * 0.4999)));

				for (size_t channel = 0; channel < numChannels; ++channel)
				{
					const auto* inputSamples  = inputBlock.getChannelPointer (channel);
					auto* outputSamples = outputBlock.getChannelPointer (channel);

					outputSamples[i] = filter.processSample(static_cast<int>(channel), inputSamples[i]);
				}
			}
		}

		juce::dsp::LinkwitzRileyFilter<float> filter;
		juce::dsp::ProcessSpec spec{};
		juce::LinearSmoothedValue<float> frequency;
	};
}

/**
 * @brief A multiband processor wrapper that splits audio across N crossover stages and applies dedicated per-band processing.
 *
 * `MultiCrossoverWrapper` constructs a serial Linkwitz-Riley (LR4) crossover tree to divide incoming audio into `NumBands`
 * distinct frequency ranges, processes each band using an instance of `Processor`, and sums the results back to the output buffer.
 * To ensure phase alignment across all reconstructed bands, lower bands are automatically routed through dedicated, state-isolated
 * All-Pass filter networks corresponding to downstream crossover frequencies.
 *
 * ### Key Features:
 * - **Compile-Time Static Allocation**: Zero dynamic heap allocations during audio thread execution.
 * - **Phase Accuracy**: Dedicated per-band All-Pass networks compensate for phase rotation introduced by higher-order crossovers.
 * - **Smooth Crossover Shifts**: Per-sample frequency smoothing guarantees glitch-free frequency modulation.
 * - **Dynamic or Fixed Band Modes**: Supports fixed frequency layouts or dynamic band enables via template configuration policies.
 *
 * @tparam Processor The DSP class instantiated for each frequency band. Must implement `prepare`, `reset`, `process`, and `createParameters`.
 * @tparam NumBands Total number of frequency bands to generate (\f$\text{NumBands} \ge 2\f$).
 * @tparam FixedFrequencies If `true`, crossover frequencies remain locked to pre-calculated logarithmic defaults.
 * @tparam FixedNumBands If `true`, all `NumBands` are active continuously. If `false`, exposes a runtime parameter to scale active bands.
 */
template <typename Processor, size_t NumBands, bool FixedFrequencies = false, bool FixedNumBands = true>
class MultiCrossoverWrapper
{
	static constexpr auto NumFilters = NumBands - 1;
public:
	static_assert(NumBands >= 2, "You need too have at least two bands! Otherwise whats the point");

    struct Parameters : public helpers::AudioParametersBase
    {
    	std::array<FloatState, NumFilters> filters;
    	[[maybe_unused]] IntState numBands;

        std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) override
        {
        	auto factory = ParameterFactory::create(factoryID, factoryName, true, false);


        	constexpr auto defaultMinF = 50.0f;
        	constexpr auto defaultMaxF = 15000.0f;

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
            		filterParams[i] = createTrackedFrequencyParameter(*factory, filters[i], "XOver" + juce::String{i+1}, "XOver " + juce::String{i+1}, 20.0f, 20000.0f, 2000.0f, defaultF, mapping);
            	}

            }

        	if constexpr (!FixedNumBands && NumBands > 2)
        	{
        		createTrackedParameter(*factory, numBands, "NumBands", "NumBands", 2, NumBands, NumBands);
        	}

            return factory;
        }

    	void prepare (const juce::dsp::ProcessSpec& spec_)
        {
	        AudioParametersBase::prepare (spec_);

        	if constexpr (FixedFrequencies)
        	{
        		constexpr auto defaultMinF = 100.0f;
				constexpr auto defaultMaxF = 10000.0f;

				const auto nOctaves = std::log2f(defaultMaxF/defaultMinF);
				const auto inc = nOctaves/ static_cast<float>(NumFilters-1);
				for (auto i = 0ul; i < NumFilters; i++)
				{
					auto defaultF = defaultMinF * std::pow(2.0f, static_cast<float>(i) *inc);
					filters[i].currentValue = defaultF;
				}
			}
        }

    	float calculateFixedFilterFrequency (const size_t index, const size_t numFilters)
        {
        	constexpr auto defaultMinF = 50.0f;
        	constexpr auto defaultMaxF = 15000.0f;

        	const auto nOctaves = std::log2f(defaultMaxF/defaultMinF);
        	const auto inc = nOctaves/ static_cast<float>(numFilters-1);

        	return defaultMinF * std::pow(2.0f, static_cast<float>(index) *inc);
        }
    private:
    	[[maybe_unused]] std::array<juce::RangedAudioParameter*, NumFilters> filterParams;
    } parameters;

    //==============================================================================
    void prepare (const juce::dsp::ProcessSpec& spec_)
    {
        spec = spec_;
        parameters.prepare (spec);

    	for (auto & processor : processors)
    		processor.prepare (spec);

    	for (auto& filter : filters)
    		filter.prepare (spec);

    	for (auto & band : compensationFilters)
    		for (auto& filter : band)
    			filter.prepare (spec);

    	inputBuffer.setSize(static_cast<int>(spec.numChannels), static_cast<int>(spec.maximumBlockSize));
    	lowBuffer.setSize(static_cast<int>(spec.numChannels), static_cast<int>(spec.maximumBlockSize));
    	highBuffer.setSize(static_cast<int>(spec.numChannels), static_cast<int>(spec.maximumBlockSize));

        reset();
    }

    void reset()
    {
    	parameters.reset();
    	auto numFilters = getNumActiveFilters();
    	for (auto i = 0ul; i < NumFilters; i++)
    	{
    		parameters.filters[i].currentValue = i < numFilters ?
										parameters.calculateFixedFilterFrequency(i, numFilters) :
										parameters.filters[numFilters-1].currentValue;

    		filters[i].setFrequency(parameters.filters[i].currentValue);
    		for (auto j = i+1ul; j < NumFilters; j++)
    		{
    			compensationFilters[i][j].setFrequency(parameters.filters[j].currentValue);
    		}
    	}


    	for (auto & processor : processors)
    		processor.reset();

    	for (auto& filter : filters)
    		filter.reset();

    	for (auto& band : compensationFilters)
    		for (auto& filter : band)
    			filter.reset();
    }

    //==============================================================================
    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
    	const auto inputBlock = context.getInputBlock();
    	auto outputBlock = context.getOutputBlock();

    	const auto prevNumFilters = getNumActiveFilters();
    	if (parameters.checkForStateChange())
    	{
    		parameters.reset(); // filter handles frequency updates manually

    		if (const auto numFilters = getNumActiveFilters(); numFilters != prevNumFilters)
    		{
    			for ( auto i = 0ul; i < NumFilters; i++)
    			{
    				parameters.filters[i].currentValue = i < numFilters ?
															parameters.calculateFixedFilterFrequency(i, numFilters) :
															parameters.filters[numFilters-1].currentValue;
    			}
    		}
    	}


    	for (auto i = 0ul; i < NumFilters; i++)
    	{
    		filters[i].setFrequency(parameters.filters[i].currentValue);
    		for (auto j = i+1ul; j < NumFilters; j++)
    		{
    			compensationFilters[i][j].setFrequency(parameters.filters[j].currentValue);
    		}
    	}


    	auto inBlock = juce::dsp::AudioBlock<float>(inputBuffer).getSubBlock(0, inputBlock.getNumSamples());
    	inBlock.copyFrom(inputBlock);

    	outputBlock.clear();

    	auto lowBlock = juce::dsp::AudioBlock<float>(lowBuffer).getSubBlock(0, inputBlock.getNumSamples());
    	auto highBlock = juce::dsp::AudioBlock<float>(highBuffer).getSubBlock(0, inputBlock.getNumSamples());


    	const auto numFilters = getNumActiveFilters();

    	for (auto i = 0ul; i < numFilters; i++)
    	{
    		auto& filter = filters[i];
    		auto& processor = processors[i];

    		filter.process(inBlock, lowBlock, highBlock);
    		juce::dsp::ProcessContextReplacing<float> processorContext{lowBlock};

    		processor.process(processorContext);

    		for ( auto j = i+1ul; j < numFilters; j++)
    			compensationFilters[i][j].process(processorContext);

    		outputBlock.add(lowBlock);

    		inBlock.copyFrom(highBlock);
    	}

	    {
    		juce::dsp::ProcessContextReplacing<float> processorContext{highBlock};
		    processors[numFilters].process(processorContext);

    		outputBlock.add(highBlock);
	    }

    	for (auto i = numFilters; i < NumFilters; ++i)
    	{
    		filters[i].reset();
    		for (auto j = i+1ul; j < NumFilters; ++j)
    			compensationFilters[i][j].reset();
    	}

    	for (auto i = numFilters+1; i < NumBands; ++i)
    		processors[i].reset();
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

    		factory->addChildFactory (std::move(processorFactory));
    	}


    	for (auto& processor : processors)
    		sjf::optional_calls::attachToSoloSet(processor, &soloSet);

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

	Processor& getProcessor(size_t index)
    {
    	jassert(index < processors.size());
	    return processors[index];
    }


	sjf::dsp::CrossoverFilter& getFilter(size_t index)
    {
    	jassert(index < filters.size());
    	return filters[index];
    }


	[[nodiscard]] size_t getNumFilters() const
    {
	    return NumFilters;
    }

	[[nodiscard]] size_t getNumProcessors() const
    {
	    return NumBands;
    }

private:

	size_t getNumActiveFilters()
	{
		if constexpr (FixedNumBands)
			return NumFilters;

		return static_cast<size_t>(parameters.numBands.currentValue) - 1;
	}

	std::array<Processor, NumBands> processors;
	std::array<crossover::Filter<>, NumFilters> filters;
	std::array<std::array<crossover::Filter<true>, NumFilters>, NumFilters> compensationFilters;
    juce::dsp::ProcessSpec spec{};
	sjf::helpers::Passthrough dummy;
	juce::AudioBuffer<float> inputBuffer, lowBuffer, highBuffer;
	SoloSet soloSet{true};
};

}
