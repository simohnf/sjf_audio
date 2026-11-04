/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 22/07/2026.
//

#pragma once

#define SJF_AUDIO_TESTS 1

#include <JuceHeader.h>

#include "sjf/helpers/sjf_PresetManager.h"

namespace sjf::tests
{
template<typename Processor, typename... Args>
class GenericTests : juce::UnitTest
{
public:
	GenericTests(const String& processorName, Args&&... args_)
	: juce::UnitTest(processorName, "sjf_audio Unit Tests")
	, args(args_...)
	{ }

	void runTest() override
	{
		juce::dsp::ProcessSpec spec;
		spec.maximumBlockSize = 32;
		spec.numChannels = 2;

		testCase("Steady State", [&](){
			String sr{};
			expect([&](){
				for (auto i = 2000; i < 200000; i *= 2)
				{
					spec.sampleRate = i;
					if (!resetsToSteadyState(spec))
					{
						sr = String(i);
						return false;
					}
				}
				return true;
			}(), "State not Steady. SampleRate: "+sr);
		});

		testCase("Parameter ranges", [&](){
			String message{};
			expect([&](){
				for (auto i = 2000; i < 200000; i *= 2)
				{
					spec.sampleRate = i;

					if (!parameterRangeConversionsWork(spec, message))
						return false;
				}
				return true;
			}(), "Parameter range conversion faulty. " + message);
		});

		testCase("Denormal Flushing", [&](){
			String message{};
			expect([&](){
				for (auto i = 2000; i < 200000; i *= 2)
				{
					spec.sampleRate = i;
					if (!flushesDenormalsToZero(spec, message))
						return false;
				}
				return true;
			}(), "Failed denormal flushing test. " + message);
		 });

		testCase("DC Offset Accumulation and Recovery", [&](){
			String message{};
			expect([&](){
				for (auto i = 2000; i < 200000; i *= 2)
				{
					spec.sampleRate = i;
					if (!handlesDCOffset(spec, message))
						return false;
				}
				return true;
			}(), "Failed DC offset test. " + message);
		 });

		testCase("Arbitrary and Changing Block Sizes", [&](){
			String message{};
			expect([&](){
				for (auto i = 2000; i < 200000; i *= 2)
				{
					spec.sampleRate = i;
					if (!handlesVariableBlockSizes(spec, message))
						return false;
				}
				return true;
			}(), "Failed variable block size test. " + message);
		});

		testCase("In-Place vs Separate Buffer Processing", [&](){
			String message{};
			expect([&](){
				for (auto i = 2000; i < 200000; i *= 2)
				{
					spec.sampleRate = i;
					if (!handlesInPlaceAndSeparateBuffers(spec, message))
						return false;
				}
				return true;
			}(), "Failed in-place vs non-replacing buffer test. " + message);
		});
	}


private:
	bool resetsToSteadyState(const juce::dsp::ProcessSpec& spec)
	{
		static constexpr auto numBlocks = 128;
		auto rng = getRandom();
		const auto blockSize = static_cast<int>(spec.maximumBlockSize);
		const auto numSamples = static_cast<int>(spec.maximumBlockSize * numBlocks);
		const auto numChannels = static_cast<int>(spec.numChannels);

		juce::AudioBuffer<float> sinBuffer, blockBuffer, outputBuffer1, outputBuffer2, outputBuffer3;
		sinBuffer.setSize(numChannels, 128);
		blockBuffer.setSize(numChannels, blockSize);

		// output buffers have trailing 0s for tails/delay
		outputBuffer1.setSize(numChannels, numSamples, false, true, true);
		outputBuffer2.setSize(numChannels, numSamples, false, true, true);
		outputBuffer3.setSize(numChannels, numSamples, false, true, true);

		fillBufferWithSin(sinBuffer);

		juce::dsp::AudioBlock<float> sinBlock(sinBuffer);
		juce::dsp::AudioBlock<float> block1(outputBuffer1);
		juce::dsp::AudioBlock<float> block2(outputBuffer2);
		juce::dsp::AudioBlock<float> block3(outputBuffer3);

		auto processBlock = [spec_ = spec](juce::dsp::AudioBlock<float>& block, Processor& processor_){
			for ( auto i = 0ul; i < numBlocks; i++)
			{
				auto subBlock = block.getSubBlock(i*spec_.maximumBlockSize, spec_.maximumBlockSize);
				juce::dsp::ProcessContextReplacing<float> context{subBlock};
				processor_.process(context);
			}
		};


		/*============//============//============//============//============
		//============//============//============//============//============
										Test Body
		//============//============//============//============//============
		//============//============//============//============//==========*/
		Processor processor;

		auto params = std::move(createParameters(processor, "Test", "Test")->getAudioProcessorParameterGroup());
		processor.prepare(spec);
		// params->setAllToDefault();

		auto initVT = helpers::PresetManager::saveToVT(*params);

		// round 1 --> should be at steady state
		processor.reset();
		block1.copyFrom(sinBlock);
		processBlock(block1, processor);

		// round 2 --> randomise parameters (not steady state)
		for (auto& p : params->getParameters(true))
			p->setValue(rng.nextFloat());

		block2.copyFrom(sinBlock);
		processBlock(block2, processor);

		// round 3 --> should return to steady state and output same as round 1
		helpers::PresetManager::loadFromVT(*params, initVT);

		processor.reset();
		block3.copyFrom(sinBlock);
		processBlock(block3, processor);

		block1.addProductOf(block3, -1.0f);
		for ( auto i = 0; i < static_cast<int>(numChannels); i++)
		{
			auto rms = juce::Decibels::gainToDecibels(outputBuffer1.getRMSLevel(i, 0, outputBuffer1.getNumSamples()), -160.0f);
			if ( rms > -70.0f)
				return false;
		}
		return true;
	}

