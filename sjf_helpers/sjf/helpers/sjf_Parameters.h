/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 27/09/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_ParameterFactory.h>

namespace sjf::helpers::parameters{
#include <atomic>

	/**
	 * @brief Pure interface implemented by custom JUCE parameters
	 *        that accept real-time audio-thread modulation offsets.
	 */
	class ParameterBase
	{
	public:
		virtual ~ParameterBase() = default;

		/**
		 * @brief Writes a pre-summed, normalized modulation offset [-1.0, +1.0]
		 *        calculated by the external ModulationSystem.
		 */
		void setModulationOffset(float offset) noexcept
		{
			modulationOffset.store(offset, std::memory_order_relaxed);
		}

		/**
		 * @brief Clears any active modulation offset back to 0.0f.
		 */
		void clearModulationOffset() noexcept
		{
			modulationOffset.store(0.0f, std::memory_order_relaxed);
		}

		/**
		 * @brief Reads the current normalized modulation offset.
		 */
		float getModulationOffset() const noexcept
		{
			return modulationOffset.load(std::memory_order_relaxed);
		}

		/**
		 * @brief Toggle whether this parameter currently responds to modulation.
		 */
		void setModulatable(bool shouldBeModulatable) noexcept { modulatable = shouldBeModulatable; }
		bool isModulatable() const noexcept { return modulatable; }

		virtual float get() const = 0;
	protected:
		/**
		 * @brief Helper that takes raw base normalized parameter value [0, 1]
		 *        adds the offset, and clamps safely to [0.0f, 1.0f].
		 */
		float applyModulationOffset(float baseNormalizedValue) const noexcept
		{
			if (!modulatable)
				return baseNormalizedValue;

			const float offset = modulationOffset.load(std::memory_order_relaxed);
			return juce::jlimit(0.0f, 1.0f, baseNormalizedValue + offset);
		}

	private:
		bool modulatable = true;
		std::atomic<float> modulationOffset { 0.0f };
	};


	class FloatParameter : public juce::AudioParameterFloat, public ParameterBase
	{
	public:
		using juce::AudioParameterFloat::AudioParameterFloat;

		// Unmodulated raw knob value
		float getDirect() const noexcept { return juce::AudioParameterFloat::get(); }

		// Combined value (used automatically by TrackedState via juceParameter->get())
		float get() const override
		{
			const float modNorm = applyModulationOffset(getNormalisableRange().convertTo0to1(getDirect()));
			return getNormalisableRange().convertFrom0to1(modNorm);
		}
	};

	class IntParameter : public juce::AudioParameterInt, public ParameterBase
	{
	public:
		using juce::AudioParameterInt::AudioParameterInt;

		int getDirect() const noexcept { return juce::AudioParameterInt::get(); }

		float get() const override
		{
			auto direct = getDirect();
			auto mod = applyModulationOffset(getNormalisableRange().convertTo0to1(direct));
			return getNormalisableRange().convertFrom0to1(mod);
		}
	};

	class ChoiceParameter : public juce::AudioParameterChoice, public ParameterBase
	{
	public:
		using juce::AudioParameterChoice::AudioParameterChoice;

		int getDirect() const noexcept { return getIndex(); }

		float get() const override
		{
			const float modNorm = applyModulationOffset(getNormalisableRange().convertTo0to1(static_cast<float>(getDirect())));
			const float plainVal = getNormalisableRange().convertFrom0to1(modNorm);
			return juce::jlimit(0.0f, static_cast<float>(choices.size() - 1), std::round(plainVal));
		}
	};

	class BoolParameter : public juce::AudioParameterBool, public ParameterBase
	{
	public:
		using juce::AudioParameterBool::AudioParameterBool;

		bool getDirect() const noexcept { return juce::AudioParameterBool::get(); }

		float get() const override
		{
			return applyModulationOffset(getNormalisableRange().convertTo0to1(getDirect())) >= 0.5f ? 1.0f : 0.0f;
		}
	};

}


