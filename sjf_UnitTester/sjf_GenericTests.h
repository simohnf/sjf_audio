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

		auto params = createParameters(processor, "Test", "Test");
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
			auto rms = juce::Decibels::gainToDecibels(outputBuffer1.getRMSLevel(i, 0, outputBuffer1.getNumSamples()));
			if ( rms > -70.0f)
				return false;
		}
		return true;
	}

	bool parameterRangeConversionsWork(const juce::dsp::ProcessSpec& spec, String& failureString)
	{
		Processor processor;
		auto rng = getRandom();
		auto params = createParameters(processor, "Test", "Test");
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
		for (int i = 0ul; i < numSamples; i++)
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

