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
	/**
	 * @brief A stereo balance wrapper template that adds equal-power pan/balance control to an underlying processor.
	 *
	 * This wrapper handles constant-power stereo balance adjustments via a square-root crossfade law ($L = \sqrt{1 - b}$, $R = \sqrt{b}$)
	 * with optional $3\text{ dB}$ center-boost scaling ($+\sqrt{2}$) to maintain unity gain at the center position.
	 *
	 * @tparam Processor The target audio processor class to be wrapped. Must expose standard JUCE DSP methods (`prepare`, `reset`, `process`, `createParameters`).
	 * @tparam UnityAtCentre When `true`, scales the output block by $\sqrt{2}$ ($\approx +3.01\text{ dB}$) so that center balance ($b = 0.5$) yields $0\text{ dB}$ unity gain per channel instead of $-3\text{ dB}$. Defaults to `true`.
	 */
	template <typename Processor, bool UnityAtCentre = true>
	class BalanceWrapper
	{
	public:
		struct Parameters : public helpers::AudioParametersBase
		{
			FloatState balance;

			std::unique_ptr<helpers::ParameterFactory> createParameters(const juce::String&,
																		const juce::String&) override
			{

				if (targetFactory)
				{
					const auto range = juce::NormalisableRange<float>{ -100.0f, 100.0f, 0.01f };
					const auto attributes = juce::AudioParameterFloatAttributes{}.withLabel("%");
					auto mapping = [](const float x){ return (0.5f * (1.0f + (x*0.01f)));};
					createTrackedParameter (*targetFactory, balance, "Balance", "Balance", range, 0.0f, mapping, attributes);
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
			jassert(spec_.numChannels == 2);
			spec = spec_;
			processor.prepare(spec);
			parameters.prepare(spec);

			for (auto& l : level)
				l.reset(spec.sampleRate, 0.05f);

			reset();
		}

		void reset()
		{
			processor.reset();
			parameters.reset();

			level[0].setCurrentAndTargetValue(std::sqrt(1.0f - parameters.balance.currentValue));
			level[1].setCurrentAndTargetValue(std::sqrt(parameters.balance.currentValue));

		}

		//==============================================================================
		template <typename ProcessContext>
		void process(const ProcessContext& context) noexcept
		{
			if constexpr (ProcessContext::usesSeparateInputAndOutputBlocks())
				context.getOutputBlock().copyFrom(context.getInputBlock());

			juce::dsp::ProcessContextReplacing<float> contextReplacing(context.getOutputBlock());

			if (!(level[0].isSmoothing() || level[1].isSmoothing()))
			{
				if (parameters.checkForStateChange())
				{
					parameters.reset(); // we handle levels manually so we just reset the smoothers
					level[0].setTargetValue(std::sqrt(1.0f - parameters.balance.currentValue));
					level[1].setTargetValue(std::sqrt(parameters.balance.currentValue));
				}
			}

			processor.process(contextReplacing);

			if constexpr (UnityAtCentre)
				contextReplacing.getOutputBlock().multiplyBy(juce::MathConstants<float>::sqrt2);

			for (auto c = 0ul; c < level.size(); c++)
				contextReplacing.getOutputBlock().getSingleChannelBlock(c).multiplyBy(level[c]);

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

		juce::dsp::ProcessSpec spec{};
		Processor processor;
		std::array<juce::LinearSmoothedValue<float>, 2> level;
	};

} // namespace sjf::helpers
