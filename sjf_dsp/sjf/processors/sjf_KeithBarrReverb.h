/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 24/07/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_ParameterFactory.h>
#include <sjf/helpers/sjf_DelayLine.h>
#include "sjf/oscillators/sjf_SinCos.h"


namespace sjf::dsp::keith_barr::reverb
{
namespace config
{
    enum class ProcessType
    {
        AllPass,
        Delay,
        FBComb,
        LowPass
    };

    struct AllPass
    {
        static constexpr auto type = ProcessType::AllPass;

        template<sjf::interpolation::InterpolatorTypes InterpType>
        static forcedinline float process(sjf::helpers::MultiTapRingBuffer& delayLine, const float input,
										  const float offset, const float length, const float feedback) noexcept
        {
            return delayLine.applyAllPass<InterpType>(input, offset, length, feedback);
        }
    };

    struct Delay
    {
        static constexpr auto type = ProcessType::Delay;

        template<sjf::interpolation::InterpolatorTypes InterpType>
        static forcedinline float process(sjf::helpers::MultiTapRingBuffer& delayLine, const float input,
        								  const float offset, const float length) noexcept
        {
            return delayLine.applyDelay<InterpType>(input, offset, length);
        }
    };

    struct FBComb
    {
        static constexpr auto type = ProcessType::FBComb;

        template<sjf::interpolation::InterpolatorTypes InterpType>
        static forcedinline float process(sjf::helpers::MultiTapRingBuffer& delayLine, const float input,
										  const float offset, const float length, const float feedback) noexcept
        {
            return delayLine.applyDelay<InterpType>(input, offset, length, feedback);
        }
    };

    struct LowPass
    {
        static constexpr auto type = ProcessType::LowPass;

        template<sjf::interpolation::InterpolatorTypes InterpType>
        static forcedinline float process(sjf::helpers::MultiTapRingBuffer& delayLine, const float input,
										  const float offset, const float alpha) noexcept
        {
            return delayLine.applyLowPass(input, static_cast<size_t>(juce::roundToInt(offset)), alpha);
        }
    };

    template<typename... Processes>
    struct StageConfig
    {
        static constexpr size_t numProcesses = sizeof...(Processes);
        using ProcessesTuple = std::tuple<Processes...>;

        static constexpr std::array<ProcessType, numProcesses> processTypes = { Processes::type... };
    };
}

template<typename StageConfiguration = config::StageConfig<config::AllPass, config::AllPass, config::Delay, config::LowPass>, size_t NumStages = 4>
class Reverb
{

	static constexpr auto ProcessesPerStage = StageConfiguration::numProcesses;
	static constexpr auto TotalNumProcesses = NumStages * ProcessesPerStage;

	// Helper to construct the full compile-time array across all stages
	static constexpr std::array<config::ProcessType, TotalNumProcesses> processTypes = []() {
		std::array<config::ProcessType, TotalNumProcesses> arr{};
		for (size_t stage = 0; stage < NumStages; ++stage)
		{
			for (size_t proc = 0; proc < ProcessesPerStage; ++proc)
			{
				arr[stage * ProcessesPerStage + proc] = StageConfiguration::processTypes[proc];
			}
		}
		return arr;
	}();

	template<config::ProcessType TypeToCount>
	static constexpr size_t NumOfTypePerStage()
	{
		size_t count = 0;
		for (auto type : StageConfiguration::processTypes)
			if (type == TypeToCount)
				count++;
		return count;
	}

	static constexpr size_t NumAllpassPerStage = NumOfTypePerStage<config::ProcessType::AllPass>();
	static constexpr size_t NumDelayPerStage = NumOfTypePerStage<config::ProcessType::Delay>();
	static constexpr size_t NumFBCombPerStage = NumOfTypePerStage<config::ProcessType::FBComb>();
	static constexpr size_t NumLowpassPerStage = NumOfTypePerStage<config::ProcessType::LowPass>();


	static constexpr size_t NumScaleableStages = (NumFBCombPerStage + NumDelayPerStage) * NumStages;

	// Compile-time array containing ONLY the indices of Delay/FBComb processes
	static constexpr std::array<size_t, NumScaleableStages> scaleableIndices = []() {
		std::array<size_t, NumScaleableStages> indices{};
		size_t writeIdx = 0;
		for (size_t i = 0; i < TotalNumProcesses; ++i)
		{
			if (processTypes[i] == config::ProcessType::Delay || processTypes[i] == config::ProcessType::FBComb)
				indices[writeIdx++] = i;
		}
		return indices;
	}();


public:
    struct Parameters : public helpers::AudioParametersBase
    {
        FloatState  size, damping, decay, diffusion, modulation;



