/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 15/08/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_OptionalCalls.h>
#include <sjf/helpers/sjf_ParameterFactory.h>

namespace sjf::helpers
{
	namespace gain_wrapper_config
	{
		template<float Min_dB = -80.0f, float Max_dB = 12.0f, float SkewForCentre = 0.0f, float Default_dB = 0.0f>
		struct GainConfig
		{
			static_assert(Min_dB < Max_dB, "Min_dB must be less than Max_dB");
			static constexpr float Min = Min_dB;
			static constexpr float Max = Max_dB;
			static constexpr float Default = Default_dB >= Min && Default_dB <= Max ? Default_dB : Min + 0.5f *(Max - Min);
			static constexpr float Skew = SkewForCentre > Min && SkewForCentre < Max ? SkewForCentre : Min + 0.5f *(Max - Min);
		};
	}

	template <typename Processor, bool InputGain, bool OutputGain, typename InputConfig = gain_wrapper_config::GainConfig<>, typename OutputConfig = gain_wrapper_config::GainConfig<>>
	class GainWrapper
	{
	public:
		static_assert(InputGain || OutputGain, "You must have either input or output gain");
		struct Parameters : public helpers::AudioParametersBase
		{
			FloatState input, output;

			std::unique_ptr<helpers::ParameterFactory> createParameters(const juce::String&,
																		const juce::String&) override
			{

				if (targetFactory)
				{
					/// we insert the parameters into the processors parameter tree,
					/// BUT The BypassWrapper::Parameters struct handles smoothing/reset etc the

					if constexpr (InputGain)
					{
						auto range = juce::NormalisableRange<float>{ InputConfig::Min, InputConfig::Max, 0.01f };
						range.setSkewForCentre(InputConfig::Skew);
						const auto attributes = juce::AudioParameterFloatAttributes{}.withLabel("dB");
						const auto mapping = [](const float x){ return juce::Decibels::decibelsToGain(x);};
						createTrackedParameter (*targetFactory, input, "InG", "Input Gain", range, InputConfig::Default, mapping, attributes);
					}

					if constexpr (OutputGain)
					{
						auto range = juce::NormalisableRange<float>{ OutputConfig::Min, OutputConfig::Max, 0.01f };
						range.setSkewForCentre(OutputConfig::Skew);
						const auto attributes = juce::AudioParameterFloatAttributes{}.withLabel("dB");
						const auto mapping = [&](const float x){ return juce::Decibels::decibelsToGain(x);};
						createTrackedParameter (*targetFactory, output, "OutG", "Output Gain", range, OutputConfig::Default, mapping, attributes);
					}
				}
				else
				{
					jassertfalse;
				}

				targetFactory = nullptr;

				return std::unique_ptr<helpers::ParameterFactory>{nullptr};
			}

			void setParameterFactory(ParameterFactory* factoryToUse) { targetFactory = factoryToUse; }

		private:
			ParameterFactory* targetFactory{nullptr};
		} parameters;

		//==============================================================================
		void prepare(const juce::dsp::ProcessSpec& spec_)
		{
			spec = spec_;
			processor.prepare(spec);
			parameters.prepare(spec);

			if constexpr (InputGain)
				inputLevel.reset(spec.sampleRate, 0.05f);
			if constexpr (OutputGain)
				outputLevel.reset(spec.sampleRate, 0.05f);

			reset();
		}

		void reset()
		{
			processor.reset();
			parameters.reset();

			if constexpr (InputGain)
				inputLevel.setCurrentAndTargetValue(parameters.input.currentValue);

			if constexpr (OutputGain)
				outputLevel.setCurrentAndTargetValue(parameters.output.currentValue);

		}

		//==============================================================================
		template <typename ProcessContext>
		void process(const ProcessContext& context) noexcept
		{
			if constexpr (ProcessContext::usesSeparateInputAndOutputBlocks())
				context.getOutputBlock().copyFrom(context.getInputBlock());

			juce::dsp::ProcessContextReplacing<float> contextReplacing(context.getOutputBlock());

			if (!smoothersActive())
			{
				if (parameters.checkForStateChange())
				{
					parameters.reset(); // we handle levels manually so we just reset the smoothers
					if constexpr (InputGain)
						inputLevel.setTargetValue(parameters.input.currentValue);
					if constexpr (OutputGain)
						outputLevel.setTargetValue(parameters.output.currentValue);
				}
			}

			if constexpr (InputGain)
			{
				contextReplacing.getOutputBlock().multiplyBy(inputLevel);
			}

			processor.process(contextReplacing);

			if constexpr (OutputGain)
			{
				contextReplacing.getOutputBlock().multiplyBy(outputLevel);
			}

		}

		//==============================================================================
		/**
			Generates the internal processor's parameter factory, hands it to our local
			parameters object to append the bypass state, and cleans up the transient pointer.
		*/
		template <typename... Args>
		std::unique_ptr<helpers::ParameterFactory>
		createParameters(const juce::String& factoryID, const juce::String& factoryName, Args&&... configArgs)
		{
			// 1. Ask the wrapped processor to generate its parameter factory layout first
			auto factory = processor.createParameters(factoryID, factoryName, std::forward<Args>(configArgs)...);

			if (factory == nullptr)
			{
				jassertfalse;
				return nullptr;
			}

			parameters.setParameterFactory(factory.get());

			parameters.createParameters(factoryID, factoryName);


			return factory;
		}

		//==============================================================================
		[[nodiscard]] Processor& getProcessor() noexcept { return processor; }
		[[nodiscard]] const Processor& getProcessor() const noexcept { return processor; }

		void setPositionInfo(const juce::AudioPlayHead::PositionInfo& positionInfo)
		{
			sjf::optional_calls::setPositionInfo(processor, positionInfo);
		}

		int getLatencySamples() { return sjf::optional_calls::getLatencySamples(processor); }

		void attachToState(juce::ValueTree& parentTree) { sjf::optional_calls::attachToState(processor, parentTree); }

	private:

		[[nodiscard]] bool smoothersActive() const
		{
			auto active = false;
			if constexpr (InputGain)
				active = active || inputLevel.isSmoothing();
			if constexpr (OutputGain)
				active = active || outputLevel.isSmoothing();
			return active;
		}

		juce::dsp::ProcessSpec spec{};
		Processor processor;
		[[maybe_unused]] juce::LinearSmoothedValue<float> inputLevel;
		[[maybe_unused]] juce::LinearSmoothedValue<float> outputLevel;
	};

} // namespace sjf::helpers
