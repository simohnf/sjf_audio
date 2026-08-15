/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 12/08/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_ParameterFactory.h>
#include <sjf/helpers/sjf_Waveshapers.h>

#include <sjf/helpers/sjf_DCBlock.h>
#include <sjf/processors/sjf_Filter_juce.h>

namespace sjf::dsp
{
	class Exciter
	{
		public:
			struct Parameters : public helpers::AudioParametersBase
			{
				FloatState  amount, shape;

				FloatState  secondOrder, thirdOrder;
				std::unique_ptr<helpers::ParameterFactory> shapeSmoothers;

				std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) override
				{
					auto factory = helpers::ParameterFactory::create (factoryID, factoryName, true, false);
					{
						auto att = juce::AudioParameterFloatAttributes().withLabel("%");
						createTrackedParameter  (*factory, amount, "Amount",  "Amount",  { 0.0f, 100.0f, 0.01f }, 0.0f,[](const float x){ return std::sqrt(x * 0.005f);}, att);
					}
					{
						auto att = juce::AudioParameterFloatAttributes().withLabel("%");
						createTrackedParameter  (*factory, shape, "Shape",  "Shape",  { 0.0f, 100.0f, 0.01f }, 0.0f,[](const float x){ return x * 0.01f;}, att);
					}

					shapeSmoothers = helpers::ParameterFactory::create("Shape", "Shape");
					createTrackedParameter  (*shapeSmoothers, secondOrder, "2nd",  "2nd",  { 0.0f, 100.0f, 0.01f }, 0.0f,[this](const float){ return std::sqrt(1.0f - shape.getParameterValue()*0.01f);});
					createTrackedParameter  (*shapeSmoothers, thirdOrder, "3rd",  "3rd",  { 0.0f, 100.0f, 0.01f }, 0.0f,[this](const float){ return std::sqrt(shape.getParameterValue()*0.01f);});

					return factory;
				}

			} parameters;

			void prepare (const juce::dsp::ProcessSpec& spec_)
			{
				spec = spec_;
				parameters.prepare(spec);
				inputFilter.prepare(spec);
				secondOrderFilter.prepare(spec);
				secondOrderFilter.setCutoffFrequency(static_cast<float>(spec.sampleRate) *0.5f*0.5f);
				secondOrderFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);

				thirdOrderFilter.prepare(spec);
				thirdOrderFilter.setCutoffFrequency(static_cast<float>(spec.sampleRate) *0.5f*0.33f);
				thirdOrderFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);


				for (auto& dc : dcBlocker)
						dc.prepare(spec);

				secondOrderBuffer.setSize(static_cast<int>(spec.numChannels), static_cast<int>(spec.maximumBlockSize));
				thirdOrderBuffer.setSize(static_cast<int>(spec.numChannels), static_cast<int>(spec.maximumBlockSize));
				reset();
			}

			void reset()
			{
				parameters.reset();
				inputFilter.reset();

				secondOrderFilter.reset();
				thirdOrderFilter.reset();

				for (auto& dcBlock : dcBlocker)
					dcBlock.reset();


				secondOrderBuffer.clear();
				thirdOrderBuffer.clear();
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

				juce::dsp::AudioBlock<float> secondOrderBlock(secondOrderBuffer);
				secondOrderBlock.copyFrom(inputBlock);
				auto subBlock2 = secondOrderBlock.getSubBlock(0, numSamples);
				juce::dsp::ProcessContextReplacing<float> secondOrderContext(subBlock2);

				inputFilter.process(secondOrderContext);

				juce::dsp::AudioBlock<float> thirdOrderBlock(thirdOrderBuffer);
				thirdOrderBlock.copyFrom(subBlock2);
				auto subBlock3 = thirdOrderBlock.getSubBlock(0, numSamples);
				juce::dsp::ProcessContextReplacing<float> thirdOrderContext(subBlock3);

				secondOrderFilter.process(secondOrderContext);
				thirdOrderFilter.process(thirdOrderContext);

				juce::dsp::AudioBlock<float>::process(subBlock2, subBlock2, [](const float x){return sjf::helpers::Waveshapers::Chebyshev<2>::calculate(x);});
				juce::dsp::AudioBlock<float>::process(subBlock3, subBlock3, [](const float x){return sjf::helpers::Waveshapers::Chebyshev<3>::calculate(x);});

				juce::dsp::ProcessContextReplacing<float> dcBlockContext2(subBlock2);
				juce::dsp::ProcessContextReplacing<float> dcBlockContext3(subBlock3);

				dcBlocker[0].process(dcBlockContext2);
				dcBlocker[1].process(dcBlockContext3);


				if (parameters.checkForStateChange())
				{

					for (auto s = 0ul; s < numSamples; s++)
					{
						parameters.tickSmoothers();
						for (auto c = 0ul; c < numChannels; c++)
						{
							subBlock2.getChannelPointer(c)[s] *= parameters.secondOrder.currentValue * parameters.amount.currentValue;
							subBlock3.getChannelPointer(c)[s]  *= parameters.thirdOrder.currentValue  * parameters.amount.currentValue;
						}

					}

					subBlock2.add(subBlock3);
				}
				else
				{
					subBlock2.multiplyBy(parameters.secondOrder.currentValue * parameters.amount.currentValue);
					subBlock3.multiplyBy (parameters.thirdOrder.currentValue * parameters.amount.currentValue);

					subBlock2.add(subBlock3);

				}




				if (ProcessContext::usesSeparateInputAndOutputBlocks())
					outputBlock.copyFrom(inputBlock);

				outputBlock.add(subBlock2);

			}


			std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName)
			{
				auto factory = parameters.createParameters (factoryID, factoryName);
				factory->addChildFactory(inputFilter.createParameters(factoryID+"Filter", factoryName+"Filter"));


				return factory;
			}

		private:

			juce::dsp::ProcessSpec spec{};
			sjf::dsp::SVF<FixedFilterType::HighPass, true> inputFilter;
			juce::dsp::StateVariableTPTFilter<float> secondOrderFilter, thirdOrderFilter;
			std::array<helpers::DCBlocker<>, 2> dcBlocker;
			juce::AudioBuffer<float> secondOrderBuffer, thirdOrderBuffer;
	};
}



//DUMMY_PLUGIN_SJF_EXCITER_H