        std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) override
        {
            auto factory = helpers::ParameterFactory::create (factoryID, factoryName);
        	createTrackedParameter(*factory, size, "Size", "Size", {0.0f, 100.0f, 0.01f}, 50.0f, [](const float x){return pow(2.0f, (2.0f*x*0.01f)-1.0f);});
        	auto fRange = NormalisableRange<float>{100.0f, 20000.0f, 0.01f};
        	fRange.setSkewForCentre(1000.0f);
        	createTrackedParameter(*factory, damping, "Damping", "Damping", fRange, 2000.0f, [&](const float x){
    			return sin(x*juce::MathConstants<float>::twoPi/spec.sampleRate);
        	});
        	createTrackedParameter(*factory, decay, "Decay", "Decay", {0.0f, 100.0f, 0.01f}, 50.0f, [](const float x){ return pow(x*0.01f, 0.5f)*0.99f;});
        	createTrackedParameter(*factory, diffusion, "Diffusion", "Diffusion", {0.0f, 100.0f, 0.01f}, 50.0f, [&](const float x){return 0.25f + x*0.01f*0.249f;});
        	createTrackedParameter(*factory, modulation, "Modulation", "Modulation", {0.0f, 100.0f, 0.01f}, 50.0f, [&](const float x){return x*0.005f;});
            return factory;
        }
    } parameters;


    void prepare (const juce::dsp::ProcessSpec& spec_)
    {
        spec = spec_;
        parameters.prepare(spec);
    	parameters.setSmootherLength(100.0f);
    	delayLine.prepare(spec);

    	inputScaling = 1.0f / (sqrtf(NumStages) * sqrt(2.0f));
    	for (auto i = 0ul; i < NumStages; i++)
    	{
    		auto& m = modulators[i];
    		m.prepare(spec);
    		m.setFrequency(jmap(static_cast<float>(i) / static_cast<float>(NumStages), 0.1f, 0.75f));
    	}

    	calculateDelayTimes();
    	std::copy(delayTimes.begin(), delayTimes.end(), delayTimesResized.begin());

    	delayLine.setMaxDelayTimeSamps(static_cast<int>(std::accumulate(delayTimes.begin(), delayTimes.end(), 0.0f) * 2.0f + 3.0f));

        reset();
    }

    void reset()
    {
        parameters.reset();
    	for (auto& m : modulators)
    		m.reset();
    	delayLine.reset();
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


    	if (ProcessContext::usesSeparateInputAndOutputBlocks())
    		outputBlock.copyFrom(inputBlock);

    	for ( auto i = 1ul; i < numChannels; ++i)
    		outputBlock.getSingleChannelBlock(0).add(outputBlock.getSingleChannelBlock(i));

    	outputBlock.multiplyBy(inputScaling);

    	juce::dsp::ProcessContextReplacing<float> reverbContext (outputBlock);
        if (parameters.checkForStateChange())
        {
            processSmoothedState(reverbContext);
        }
        else
        {
            processStaticState(reverbContext);
        }
    	outputBlock.multiplyBy(outputScaling);
    }

    std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName)
    {
        return parameters.createParameters (factoryID, factoryName);
    }


    // void setPositionInfo(const juce::AudioPlayHead::PositionInfo& positionInfo_)
    // {
    // }

