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

namespace sjf::dsp
{
	/// just a wrapper around the juce::dsp::Compressor class
	class Compressor
	{
	public:
		struct Parameters : public helpers::AudioParametersBase
		{
			FloatState  threshold, ratio, attack, release;


			std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) override
			{
				auto factory = helpers::ParameterFactory::create (factoryID, factoryName);
				{
					 auto range = NormalisableRange<float>{-60.0f, 0.0f, 0.01f};
					 range.setSkewForCentre(-6.0f);
					 const auto attributes = AudioParameterFloatAttributes().withLabel("dB");
					 createTrackedParameter(*factory, threshold, "Thr", "Threshold", range, range.end, {}, attributes);
				}
				{
					auto range = NormalisableRange<float>{1, 20, 0.01f};
					range.setSkewForCentre(2);
					const auto attributes = AudioParameterFloatAttributes().withLabel(": 1");
					createTrackedParameter(*factory, ratio, "Rat", "Ratio", range, 1.1f, {}, attributes);
				}
				{
					auto range = NormalisableRange<float>{0, 5000, 0.01f};
					range.setSkewForCentre(100);
					const auto attributes = AudioParameterFloatAttributes().withLabel("ms");
					createTrackedParameter(*factory, attack, "Att", "Attack", range, 10, {}, attributes);
				}
				{
					auto range = NormalisableRange<float>{0, 200, 0.01f};
					range.setSkewForCentre(10);
					const auto attributes = AudioParameterFloatAttributes().withLabel("ms");
					createTrackedParameter(*factory, release, "Rel", "Release", range, 100, {}, attributes);
				}

				return factory;
			}
		} parameters;




		void prepare (const juce::dsp::ProcessSpec& spec_)
		{
			spec = spec_;
			parameters.prepare(spec);
			compressor.prepare(spec);
			reset();
		}

		void reset()
		{
			parameters.reset();
			updateCompressorParams();
			compressor.reset();
		}

		template <typename ProcessContext>
		void process (const ProcessContext& context) noexcept
		{
			if (parameters.checkForStateChange())
			{
			   processSmoothedState(context);
			}
			else
			{
				compressor.process(context);
			}
		}


		std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName)
		{
			return parameters.createParameters (factoryID, factoryName);
		}

	private:
		template <typename ProcessContext>
		void processSmoothedState(const ProcessContext& context) noexcept
		{
			const auto& inputBlock = context.getInputBlock();
			auto& outputBlock      = context.getOutputBlock();
			const auto numChannels = outputBlock.getNumChannels();
			const auto numSamples  = outputBlock.getNumSamples();

			jassert (inputBlock.getNumChannels() == numChannels);
			jassert (inputBlock.getNumSamples()  == numSamples);

			for (auto i = 0ul; i < numSamples; i++)
			{
				parameters.tickSmoothers();
				updateCompressorParams();
				for (auto j = 0ul; j < numChannels; j++)
					outputBlock.getChannelPointer(j)[i] = compressor.processSample(static_cast<int>(j), inputBlock.getChannelPointer(j)[i]);
			}
		}

		void updateCompressorParams()
		{
			compressor.setRatio(parameters.ratio.currentValue);
			compressor.setAttack(parameters.attack.currentValue);
			compressor.setRelease(parameters.release.currentValue);
			compressor.setThreshold(parameters.threshold.currentValue);
		}

		juce::dsp::Compressor<float> compressor;
		juce::dsp::ProcessSpec spec{};
	};
}



//DUMMY_PLUGIN_SJF_COMPRESSOR_JUCE_H
