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
#include <sjf/oscillators/sjf_SinCos.h>
#include <sjf/processors/Reverbs/sjf_ReverbHelpers.h>
#include <sjf/processors/Reverbs/sjf_MultiTapRingBufferConfig.h>



namespace sjf::dsp::keith_barr::reverb
{



template<typename StageConfiguration = ringbuffer_config::StageConfig<ringbuffer_config::AllPass, ringbuffer_config::AllPass, ringbuffer_config::Delay, ringbuffer_config::LowPass>, size_t NumStages = 8>
class Tank
{
	static constexpr auto ProcessesPerStage = StageConfiguration::numProcesses;
	static constexpr auto TotalNumProcesses = NumStages * ProcessesPerStage;

	// Helper to construct the full compile-time array across all stages
	static constexpr std::array<ringbuffer_config::ProcessType, TotalNumProcesses> processTypes = []() {
		std::array<ringbuffer_config::ProcessType, TotalNumProcesses> arr{};
		for (size_t stage = 0; stage < NumStages; ++stage)
		{
			for (size_t proc = 0; proc < ProcessesPerStage; ++proc)
			{
				arr[stage * ProcessesPerStage + proc] = StageConfiguration::processTypes[proc];
			}
		}
		return arr;
	}();

	template<ringbuffer_config::ProcessType TypeToCount>
	static constexpr size_t NumOfTypePerStage()
	{
		size_t count = 0;
		for (auto type : StageConfiguration::processTypes)
			if (type == TypeToCount)
				count++;
		return count;
	}

	static constexpr size_t NumAllpassPerStage = NumOfTypePerStage<ringbuffer_config::ProcessType::AllPass>();
	static constexpr size_t NumDelayPerStage = NumOfTypePerStage<ringbuffer_config::ProcessType::Delay>();
	static constexpr size_t NumFBCombPerStage = NumOfTypePerStage<ringbuffer_config::ProcessType::FBComb>();
	static constexpr size_t NumLPFBCombPerStage = NumOfTypePerStage<ringbuffer_config::ProcessType::LPFBComb>();
	static constexpr size_t NumLowpassPerStage = NumOfTypePerStage<ringbuffer_config::ProcessType::LowPass>();


	static constexpr size_t NumScaleableStages = (NumFBCombPerStage + NumDelayPerStage + NumAllpassPerStage + NumLPFBCombPerStage) * NumStages;
	static constexpr size_t ScaleablePerStage = ProcessesPerStage - NumLowpassPerStage;

	static constexpr auto DelayTimesMs = reverb_helpers::ReverbDelayTimeCalculator::calculateMsDelayTimes<NumScaleableStages, 10.0f, 80.0f, reverb_helpers::ReverbDelayTimeCalculator::SpacingType::Stochastic>();

	// Compile-time array containing ONLY the indices of Delay/FBComb processes
	static constexpr std::array<size_t, NumScaleableStages> scaleableIndices = []() {
		std::array<size_t, NumScaleableStages> indices{};
		size_t writeIdx = 0;
		for (size_t i = 0; i < TotalNumProcesses; ++i)
		{
			if (processTypes[i] != ringbuffer_config::ProcessType::LowPass)
				indices[writeIdx++] = i;
		}
		return indices;
	}();


public:
    struct Parameters : public helpers::AudioParametersBase
    {
        FloatState  size, damping, decay, diffusion, modulation;

    	std::array<FloatState, NumScaleableStages> delayTimes;
    	std::unique_ptr<helpers::ParameterFactory> delayTimeGroup;

