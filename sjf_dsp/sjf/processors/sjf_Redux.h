/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 16/08/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_ParameterFactory.h>

#include "sjf/helpers/sjf_BypassWrapper.h"
#include "sjf_Filter_juce.h"

namespace sjf::dsp{
/**
 * @brief A simple fixed-point audio bit-crusher processor.
 *
 * Quantizes continuous 32-bit floating-point audio samples into a reduced bit depth (1 to 16 bits),
 * introducing controlled bit-resolution reduction noise and quantization distortion.
 */
class BitCrush
{
public:
    struct Parameters : public helpers::AudioParametersBase
    {
        IntState    bits;

        std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) override
        {
            auto factory = helpers::ParameterFactory::create (factoryID, factoryName);
        	createTrackedParameter(*factory, bits, "Bits", "Bits", 1, 16, 16, [](const int x){
        		return static_cast<int>(std::pow(2, x - 1));
        	});
            return factory;
        }
    } parameters;


    void prepare (const juce::dsp::ProcessSpec& spec_)
    {
        spec = spec_;
        parameters.prepare(spec);
        reset();
    }

    void reset()
    {
        parameters.reset();
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

    	if constexpr (ProcessContext::usesSeparateInputAndOutputBlocks())
    		context.getOutputBlock().copyFrom(context.getInputBlock());

    	auto processBlock = context.getOutputBlock();
    	parameters.checkForStateChange();


    	const auto bits = static_cast<float>(parameters.bits.currentValue);
    	const auto invBits = 1.0f / bits;
    	processBlock.multiplyBy(bits);
    	juce::dsp::AudioBlock<float>::process(processBlock, processBlock, [](const float x){
    		return static_cast<float>(roundToInt(x));
    	});
    	processBlock.multiplyBy(invBits);
    }

    std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName)
    {
        return parameters.createParameters (factoryID, factoryName);
    }

private:
    juce::dsp::ProcessSpec spec{};
};


/**
 * @brief A sample-and-hold downsampler processor with optional anti-aliasing filters and clock jitter.
 *
 * Performs sample rate reduction (decimation) using a phase accumulator. Optionally applies
 * low-pass SVF anti-aliasing pre/post-filtering and clock phase jitter.
 *
 * @tparam AddFilters Enables input and output low-pass filtering when `true`.
 * @tparam AddJitter Enables pseudo-random clock jitter on the accumulator when `true`. Defaults to `false`.
 */
template<bool AddFilters, bool AddJitter = false>
class Downsampler
{
public:
    struct Parameters : public helpers::AudioParametersBase
    {
        FloatState    sampleRate; // this ara,eter is actually mapped to the per sample phase increase
    	[[maybe_unused]] FloatState outputFilterOctave, jitter;
    	[[maybe_unused]] BoolState inputFilter, outputFilter;
		std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) override
        {
            auto factory = helpers::ParameterFactory::create (factoryID, factoryName);
			{
				auto range = juce::NormalisableRange<float> {20.0f, 40000.0f, 0.01f};
            	range.setSkewForCentre(5000.0f);
            	const auto att  = juce::AudioParameterFloatAttributes{}.withLabel("Hz");
            	const auto mapping = [this, range](const float x){
            		const auto sr = juce::jmin(static_cast<float>(spec.sampleRate), range.end);
            		if (x >= sr)
            			return 1.0f;
            		return x / sr;
            	};
            	createTrackedParameter(*factory, sampleRate, "SR", "Sample Rate", range, 40000.0f, mapping, att);
			}

			if constexpr (AddFilters)
			{
				createTrackedParameter(*factory, inputFilter, "InFilt", "Input Filter", false);
				createTrackedParameter(*factory, outputFilter, "OutFilt", "Output Filter", false);
				const auto range = juce::NormalisableRange<float> {-4.0f, 4.0f, 0.01f};
				const auto att  = juce::AudioParameterFloatAttributes{};
				const auto mapping = [this](const float x){
					const auto mult = std::pow(2.0f, x);
					return juce::jmin(static_cast<float>(spec.sampleRate * 0.49f), sampleRate.getParameterValue() * mult);
				};
				createTrackedParameter(*factory, outputFilterOctave, "OutFilt8ve", "Output Filter Octave", range, 0.0f, mapping, att);
			}

			if constexpr (AddJitter)
			{
				const auto att  = juce::AudioParameterFloatAttributes{}.withLabel("%");
				createTrackedParameter(*factory, jitter, "Jitter", "Jitter", {0.0f, 100.0f, 0.01f}, 0.0f, [](const float x){ return x * 0.01f * 0.25f;}, att);
			}

            return factory;
        }
    } parameters;


    void prepare (const juce::dsp::ProcessSpec& spec_)
    {
        spec = spec_;
        parameters.prepare(spec);
    	output.resize(spec.numChannels, 0);

    	if constexpr (AddFilters)
    	{
    		inputFilter.prepare(spec);
    		outputFilter.prepare(spec);
    	}

        reset();
    }

    void reset()
    {
        parameters.reset();
    	phase = 0;
    	std::ranges::fill(output, 0.0f);

    	if constexpr (AddFilters)
    	{
    		inputFilter.reset();
    		outputFilter.reset();
    	}
    	if constexpr (AddJitter)
    		rnd.setSeedRandomly();
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

    	if constexpr (ProcessContext::usesSeparateInputAndOutputBlocks())
    	{
    		context.getOutputBlock().copyFrom(context.getInputBlock());
    	}

		const juce::dsp::ProcessContextReplacing<float> replacingContext(context.getOutputBlock());
    	if constexpr (AddFilters)
    	{
    		if (parameters.inputFilter.currentValue)
    			inputFilter.process(replacingContext);
    		else
    			inputFilter.reset();
    	}

    	if (parameters.checkForStateChange())
    	{
    		processInternal<true>(replacingContext.getOutputBlock());
    	}
    	else
    	{
    		if (parameters.sampleRate.currentValue < 1.0f)
				processInternal<false>(replacingContext.getOutputBlock());
    	}

    	if constexpr (AddFilters)
    	{
    		if (parameters.outputFilter.currentValue)
				outputFilter.process(replacingContext);
    		else
    			outputFilter.reset();
    	}
    }

    std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName)
    {
        auto factory = parameters.createParameters (factoryID, factoryName);

    	if constexpr (AddFilters)
    	{
    		dummyGroup = inputFilter.createParameters("Dummy", "Dummy");
    		dummyGroup->addChild(outputFilter.createParameters("Out", "Out"));

    		inputFilter.getProcessor().parameters.updateMapping(inputFilter.getProcessor().parameters.cutoff, [this](float){
    			return juce::jmax(0.0f, parameters.sampleRate.getParameterValue() * 0.49f);
    		});
    		inputFilter.parameters.updateMapping(inputFilter.parameters.bypass, [this](bool){ return !static_cast<bool>(parameters.inputFilter.getParameterValue());});


    		const auto outFilterMapping = [this](float){
    			const auto mult = std::pow(2.0f, parameters.outputFilterOctave.getParameterValue());
    			return juce::jlimit(0.0f, static_cast<float>(spec.sampleRate * 0.49f), parameters.sampleRate.getParameterValue() * mult);
    		};
    		outputFilter.getProcessor().parameters.updateMapping(outputFilter.getProcessor().parameters.cutoff, outFilterMapping);
    		outputFilter.parameters.updateMapping(outputFilter.parameters.bypass, [this](bool){ return !parameters.outputFilter.getParameterValue();});
    	}
    	return factory;
    }

