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
#include <sjf/processors/Reverbs/sjf_KeithBarrReverb.h>
#include <sjf/processors/Reverbs/sjf_MultitapDiffuser.h>
#include <sjf/processors/sjf_Filter_juce.h>
#include <sjf/helpers/sjf_ProcessorSequence.h>
#include <sjf/helpers/sjf_BypassWrapper.h>

#include <sjf/helpers/sjf_ProcessorSelector.h>
#include <sjf/processors/Reverbs/sjf_RotateDelayDiffuser.h>

#include "sjf/processors/sjf_Delay.h"

namespace sjf::dsp
{

	class PreDelay
	{
	public:
		static constexpr auto MaxPreDelayMS = 100.0f;
		struct Parameters : public helpers::AudioParametersBase
		{
			FloatState  time;

			std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) override
			{
				auto factory = helpers::ParameterFactory::create (factoryID, factoryName);
				createTrackedParameter  (*factory, time, "Time",  "Time",  { 0.0f, MaxPreDelayMS, 0.01f }, 0.0f, [&](const float x){ return x * 0.001f * static_cast<float>(spec.sampleRate); });

				return factory;
			}
		} parameters;

		void prepare (const juce::dsp::ProcessSpec& spec_)
		{
			spec = spec_;
			parameters.prepare(spec);

			delayLine.resize(spec.numChannels, {});
			for (auto & dl : delayLine)
			{
				dl.prepare(spec);
				dl.setMaxDelayTimeMS(MaxPreDelayMS * 1.1f);
			}

			reset();
		}

		void reset()
		{
			parameters.reset();
			for (auto & dl : delayLine)
				dl.reset();
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

			if (parameters.checkForStateChange())
			{
				processSmoothedState(context);
			}
			else
			{
				processStaticState(context);
			}
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

			jassert (inputBlock.getNumChannels() == numChannels);
			jassert (inputBlock.getNumSamples() == numSamples);

			const auto delayTime = parameters.time.currentValue;

			for (size_t channel = 0; channel < numChannels; ++channel)
			{
				auto* inputSamples  = inputBlock.getChannelPointer (channel);
				auto* outputSamples = outputBlock.getChannelPointer (channel);

				auto& dl = delayLine[channel];
				for (size_t i = 0; i < numSamples; ++i)
				{
					dl.writeSample(inputSamples[i]);
					outputSamples[i] = dl.readSample<sjf::interpolation::InterpolatorTypes::linear>(delayTime);
				}
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

			for (size_t i = 0; i < numSamples; ++i)
			{
				parameters.tickSmoothers();
				const auto delayTime = parameters.time.currentValue;
				for (size_t channel = 0; channel < numChannels; ++channel)
				{
					auto& dl = delayLine[channel];
					dl.writeSample(inputBlock.getChannelPointer(channel)[i]);
					outputBlock.getChannelPointer(channel)[i] = dl.readSample<sjf::interpolation::InterpolatorTypes::linear>(delayTime);
				}
			}
		}

		juce::dsp::ProcessSpec spec{};
		std::vector<sjf::helpers::DelayLine> delayLine;
	};


	struct Reverb
	{
	public:

		void prepare(const juce::dsp::ProcessSpec& spec_)
		{
			preDelay.prepare(spec_);
			filter.prepare(spec_);
			inputDiffuser.prepare(spec_);
			tank.prepare(spec_);
			reset();
		}

		void reset()
		{
			preDelay.reset();
			filter.reset();
			inputDiffuser.reset();
			tank.reset();
		}

		template<typename ProcessContext>
		void process(const ProcessContext& context) noexcept
		{
			preDelay.process(context);
			filter.process(context);
			inputDiffuser.process(context);
			tank.process(context);
		}

		std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName)
		{
			auto mainFactory = helpers::ParameterFactory::create (factoryID, factoryName);

			mainFactory->addChildFactory(preDelay.createParameters(factoryID + "PreDelay", factoryName + " Pre Delay"));
			mainFactory->addChildFactory(filter.createParameters(factoryID + "Filter", factoryName + " Filter"));
			mainFactory->addChildFactory(inputDiffuser.createParameters(factoryID + "Diffuser", factoryName + " Diffuser",
																		helpers::processor_sequence::SubFactoryConfig{"MT", "MT"},
																		helpers::processor_sequence::SubFactoryConfig{"RD", "RD"}
																		));
			mainFactory->addChildFactory(tank.createParameters(factoryID + "Tank", factoryName + " Reverb Tank"));



			return mainFactory;
		}


	private:
		/**
		 * @brief Internal DSP processing sequence: Filter >> Input Diffuser >> Reverb Tank.
		 */
		PreDelay preDelay;
		SVF<true, true> filter;
		sjf::helpers::ProcessorSelector<MultiTapDiffuser<64>, RotateDelayDiffuser<3, 3>> inputDiffuser;
		helpers::BypassWrapper<keith_barr::reverb::Tank<>, helpers::bypass_wrapper_config::Mix> tank;
	};
}



//DUMMY_PLUGIN_SJF_REVERBPLAYGROUND_H