        std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) override
        {
            auto factory = helpers::ParameterFactory::create (factoryID, factoryName);
        	createTrackedParameter(*factory, size, "Size", "Size", {0.0f, 100.0f, 0.01f}, 50.0f, sizeMapping);
        	auto fRange = NormalisableRange<float>{100.0f, 20000.0f, 0.01f};
        	fRange.setSkewForCentre(1000.0f);
        	createTrackedParameter(*factory, damping, "Damping", "Damping", fRange, 2000.0f, [&](const float x){
        		return helpers::functions::dsp_functions::calculateOnepoleCoefficient(x, static_cast<float>(spec.sampleRate));
        	});
        	createTrackedParameter(*factory, decay, "Decay", "Decay", {0.0f, 100.0f, 0.01f}, 50.0f, [](const float x){ return pow(jmap(x*0.01f, 0.01f, 0.99f), 0.5f);});
        	createTrackedParameter(*factory, diffusion, "Diffusion", "Diffusion", {0.0f, 100.0f, 0.01f}, 50.0f, [&](const float x){return 0.25f + x*0.01f*0.249f;});
        	createTrackedParameter(*factory, modulation, "Modulation", "Modulation", {0.0f, 100.0f, 0.01f}, 50.0f, [&](const float x){return x*0.001f;});

        	delayTimeGroup = helpers::ParameterFactory::create("delayTimes", "DelayTimes");
        	for ( auto s = 0ul; s < NumStages; ++s )
        	{
        		const auto offset = s*ScaleablePerStage;
        		for ( auto dt = 0ul; dt< ScaleablePerStage; ++dt)
        		{
        			const auto i = offset + dt;
        			auto mapping = [this, s, dt](const float){ return delayTimeSampsPreCompute[s][dt]; };
        			createTrackedParameter(*delayTimeGroup, delayTimes[i], "DelayTime"+String(i), "DelayTime "+String(i), {0.0f, 1.0f}, 0.0f, mapping);
        		}
        	}

        	static_assert(NumStages*ScaleablePerStage == NumScaleableStages);

	        {
            	using namespace reverb_helpers;
		        reverb_helpers::ReverbDelayTimeCalculator::calculateCoprimeSampleTimes< ReverbDelayTimeCalculator::FillMode::Row,
            																			ReverbDelayTimeCalculator::ShuffleMode::Both>
																						(DelayTimesMs, delayTimeSampsPreCompute, spec.sampleRate);
	        }
            return factory;
        }

    	bool checkForStateChange()
        {

        	const auto mappedSize = sizeMapping(size.getParameterValue());
        	if (!approximatelyEqual(mappedSize, lastSize) && !size.isSmoothing()) // size just changed
        	{
        		using namespace reverb_helpers;
        		reverb_helpers::ReverbDelayTimeCalculator::calculateCoprimeSampleTimes< ReverbDelayTimeCalculator::FillMode::Row,
																						ReverbDelayTimeCalculator::ShuffleMode::Both>
																						(DelayTimesMs, delayTimeSampsPreCompute, spec.sampleRate, mappedSize);
        		lastSize = mappedSize;
        	}
        	return AudioParametersBase::checkForStateChange();
        }

    	void reset()
        {
        	const auto mappedSize = sizeMapping(size.getParameterValue());
        	using namespace reverb_helpers;
        	reverb_helpers::ReverbDelayTimeCalculator::calculateCoprimeSampleTimes< ReverbDelayTimeCalculator::FillMode::Row,
																					ReverbDelayTimeCalculator::ShuffleMode::Both>
																					(DelayTimesMs, delayTimeSampsPreCompute, spec.sampleRate, mappedSize);
        	lastSize = mappedSize;
        	helpers::AudioParametersBase::reset();
        }

    	std::array<std::array<size_t, ScaleablePerStage>, NumStages> delayTimeSampsPreCompute;
    	std::function<float(float)> sizeMapping = [](const float x){return pow(2.0f, jmap(2.0f*x*0.01f - 1.0f, -1.0f, 1.0f, -0.8f, 0.8f));};
    	float lastSize = 0.0f;
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
    		m.setFrequency(jmap( fmod(juce::MathConstants<float>::euler * static_cast<float>(i) / static_cast<float>(NumStages), 1.0f), 0.1f, 3.75f));
    	}

