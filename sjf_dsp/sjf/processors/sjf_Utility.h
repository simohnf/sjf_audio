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
#include <sjf/helpers/sjf_ParameterFactory.h>

#include <sjf/helpers/sjf_BypassWrapper.h>

#include <sjf/helpers/Utility/sjf_BalanceWrapper.h>
#include <sjf/helpers/Utility/sjf_GainWrapper.h>
#include <sjf/helpers/sjf_Passthrough.h>
#include <sjf/helpers/Utility/sjf_DCBlockerWrapper.h>
#include <sjf/helpers/Utility/sjf_InputMatrixWrapper.h>
#include <sjf/helpers/Utility/sjf_MidSideWrapper.h>
#include <sjf/helpers/Utility/sjf_PolarityWrapper.h>


namespace sjf::dsp
{
namespace utility_configs
{
	struct StereoSpread{};
	struct MonoSwitch{};
	struct MonoBass{};
	struct PolarityInvert{};
	struct InputMatrix{};
	struct OutputGain{};
	struct Balance{};
	struct DCBlock{};
	struct Mute{};
}

/**
 * @brief A meta-processor utility class that constructs a modular, compile-time DSP chain using template policy tags.
 *
 * This class wraps `sjf::helpers::Passthrough` with a series of conditionally applied DSP wrapper templates based on
 * variadic template configuration parameters (`Configs...`). It evaluates policy flags at compile time to build an optimal
 * nested processor type stack in the following order:
 *
 * 1. **Polarity** (`helpers::PolarityWrapper`) - Pre-processing channel polarity inversion.
 * 2. **Input Matrix** (`helpers::InputMatrixWrapper`) - Input channel selection, swapping, or mono summing.
 * 3. **Mid/Side** (`helpers::MidSideWrapper`) - Spatial processing, stereo spread adjustment, and low-frequency mono summing.
 * 4. **Balance** (`helpers::BalanceWrapper`) - Constant-power stereo panning/balance control.
 * 5. **DC Block** (`helpers::DCBlockerWrapper`) - Post-processing DC offset transient filter.
 * 6. **Output Gain** (`helpers::GainWrapper`) - Final output gain stage with smoothed volume scaling.
 *
 * @tparam Configs Variadic list of configuration tags from `sjf::dsp::utility_configs` (e.g., `utility_configs::PolarityInvert`,
 *                 `utility_configs::InputMatrix`, `utility_configs::StereoSpread`, `utility_configs::MonoBass`, `utility_configs::Balance`,
 *                 `utility_configs::DCBlock`, `utility_configs::OutputGain`, `utility_configs::Mute`).
 */
template <typename ... Configs>
class Utility
{
public:
	static constexpr bool hasPolarityInvert	= sjf::helpers::functions::utilities::configurationAvailable<utility_configs::PolarityInvert, Configs...>;

	static constexpr bool hasMonoSwitch		= sjf::helpers::functions::utilities::configurationAvailable<utility_configs::MonoSwitch, Configs...>;
	static constexpr bool hasInputMatrix	= sjf::helpers::functions::utilities::configurationAvailable<utility_configs::InputMatrix, Configs...>;
	static_assert(!(hasMonoSwitch && hasInputMatrix), "Input Matrix already includes Mono, you can't add both");

	static constexpr bool hasStereoSpread	= sjf::helpers::functions::utilities::configurationAvailable<utility_configs::StereoSpread, Configs...>;
	static constexpr bool hasMonoBass		= sjf::helpers::functions::utilities::configurationAvailable<utility_configs::MonoBass, Configs...>;



	static constexpr bool hasBalance		= sjf::helpers::functions::utilities::configurationAvailable<utility_configs::Balance, Configs...>;
	static constexpr bool hasDCBlock		= sjf::helpers::functions::utilities::configurationAvailable<utility_configs::DCBlock, Configs...>;

	static constexpr bool hasOutputGain		= sjf::helpers::functions::utilities::configurationAvailable<utility_configs::OutputGain, Configs...>;

	static constexpr bool hasMute			= sjf::helpers::functions::utilities::configurationAvailable<utility_configs::Mute, Configs...>;


	template<typename T>
	using Polarity		= std::conditional_t<hasPolarityInvert, helpers::PolarityWrapper<T, true, false>, T>;

	template<typename T>
	using InputMatrix	= std::conditional_t<hasInputMatrix||hasMonoSwitch, helpers::InputMatrixWrapper<T, hasMonoSwitch>, T>;

	template<typename T>
	using MidSide		= std::conditional_t<hasMonoBass||hasStereoSpread, helpers::MidSideWrapper<T, hasStereoSpread, hasMonoBass>, T>;

	template<typename T>
	using Balance		= std::conditional_t<hasBalance, helpers::BalanceWrapper<T>, T>;

	template<typename T>
	using DCBlock		= std::conditional_t<hasDCBlock, helpers::DCBlockerWrapper<T, true, true>, T>;

	template<typename T>
	using OutGain		= std::conditional_t<hasOutputGain, helpers::GainWrapper<T, false, true>, T>;


	using Processor = OutGain<DCBlock<Balance<MidSide<InputMatrix<Polarity<sjf::helpers::Passthrough>>>>>>;

	struct Parameters : public helpers::AudioParametersBase
	{
		std::unique_ptr<helpers::ParameterFactory> createParameters(const juce::String& factoryID,
																	const juce::String& factoryName) override
		{
			auto factory = helpers::ParameterFactory::create(factoryID, factoryName, true, false);
			return factory;
		}
	} parameters;


	void prepare(const juce::dsp::ProcessSpec& spec_)
	{
		jassert(spec_.numChannels == 2);

		spec = spec_;
		parameters.prepare(spec);
		processor.prepare(spec);
		reset();
	}

	void reset()
	{
		processor.reset();
	}

	template <typename ProcessContext>
	void process(const ProcessContext& context) noexcept
	{
		processor.process(context);
	}

	std::unique_ptr<helpers::ParameterFactory> createParameters(const juce::String& factoryID,
																const juce::String& factoryName)
	{
		return processor.createParameters(factoryID, factoryName);
	}
private:

	juce::dsp::ProcessSpec spec{};
	Processor processor;


};

using LiveUtility = Utility<utility_configs::Balance,
							utility_configs::InputMatrix,
							utility_configs::MonoBass,
							utility_configs::OutputGain,
							utility_configs::DCBlock,
							utility_configs::Mute,
							utility_configs::PolarityInvert,
							utility_configs::StereoSpread>;
} // namespace sjf::dsp