	bool parameterRangeConversionsWork(const juce::dsp::ProcessSpec& spec, String& failureString)
	{
		Processor processor;
		auto rng = getRandom();
		auto params = std::move(createParameters(processor, "Test", "Test")->getAudioProcessorParameterGroup());
		if (!params)
			return true;
		processor.prepare(spec);
		// params->setAllToDefault();
		processor.reset();

		auto getParameterValue = [](RangedAudioParameter* rangedParam) {
			if (const auto choiceParam = dynamic_cast<AudioParameterChoice*>(rangedParam))
				return static_cast<float>(choiceParam->getIndex());
			if (const auto intParam = dynamic_cast<AudioParameterInt*>(rangedParam))
				return static_cast<float>(intParam->get());
			if (const auto floatParam = dynamic_cast<AudioParameterFloat*>(rangedParam))
				return floatParam->get();
			if (const auto boolParam = dynamic_cast<AudioParameterBool*>(rangedParam))
				return static_cast<float>(boolParam->get());
			jassertfalse;
			return rangedParam->getCurrentValueAsText().getFloatValue();
		};

		auto paramName = [&params](RangedAudioParameter* rangedParam){
			return helpers::ParameterFactory::getNameWithoutParentPrefix(*rangedParam, *params);
		};

		for (auto p : params->getParameters(true))
		{
			if(const auto ranged = dynamic_cast<RangedAudioParameter*>(p))
			{
				const auto range  = ranged->getNormalisableRange();
				const auto tolerance = juce::absoluteTolerance(range.interval * 0.5f);
				// check default
				{
					// processor initialises to default values
					const auto before = getParameterValue(ranged);
					ranged->setValue(rng.nextFloat());
					ranged->setValue(ranged->getDefaultValue());
					const auto after = getParameterValue(ranged);

					if (!approximatelyEqual(before, after, tolerance))
					{
						failureString = paramName(ranged) + " default value does not convert correctly";
						return false;
					}

					const auto converted = ranged->convertFrom0to1(ranged->getDefaultValue());
					if (!approximatelyEqual(ranged->convertTo0to1(converted), ranged->getDefaultValue()))
					{
						failureString = paramName(ranged) + " default value does not convert correctly";
						return false;
					}
				}

				// check minimum
				{
					const auto converted = ranged->convertFrom0to1(0.0f);
					if (!juce::approximatelyEqual(ranged->convertTo0to1(converted), 0.0f))
					{
						failureString = paramName(ranged) + " minimum value does not convert correctly";
						return false;
					}

					ranged->setValue(0.0f);
					if (!juce::approximatelyEqual(getParameterValue(ranged), range.start, tolerance))
					{
						failureString = paramName(ranged) + " minimum value does not convert correctly";
						return false;
					}
				}

				// check maximum
				{
					const auto converted = ranged->convertFrom0to1(1.0f);
					if (!juce::approximatelyEqual(ranged->convertTo0to1(converted), 1.0f))
					{
						failureString = paramName(ranged) + " maximum value does not convert correctly";
						return false;
					}

					ranged->setValue(1.0f);
					if (!juce::approximatelyEqual(getParameterValue(ranged), range.end, tolerance ))
					{
						failureString = paramName(ranged) + " maximum value does not convert correctly";
						return false;
					}
				}
			}
		}
		return true;
	}