    	// calculateDelayTimes();
    	// std::copy(delayTimes.begin(), delayTimes.end(), delayTimesResized.begin());

    	constexpr auto LPs = static_cast<float>(NumStages * NumLowpassPerStage);
    	delayLine.setMaxDelayTimeSamps(static_cast<int>((std::accumulate(DelayTimesMs.begin(), DelayTimesMs.end(), 0.0f) * spec.sampleRate * 0.001f + LPs) * 2.0f + 3.0f));

        reset();
    }

    void reset()
    {
        parameters.reset();
    	for (auto& m : modulators)
    		m.reset();
    	delayLine.reset();
    	std::fill(taps.begin(), taps.end(), 0.0f);
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

private:
    template <typename ProcessContext>
    void processStaticState (const ProcessContext& context) noexcept
    {
        const auto& inputBlock = context.getInputBlock();
        auto& outputBlock      = context.getOutputBlock();
        const auto numChannels = outputBlock.getNumChannels();
        const auto numSamples  = outputBlock.getNumSamples();


        auto* inputSamples  = inputBlock.getChannelPointer (0);

    	// calculateResizedDelayTimesAndOffsets();

    	for (size_t i = 0; i < numSamples; ++i)
    	{
    		processSample<sjf::interpolation::InterpolatorTypes::none, true>(inputSamples[i]);

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
    			// calculateResizedDelayTimesAndOffsets();
    			processSample<sjf::interpolation::InterpolatorTypes::cubic, false>(inputSamples[i]);

    			mixTapsForOutput(outputBlock, numChannels, i);
    		}
    	}
    	else
    	{
    		for (size_t i = 0; i < numSamples; ++i)
    		{
    			parameters.tickSmoothers();
    			// calculateResizedDelayTimesAndOffsets();
    			processSample<sjf::interpolation::InterpolatorTypes::none, true>(inputSamples[i]);

    			mixTapsForOutput(outputBlock, numChannels, i);
    		}
    	}
    }

	template<sjf::interpolation::InterpolatorTypes InterpType, bool StaticSize>
	void processSample(float input)
    {
    	auto sample = taps.back() * parameters.decay.currentValue;

    	auto modFold = [](const float x){
    		if (x < 0.0f)
    			return -1.0f * x;
    		if (x > 1.0f)
    			return 1.0f - (x - 1.0f);
    		return x;
    	};

    	const auto diffusion = parameters.diffusion.currentValue;
    	const auto modDepth = parameters.modulation.currentValue;

    	auto delayTime = [&](const size_t Stage, const size_t ProcessNum){
    		auto p = ScaleablePerStage*Stage + ProcessNum;
    		return parameters.delayTimes[p].currentValue;
    	};

    	auto offset = 0.0f;
    	const auto decay = 0.0f; /// TODO: add parameter for decay, or link to diffusion

    	for ( auto i = 0ul; i < NumStages; ++i)
    	{
    		auto processNum = 0ul;
    		using namespace ringbuffer_config;

    		sample += input; // each stage adds the input again
    		if constexpr (NumAllpassPerStage > 0)
    		{
    			auto mod_ = modulators[i]();
    			const auto mod = (i & 1 ? mod_.sinOut : mod_.cosOut) * modDepth;
    			if constexpr (StaticSize)
    			{
    				auto dt = delayTime(i, processNum);
    				sample = AllPass::process<InterpType>(delayLine, sample, offset, dt, modFold(diffusion + mod));
    				offset += dt;
    				jassert(!std::isnan(sample) && !std::isinf(sample));
    				processNum++;
    				for ( auto j = 1ul; j < NumAllpassPerStage; ++j)
    				{
    					dt = delayTime(i, processNum);
    					sample = AllPass::process<InterpType>(delayLine, sample, offset, dt, diffusion);
    					offset += dt;
    					jassert(!std::isnan(sample) && std::isfinite(sample));
    					processNum++;
    				}
    			}
    			else
    			{
    				for ( auto j = 0ul; j < NumAllpassPerStage; ++j)
    				{
    					const auto dt = delayTime(i, processNum);
    					sample = ringbuffer_config::Delay::process<InterpType>(delayLine, sample, offset, dt);
    					offset += dt;
    					jassert(!std::isnan(sample) && !std::isinf(sample));
    					processNum++;
    				}
    			}
    		}

    		if constexpr (NumDelayPerStage > 0)
    		for ( auto j = 0ul; j < NumDelayPerStage; ++j)
    		{
    			const auto dt = delayTime(i, processNum);
    			sample = ringbuffer_config::Delay::process<InterpType>(delayLine, sample, offset, dt);
    			offset += dt;
    			jassert(!std::isnan(sample) && !std::isinf(sample));
    			processNum++;
    		}

    		if constexpr (NumFBCombPerStage > 0)
    		{
    			if constexpr (StaticSize)
    			{
    				for ( auto j = 0ul; j < NumFBCombPerStage; ++j)
    				{
    					const auto dt = delayTime(i, processNum);
    					sample = FBComb::process<InterpType>(delayLine, sample, offset, dt, decay);
    					offset += dt;
    					jassert(!std::isnan(sample) && !std::isinf(sample));
    					processNum++;
    				}
    			}
    			else
    			{
    				for ( auto j = 0ul; j < NumFBCombPerStage; ++j)
    				{
    					const auto dt = delayTime(i, processNum);
    					sample = ringbuffer_config::Delay::process<InterpType>(delayLine, sample, offset, dt);
    					offset += dt;
    					jassert(!std::isnan(sample) && !std::isinf(sample));
    					processNum++;
    				}
    			}

    		}


    		if constexpr (NumLPFBCombPerStage > 0)
    		{
    			if constexpr (StaticSize)
    			{
    				for ( auto j = 0ul; j < NumLPFBCombPerStage; ++j)
    				{
    					const auto dt = delayTime(i, processNum);
    					sample = ringbuffer_config::LPFBComb::process<interpolation::InterpolatorTypes::none>(delayLine, sample, offset, dt, decay, parameters.damping.currentValue);
    					offset += dt;
    					jassert(!std::isnan(sample) && !std::isinf(sample));
    					processNum++;
    				}
    			}
    			else
    			{
    				for ( auto j = 0ul; j < NumLPFBCombPerStage; ++j)
    				{
    					const auto dt = delayTime(i, processNum);
    					sample = ringbuffer_config::Delay::process<interpolation::InterpolatorTypes::none>(delayLine, sample, offset, dt);
    					offset += dt;
    					jassert(!std::isnan(sample) && !std::isinf(sample));
    					processNum++;
    				}
    			}
    		}

    		if constexpr (NumLowpassPerStage > 0)
    		{
    			if constexpr (StaticSize)
    			{
    				for ( auto j = 0ul; j < NumLowpassPerStage; ++j)
    				{
    					sample = LowPass::process<interpolation::InterpolatorTypes::none>(delayLine, sample, offset, parameters.damping.currentValue);
    					offset += 1;
    					jassert(!std::isnan(sample) && !std::isinf(sample));
    					processNum++;
    				}
    			}
    			else
    			{
    				for ( auto j = 0ul; j < NumLowpassPerStage; ++j)
    				{
    					sample = ringbuffer_config::Delay::process<interpolation::InterpolatorTypes::none>(delayLine, sample, offset, 1);
    					offset += 1;
    					jassert(!std::isnan(sample) && !std::isinf(sample));
    					processNum++;
    				}
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

	std::array<float, NumStages> taps{};
	std::array<dsp::oscillators::SinCos, NumStages> modulators;
	sjf::helpers::MultiTapRingBuffer delayLine;
    juce::dsp::ProcessSpec spec{};
	float inputScaling = 1.0f / sqrtf(NumStages);
	const float outputScaling = 1.0f / sqrtf(NumStages);
};


}

