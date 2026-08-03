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
#include <sjf/helpers/sjf_DelayLine.h>

#include <sjf/helpers/sjf_ParameterFactory.h>
namespace sjf::dsp
{
	template<size_t RotationsOrder = 3, size_t NumDiffusionSteps = 3>
	class RotateDelayDiffuser {
	public:
		static constexpr auto MaxDiffusionLengthMS = 60.0f;


		static constexpr auto NumChannels = [](){
			static_assert(RotationsOrder > 0, "There must be at least 2 channels to rotate");
			static_assert(NumDiffusionSteps > 0, "There must be at least 1 diffusion step");
			auto i = 1ul;
			auto ret = 1;
			while (i < NumDiffusionSteps)
			{
				ret *= 2;
				i++;

			}
			return ret;
		}();

		static constexpr auto DelayTimesMS = reverb_helpers::ReverbDelayTimeCalculator::calculateMsDelayTimes<NumChannels*NumDiffusionSteps, 1.0f, 60.0f, reverb_helpers::ReverbDelayTimeCalculator::SpacingType::Stochastic>();


		static constexpr std::array<std::array<size_t, NumChannels>, NumDiffusionSteps> shuffledIndices = [](){
			std::array<std::array<size_t, NumChannels>, NumDiffusionSteps> arr{};
			auto offset = juce::MathConstants<float>::euler * juce::MathConstants<float>::sqrt2 / juce::MathConstants<float>::pi;
			for (auto s = 0ul; s < NumDiffusionSteps; s++)
			{
				std::array<size_t, NumChannels> indices{};
				for (size_t c = 0; c < NumChannels; c++)
					indices[c] = c;

				size_t currentSize = NumChannels;

				for (auto c = 0ul; c < NumChannels; c++)
				{
					const auto unwrapped = offset * juce::MathConstants<float>::pi / static_cast<float>(currentSize);

					const auto wrapped = unwrapped - static_cast<float>(static_cast<int>(unwrapped));

					const auto index1 = static_cast<size_t>(wrapped * static_cast<float>(currentSize));
					//
					arr[s][c] = indices[index1];

					for (size_t i = index1; i + 1 < currentSize; ++i)
						indices[i] = indices[i + 1];

					--currentSize;

					offset += (c & 1) ? juce::MathConstants<float>::euler : juce::MathConstants<float>::sqrt2;

				}
			}
			return arr;
		}();

		static constexpr std::array<std::array<bool, NumChannels>, NumDiffusionSteps> polarityFlip = [](){
			std::array<std::array<bool, NumChannels>, NumDiffusionSteps> arr{};
			auto offset = juce::MathConstants<float>::euler * juce::MathConstants<float>::sqrt2 * juce::MathConstants<float>::pi;
			for ( auto s = 0ul; s < NumDiffusionSteps; s++)
			{
				for (auto c = 0ul; c < NumChannels; c++)
				{
					arr[s][c] = sjf::helpers::functions::waveforms::wrapPhase(offset * juce::MathConstants<float>::pi)  > 0.75f;
					offset += c & 1 ? juce::MathConstants<float>::sqrt2 * juce::MathConstants<float>::pi : juce::MathConstants<float>::euler / juce::MathConstants<float>::halfPi;
				}
			}
			return arr;
		}();

		struct Parameters : public sjf::helpers::AudioParametersBase
		{
			std::unique_ptr<sjf::helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) override
			{
				auto factory = sjf::helpers::ParameterFactory::create (factoryID, factoryName);
				return factory;
			}
		} parameters;