	bool flushesDenormalsToZero(const juce::dsp::ProcessSpec& spec, String& failureString)
	{
		static constexpr auto numBlocks = 256; // Long decay period (~8000+ samples)
		const auto blockSize = static_cast<int>(spec.maximumBlockSize);
		const auto numSamples = blockSize * numBlocks;
		const auto numChannels = static_cast<int>(spec.numChannels);

		Processor processor;
		auto params = std::move(createParameters(processor, "Test", "Test")->getAudioProcessorParameterGroup());
		processor.prepare(spec);
		processor.reset();

		// 1. Send an initial impulse to excite internal states (filters, delays)
		juce::AudioBuffer<float> impulseBuffer(numChannels, blockSize);
		impulseBuffer.clear();
		for (auto c = 0; c < numChannels; ++c)
			impulseBuffer.setSample(c, 0, 1.0f);

		juce::dsp::AudioBlock<float> impulseBlock(impulseBuffer);
		juce::dsp::ProcessContextReplacing<float> impulseContext{ impulseBlock };
		processor.process(impulseContext);

		// 2. Process silence / tiny subnormal noise across multiple blocks
		juce::AudioBuffer<float> decayBuffer(numChannels, numSamples);
		decayBuffer.clear();

		juce::dsp::AudioBlock<float> decayBlock(decayBuffer);
		for (auto i = 0ul; i < numBlocks; ++i)
		{
			auto subBlock = decayBlock.getSubBlock(i * spec.maximumBlockSize, spec.maximumBlockSize);
			juce::dsp::ProcessContextReplacing<float> context{ subBlock };
			processor.process(context);
		}

		// 3. Scan output for subnormal floats
		for (auto c = 0; c < numChannels; ++c)
		{
			const auto* data = decayBuffer.getReadPointer(c);
			for (auto i = 0; i < numSamples; ++i)
			{
				if (std::fpclassify(data[i]) == FP_SUBNORMAL)
				{
					failureString = "Processor produced subnormal float (denormal) at channel "
									+ String(c) + ", sample " + String(i)
									+ " (Value: " + String(data[i]) + ")";
					return false;
				}
			}
		}

		return true;
	}

	bool handlesDCOffset(const juce::dsp::ProcessSpec& spec, String& failureString)
	{
		static constexpr auto dcBlocks = 64;     // Blocks of constant DC
		static constexpr auto tailBlocks = 256;  // Blocks of silence to allow tail to clear
		const auto blockSize = static_cast<int>(spec.maximumBlockSize);
		const auto numChannels = static_cast<int>(spec.numChannels);

		Processor processor;
		auto params = std::move(createParameters(processor, "Test", "Test")->getAudioProcessorParameterGroup());
		processor.prepare(spec);
		processor.reset();

		// 1. Process constant DC offset block
		juce::AudioBuffer<float> dcBuffer(numChannels, blockSize);
		for (auto c = 0; c < numChannels; ++c)
			for (auto i = 0; i < blockSize; ++i)
				dcBuffer.setSample(c, i, 1.0f);

		juce::dsp::AudioBlock<float> dcBlock(dcBuffer);

		for (auto b = 0ul; b < dcBlocks; ++b)
		{
			juce::dsp::ProcessContextReplacing<float> context{ dcBlock };
			processor.process(context);

			// Assert output stays finite and does not blow up uncontrollably
			for (auto c = 0; c < numChannels; ++c)
			{
				const auto* data = dcBuffer.getReadPointer(c);
				for (auto i = 0; i < blockSize; ++i)
				{
					if (!std::isfinite(data[i]) || std::abs(data[i]) > 100.0f)
					{
						failureString = "DC input caused output explosion at channel "
										+ String(c) + ", value: " + String(data[i]);
						return false;
					}
				}
			}
		}

		// 2. Process silence blocks to test offset recovery
		juce::AudioBuffer<float> silenceBuffer(numChannels, blockSize);
		juce::dsp::AudioBlock<float> silenceBlock(silenceBuffer);

		for (auto b = 0ul; b < tailBlocks; ++b)
		{
			silenceBuffer.clear();
			juce::dsp::ProcessContextReplacing<float> context{ silenceBlock };
			processor.process(context);
		}

		// 3. Verify that output settled back down after silence
		for (auto c = 0; c < numChannels; ++c)
		{
			const auto rms = juce::Decibels::gainToDecibels(silenceBuffer.getRMSLevel(c, 0, blockSize), -160.0f);
			if (rms > -100.0f)
			{
				failureString = "DC offset trapped in processor state (Final RMS: "
								+ String(rms) + " dB on channel " + String(c) + ")";
				return false;
			}
		}

		return true;
	}