private:
    template <typename ProcessContext>
    void processStaticState (const ProcessContext& context) noexcept
    {
        const auto& inputBlock = context.getInputBlock();
        auto& outputBlock      = context.getOutputBlock();
        const auto numChannels = outputBlock.getNumChannels();
        const auto numSamples  = outputBlock.getNumSamples();


        auto* inputSamples  = inputBlock.getChannelPointer (0);

    	calculateResizedDelayTimesAndOffsets();

    	for (size_t i = 0; i < numSamples; ++i)
    	{
    		processSample<sjf::interpolation::InterpolatorTypes::none>(inputSamples[i]);

    		mixTapsForOutput(outputBlock, numChannels, i);
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

    	auto* inputSamples  = inputBlock.getChannelPointer (0);

    	if (parameters.size.isSmoothing())
    	{
    		for (size_t i = 0; i < numSamples; ++i)
    		{
    			parameters.tickSmoothers();
    			calculateResizedDelayTimesAndOffsets();
    			processSample<sjf::interpolation::InterpolatorTypes::cubic>(inputSamples[i]);

    			mixTapsForOutput(outputBlock, numChannels, i);
    		}
    	}
    	else
    	{
    		for (size_t i = 0; i < numSamples; ++i)
    		{
    			parameters.tickSmoothers();
    			calculateResizedDelayTimesAndOffsets();
    			processSample<sjf::interpolation::InterpolatorTypes::none>(inputSamples[i]);

    			mixTapsForOutput(outputBlock, numChannels, i);
    		}
    	}
    }

	template<sjf::interpolation::InterpolatorTypes InterpType>
	void processSample(float input)
    {
    	auto sample = taps.back() * parameters.decay.currentValue;
    	input *= inputScaling;

    	auto modFold = [](const float x){
    		if (x < 0.0f)
    			return -1.0f * x;
    		if (x > 1.0f)
    			return 1.0f - (x - 1.0f);
    		return x;
    	};

    	const auto diffusion = parameters.diffusion.currentValue;
    	const auto modDepth = parameters.modulation.currentValue;

    	auto processNum = 0ul;

    	for ( auto i = 0ul; i < NumStages; ++i)
    	{
    		using namespace config;

    		sample += input; // each stage adds the input again
    		auto mod_ = modulators[i]();
    		const auto mod = (i & 1 ? mod_.sinOut : mod_.cosOut) * modDepth;
    		if constexpr (NumAllpassPerStage > 0)
    		{
    			sample = AllPass::process<interpolation::InterpolatorTypes::none>(delayLine, sample, delayOffsets[processNum], delayTimesResized[processNum], modFold(diffusion + mod));
    			processNum++;
    			for ( auto j = 1ul; j < NumAllpassPerStage; ++j)
    			{
    				sample = AllPass::process<interpolation::InterpolatorTypes::none>(delayLine, sample, delayOffsets[processNum], delayTimesResized[processNum], diffusion);
    				processNum++;
    			}
    		}

    		if constexpr (NumDelayPerStage > 0)
    		for ( auto j = 0ul; j < NumDelayPerStage; ++j)
    		{
    			sample = Delay::process<InterpType>(delayLine, sample, delayOffsets[processNum], delayTimesResized[processNum]);
    			processNum++;
    		}

    		if constexpr (NumFBCombPerStage > 0)
    		{
    			for ( auto j = 0ul; j < NumFBCombPerStage; ++j)
    			{
    				sample = FBComb::process<InterpType>(delayLine, sample, delayOffsets[processNum], delayTimesResized[processNum], 0.0f);
    				processNum++;
    			}
    		}

    		if constexpr (NumLowpassPerStage > 0)
    		{
    			for ( auto j = 0ul; j < NumLowpassPerStage; ++j)
    			{
    				sample = LowPass::process<interpolation::InterpolatorTypes::none>(delayLine, sample, static_cast<size_t>(roundToInt(delayOffsets[processNum])), parameters.damping.currentValue);
    				processNum++;
    			}
    		}

    		taps[i] = sample;
    	}

    	delayLine.advanceWritePointer();
    }

	void mixTapsForOutput(const juce::dsp::AudioBlock<float>& outputBlock, const size_t numChannels, const size_t i)
    {
    	for (auto c = 0ul; c < numChannels; ++c)
    	{
			const auto out = outputBlock.getChannelPointer(c);
    		out[i] = 0;
    		bool invert = false;
    		for (auto t = 0ul; t < taps.size(); ++t)
    		{
    			const auto tap = taps[(t+c) % taps.size()];
    			out[i] += invert ? -tap : tap;
    			invert = !invert;
    		}

    	}
    }

	void calculateDelayTimes()
    {
    	auto calculate = [count = 1.0f](const size_t index, const size_t stepsPerStage, const float low, const float high) mutable{
    		const auto frac = count / static_cast<float>(stepsPerStage);
    		const auto range = high - low;
    		const auto mult = (count + static_cast<float>(index + 1)) * juce::MathConstants<float>::pi;
    		count*= juce::MathConstants<float>::euler;
    		return low + fmod(range * frac + mult * range, range);
    	};

		const auto sampsPerMS = static_cast<float>(spec.sampleRate) * 0.001f;

    	auto processNum = 0ul;
    	for ( auto i = 0ul; i < NumStages; ++i)
    	{
    		for ( auto j = 0ul; j < NumAllpassPerStage; ++j)
    		{
    			delayTimes[processNum] =  roundToInt(sampsPerMS * calculate(j, NumAllpassPerStage, 10.0f, 100.0f));
    			processNum++;
    		}

    		for ( auto j = 0ul; j < NumDelayPerStage; ++j)
    		{
    			delayTimes[processNum] =  roundToInt(sampsPerMS * calculate(j, NumDelayPerStage, 20.0f, 75.0f));
    			processNum++;
    		}

    		for ( auto j = 0ul; j < NumFBCombPerStage; ++j)
    		{
    			delayTimes[processNum] =  roundToInt(sampsPerMS * calculate(j, NumFBCombPerStage, 20.0f, 75.0f));
    			processNum++;
    		}

    		for ( auto j = 0ul; j < NumLowpassPerStage; ++j)
    		{
    			delayTimes[processNum] = 1.0f;
    			processNum++;
    		}
    	}
    }

	void calculateResizedDelayTimesAndOffsets()
    {
    	// Only scale pure Delay and FBComb stages with the size parameter
    	const auto size = parameters.size.currentValue;
    	for ( auto scaleableIndex : scaleableIndices)
    		delayTimesResized[scaleableIndex] = jmax(1.0f, delayTimes[scaleableIndex] * size);


    	auto acc = 0.0f;
    	for (size_t i = 0ul; i < TotalNumProcesses; ++i)
    	{
    		delayOffsets[i] = acc;
    		acc += delayTimesResized[i];
    	}
    }

	std::array<float, TotalNumProcesses> delayTimes{}, delayTimesResized{};
	std::array<float, TotalNumProcesses> delayOffsets{};
	std::array<float, NumStages> taps{};
	std::array<dsp::oscillators::SinCos, NumStages> modulators;
	sjf::helpers::MultiTapRingBuffer delayLine;
    juce::dsp::ProcessSpec spec{};
	float inputScaling = 1.0f / sqrtf(NumStages);
	const float outputScaling = 1.0f / sqrtf(NumStages);
};
}