		void prepare (const juce::dsp::ProcessSpec& spec_)
		{
			spec = spec_;
			parameters.prepare(spec);
			diffusionBuffer.setSize(NumChannels, static_cast<int>(spec.maximumBlockSize));

			reverb_helpers::ReverbDelayTimeCalculator::calculateCoprimeSampleTimes< reverb_helpers::ReverbDelayTimeCalculator::FillMode::Column,
																					reverb_helpers::ReverbDelayTimeCalculator::ShuffleMode::Row>
																						(DelayTimesMS, delayTimesSamps, spec.sampleRate);
			for ( auto s = 0ul; s < NumDiffusionSteps; s++)
			{
				for (auto c = 0ul; c < NumChannels; c++)
				{
					auto& dl = delayLines[s][c];
					dl.prepare(spec);
					dl.setMaxDelayTimeMS(MaxDiffusionLengthMS * 2.0f);
				}
			}

			reset();
		}

		void reset()
		{
			parameters.reset();
			for ( auto s = 0ul; s < NumDiffusionSteps; s++)
				for (auto c = 0ul; c < NumChannels; c++)
					delayLines[s][c].reset();
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

			diffusionBuffer.clear();
			auto diffusionBlock = juce::dsp::AudioBlock<float>(diffusionBuffer);
			diffusionBlock.copyFrom(inputBlock);
			applyHadamardDirectToBuffer(diffusionBuffer);

			juce::dsp::ProcessContextReplacing<float> diffusionContext { diffusionBlock };

			// if (parameters.checkForStateChange())
			// {
			//     processSmoothedState(diffusionContext);
			// }
			// else
			{
				processStaticState(diffusionContext);
			}

			// this breaks down if input channels are greater than diffusion channels!!!
			for (auto c = 0ul; c < numChannels; c++)
			{
				auto c2 = c % NumChannels;
				outputBlock.getSingleChannelBlock(c).copyFrom(diffusionBlock.getSingleChannelBlock(c2));
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

			for ( auto stage = 0ul; stage < NumDiffusionSteps; stage++)
			{
				for (size_t channel = 0; channel < NumChannels; ++channel)
				{
					auto* inputSamples  = inputBlock.getChannelPointer (channel);
					auto* outputSamples = outputBlock.getChannelPointer (shuffledIndices[stage][channel]);

					for (size_t i = 0; i < numSamples; ++i)
					{
						delayLines[stage][channel].writeSample(inputSamples[i]);
						outputSamples[i] = delayLines[stage][channel].readSample(delayTimesSamps[stage][channel]);
					}
					if (polarityFlip[stage][channel])
						outputBlock.getSingleChannelBlock(channel).multiplyBy(-1.0f);
				}
				applyHadamardDirectToBuffer(diffusionBuffer);
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

				// Sample-by-sample audio processing logic occurs here
			}
		}

		void applyHadamardDirectToBuffer(juce::AudioBuffer<float>& buffer)
		{
			jassert(buffer.getNumChannels() == NumChannels);
			const auto numSamples = static_cast<size_t>(buffer.getNumSamples());
			auto* const* channels = buffer.getArrayOfWritePointers();

			// 1. Fast Walsh-Hadamard Transform across channel pointers
			for (auto len = 1ul; len < NumChannels; len <<= 1)
			{
				for (auto i = 0ul; i < NumChannels; i += 2 * len)
				{
					for (auto j = 0ul; j < len; ++j)
					{
						auto* chA = channels[i + j];
						auto* chB = channels[i + len + j];

						// Butterfly combination over the entire buffer length
						for (auto s = 0ul; s < numSamples; ++s)
						{
							const auto a = chA[s];
							const auto b = chB[s];
							chA[s] = a + b;
							chB[s] = a - b;
						}
					}
				}
			}
			buffer.applyGain(scalingFactor);
		}

		juce::dsp::ProcessSpec spec{};
		std::array<std::array<sjf::helpers::DelayLine, NumChannels>, NumDiffusionSteps> delayLines{};
		std::array<std::array<size_t, NumChannels>, NumDiffusionSteps> delayTimesSamps{};
		juce::AudioBuffer<float> diffusionBuffer;
		const float scalingFactor = static_cast<float>(std::sqrt(1.0f / static_cast<float>(NumChannels)));
	};
}