	bool handlesVariableBlockSizes(const juce::dsp::ProcessSpec& spec, String& failureString)
	{
		const auto totalSamples = juce::jmax(static_cast<int>(spec.sampleRate), 4192);
		const auto numChannels = static_cast<int>(spec.numChannels);

		// 1. Prepare input sine wave buffer
		juce::AudioBuffer<float> inputBuffer(numChannels, totalSamples);
		fillBufferWithSin(inputBuffer);


		// Prepare output storage for reference run and variable run
		juce::AudioBuffer<float> referenceOutput(numChannels, totalSamples);
		juce::AudioBuffer<float> variableOutput(numChannels, totalSamples);

		juce::dsp::AudioBlock<float> refBlock(referenceOutput);
		refBlock.copyFrom(inputBuffer);
		juce::dsp::AudioBlock<float> varBlock(variableOutput);
		varBlock.copyFrom(inputBuffer);

		Processor processor;
		auto params = std::move(createParameters(processor, "Test", "Test")->getAudioProcessorParameterGroup());

		// 2. RUN A: Reference run (processed in max block sizes)
		{

			auto specForRef = spec;
			specForRef.maximumBlockSize = static_cast<uint32_t>(spec.maximumBlockSize);
			processor.prepare(specForRef);
			processor.reset();


			const auto maxBlock = static_cast<size_t>(spec.maximumBlockSize);

			for (auto offset = 0ul; offset < static_cast<size_t>(totalSamples); offset += maxBlock)
			{
				const auto numSamplesThisBlock = std::min(maxBlock, static_cast<size_t>(totalSamples) - offset);
				auto subBlock = refBlock.getSubBlock(offset, numSamplesThisBlock);
				juce::dsp::ProcessContextReplacing<float> context{ subBlock };
				processor.process(context);
			}
		}

		// 3. RUN B: Variable block run (processed in varying/arbitrary non-power-of-two sizes)
		{
			auto specForVar = spec;
			processor.prepare(specForVar);
			processor.reset();

			size_t processedSamples = 0;
			// Sequence of arbitrary prime / odd block sizes to test boundaries
			static const std::array<size_t, 8> testBlockSizes = { 7, 3, 1, 11, 17, 23, 5, 31 };
			size_t blockIndex = 0;

			while (processedSamples < static_cast<size_t>(totalSamples))
			{
				// Pick block size from arbitrary array or random range
				auto chunkSize = testBlockSizes[blockIndex % testBlockSizes.size()];
				chunkSize = std::min(chunkSize, static_cast<size_t>(totalSamples) - processedSamples);

				auto subBlock = varBlock.getSubBlock(processedSamples, chunkSize);
				juce::dsp::ProcessContextReplacing<float> context{ subBlock };
				processor.process(context);

				processedSamples += chunkSize;
				blockIndex++;
			}
		}

		// 4. Compare Reference vs Variable output
		refBlock.addProductOf(varBlock, -1.0f);

		for (auto c = 0; c < numChannels; ++c)
		{
			const auto rmsDiff = juce::Decibels::gainToDecibels(referenceOutput.getRMSLevel(c, 0, totalSamples), -160.0f);
			if (rmsDiff > -80.0f)
			{
				failureString = "Variable block size mismatch on channel "
								+ String(c) + " (Difference RMS: " + String{rmsDiff} + " dB); SR: " + juce::String{spec.sampleRate};
				return false;
			}
		}

		return true;
	}