private:
	template<bool IsSmoothing>
	void processInternal(const juce::dsp::AudioBlock<float> processBlock)
	{
		const auto numSamples = processBlock.getNumSamples();
		const auto numChannels = processBlock.getNumChannels();
		for (auto i = 0ul; i < numSamples; i++)
		{
			if constexpr (IsSmoothing)
				parameters.tickSmoothers();

			if (phase < 1.0f)
			{
				for (auto c = 0ul; c < numChannels; c++)
					processBlock.getChannelPointer(c)[i] = output[c];
			}
			else
			{
				for (auto c = 0ul; c < numChannels; c++)
					output[c] = processBlock.getChannelPointer(c)[i];
				phase = sjf::helpers::functions::waveforms::wrapPhase(phase);
			}

			if constexpr (AddJitter)
			{
				const auto jitter = parameters.jitter.currentValue * rnd.nextFloat();
				phase += parameters.sampleRate.currentValue + parameters.sampleRate.currentValue * jitter;
			}
			else
			{
				phase += parameters.sampleRate.currentValue;
			}

		}
	}

	using Filter = sjf::dsp::SVF<FixedFilterType::LowPass, true, filter_config::FrequencyRange<>, false>;
	[[maybe_unused]] sjf::helpers::BypassWrapper<Filter, helpers::bypass_wrapper_config::OnOff> inputFilter, outputFilter;
	[[maybe_unused]] std::unique_ptr<AudioProcessorParameterGroup> dummyGroup{nullptr};
	[[maybe_unused]] juce::Random rnd;
    juce::dsp::ProcessSpec spec{};
	float phase{};
	std::vector<float> output{};
};

class Redux
{
public:
	struct Parameters : public sjf::helpers::AudioParametersBase
	{
		std::unique_ptr<sjf::helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) override
		{
			return helpers::ParameterFactory::create(factoryID, factoryName, true, false);
		}
	} parameters;

	void prepare(const juce::dsp::ProcessSpec& spec)
	{
		bitCrush.prepare(spec);
		downsampler.prepare(spec);
	}

	void reset()
	{
		bitCrush.reset();
		downsampler.reset();
	}

	template<typename ProcessContext>
	void process(const ProcessContext& context)
	{
		if constexpr (ProcessContext::usesSeparateInputAndOutputBlocks())
			context.getOutputBlock().copyFrom(context.getInputBlock());
		const juce::dsp::ProcessContextReplacing<float> contextReplacing(context.getOutputBlock());

		downsampler.process(contextReplacing);
		bitCrush.process(contextReplacing);
	}

	std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName)
	{
		auto factory = parameters.createParameters(factoryID, factoryName);
		factory->addChildFactory(downsampler.createParameters(factoryID+"DS", factoryName + " Downsampler"));
		factory->addChildFactory(bitCrush.createParameters(factoryID+"BC", factoryName + " Bit Crusher"));

		return factory;
	}

private:
	template<typename Processor>
	struct Wrapper :public sjf::helpers::BypassWrapper<Processor, helpers::bypass_wrapper_config::Bypass, helpers::bypass_wrapper_config::Mix>{};

	Wrapper<BitCrush> bitCrush;
	Wrapper<Downsampler<true, true>> downsampler;
};

}