	bool handlesInPlaceAndSeparateBuffers(const juce::dsp::ProcessSpec& spec, String& failureString)
	{
		static constexpr auto numBlocks = 64;
		const auto maxBlockSize = static_cast<size_t>(spec.maximumBlockSize);
		const auto totalSamples = maxBlockSize * numBlocks;
		const auto numChannels = static_cast<int>(spec.numChannels);

		// 1. Create clean reference input
		auto inputReference = juce::AudioBuffer<float>{numChannels, static_cast<int>(totalSamples)};
		fillBufferWithSin(inputReference);
		const auto inputBlock = juce::dsp::AudioBlock<float>{inputReference};

		// Prepare buffers for In-Place (Replacing)
		auto inPlaceBuffer = juce::AudioBuffer<float>{numChannels, static_cast<int>(totalSamples)};
		auto inPlaceBlock = juce::dsp::AudioBlock<float>{inPlaceBuffer};
		inPlaceBlock.copyFrom(inputBlock);

		// Prepare buffers for Separate (Non-Replacing)
		auto nonReplacingInput = juce::AudioBuffer<float>{numChannels, static_cast<int>(totalSamples)};
		auto nonReplacingInBlock = juce::dsp::AudioBlock<float>{nonReplacingInput};
		nonReplacingInBlock.copyFrom(inputBlock);


		juce::AudioBuffer<float> nonReplacingOutput(numChannels, static_cast<int>(totalSamples));
		auto nonReplacingOutBlock = juce::dsp::AudioBlock<float>{nonReplacingOutput};
		nonReplacingOutput.clear();

		Processor processor;
		auto params = std::move(createParameters(processor, "Test", "Test")->getAudioProcessorParameterGroup());

		// 2. RUN A: Process In-Place (ProcessContextReplacing)
		{

			processor.prepare(spec);
			processor.reset();

			for (size_t offset = 0; offset < totalSamples; offset += maxBlockSize)
			{
				auto subBlock = inPlaceBlock.getSubBlock(offset, maxBlockSize);
				juce::dsp::ProcessContextReplacing<float> context{ subBlock };
				processor.process(context);
			}
		}

		// 3. RUN B: Process Separate (ProcessContextNonReplacing)
		{
			processor.prepare(spec);
			processor.reset();

			for (size_t offset = 0; offset < totalSamples; offset += maxBlockSize)
			{
				auto subInBlock = nonReplacingInBlock.getSubBlock(offset, maxBlockSize);
				auto subOutBlock = nonReplacingOutBlock.getSubBlock(offset, maxBlockSize);
				juce::dsp::ProcessContextNonReplacing<float> context{ subInBlock, subOutBlock };
				processor.process(context);
			}
		}

		// 4. CHECK 1: Input Preservation Check (nonReplacingInput must match inputReference)
		nonReplacingInBlock.subtract(inputBlock);
		for (auto c = 0; c < numChannels; ++c)
		{
			const auto inputMutatedRMS = juce::Decibels::gainToDecibels( nonReplacingInput.getRMSLevel(c, 0, static_cast<int>(totalSamples)), -160.0f);
			if (inputMutatedRMS > -100.0f)
			{
				failureString = "ProcessContextNonReplacing mutated the input buffer on channel "
								+ String(c) + " (Difference RMS: " + String(inputMutatedRMS) + " dB)";
				return false;
			}
		}

		// 5. CHECK 2: Output Equivalence Check (inPlaceBuffer must match nonReplacingOutput)
		inPlaceBlock.subtract(nonReplacingOutBlock);
		for (auto c = 0; c < numChannels; ++c)
		{
			const auto rmsDiff = juce::Decibels::gainToDecibels(inPlaceBuffer.getRMSLevel(c, 0, static_cast<int>(totalSamples)), -160.0f);
			if (rmsDiff > -100.0f)
			{
				failureString = "In-place vs separate buffer output mismatch on channel "
								+ String(c) + " (Difference RMS: " + String(rmsDiff) + " dB)";
				return false;
			}
		}
		return true;
	}

	/*=========//=========//=========//=========//=========//=========
	//=========//=========//=========//=========//=========//=========
								Helpers
	//=========//=========//=========//=========//=========//=========
	//=========//=========//=========//=========//=========//=======*/

	static void fillBufferWithSin(juce::AudioBuffer<float>& buffer)
	{
		const auto numChannels = buffer.getNumChannels();
		const auto numSamples = buffer.getNumSamples();
		const auto chan = buffer.getArrayOfWritePointers()[0];
		for (auto i = 0ul; i < static_cast<size_t>(numSamples); i++)
			chan[i] = 0.99f * juce::dsp::FastMathApproximations::sin(2.0f * juce::MathConstants<float>::pi * static_cast<float>(i) / static_cast<float>(numSamples));

		for (auto i = 1; i < numChannels; i++)
			buffer.copyFrom(i, 0, buffer, 0, 0, numSamples);
	}

	auto createParameters(Processor& processor, const String& factoryId, const String& factoryName)
	{
		if constexpr (sizeof...(Args) > 0)
		{
			// std::apply expands all tuple elements into 'unpackedArgs...' at once
			return std::apply([&](auto&&... unpackedArgs) {
				return processor.createParameters(factoryId, factoryName, std::forward<decltype(unpackedArgs)>(unpackedArgs)...);
			}, args);
		}
		else
		{
			return processor.createParameters(factoryId, factoryName);
		}
	}

	const std::tuple<Args...> args;
};
}

