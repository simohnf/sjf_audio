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

namespace sjf::dsp
{
template<size_t MaxNumTaps = 64, size_t NumChannels = 2, size_t MaxEarlyReflectionTimeMS = 100>
class MultiTapDiffuser
{
public:

	struct Parameters : public helpers::AudioParametersBase
    {
        FloatState  diffusion;

		std::array<FloatState, MaxNumTaps> tapAmplitudes;

		std::unique_ptr<helpers::ParameterFactory> tapGroup;


        std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) override
        {
            auto factory = helpers::ParameterFactory::create (factoryID, factoryName);
        	createTrackedParameter(*factory, diffusion, "Diffusion", "Diffusion", {0.0f, 100.0f, 0.01f}, 50.0f, [](const float x){return x*0.01f;});

        	tapGroup = helpers::ParameterFactory::create(factoryID+"Taps", factoryName+"Taps");
        	for ( auto i = 0ul; i < MaxNumTaps; ++i )
        	{
        		auto mapping = [this, i](const float){ return tapAmplitudesPreCompute[i]; };
        		createTrackedParameter(*tapGroup, tapAmplitudes[i], "Amplitude"+String(i), "Amplitude "+String(i), {0.0f, 1.0f}, 0.0f, mapping);
        	}

            return factory;
        }

		void prepare(const juce::dsp::ProcessSpec& spec_)
        {
	        AudioParametersBase::prepare(spec_);
        	for ( auto chan = 0ul; chan < NumChannels; ++chan )
        	{
        		for (auto i = 0ul; i < MaxNumTaps; ++i)
        			tapTimes[chan][i] = static_cast<float>(spec.sampleRate)*0.001f*tapTimesMS[chan][i];
        	}
        }

		bool checkForStateChange()
        {
	        for ( auto t = 0ul; t < MaxNumTaps; ++t)
	        {
	        	const auto scale = pow(2.0f, jmap(1.0f-diffusion.getParameterValue()*0.01f, -1.0f, 1.0f ));
	        	const auto exponent = juce::MathConstants<float>::euler * scale;
	        	const auto polarity = helpers::functions::waveforms::wrapPhase(static_cast<float>(t) * juce::MathConstants<float>::sqrt2 / juce::MathConstants<float>::euler) > 0.75f ? -1.0f : 1.0f;
	        	tapAmplitudesPreCompute[t] =  polarity * std::pow(1.0f - static_cast<float>(t) / static_cast<float>(MaxNumTaps), exponent);
	        }

        	return AudioParametersBase::checkForStateChange();
        }

		static constexpr std::array<std::array<float, MaxNumTaps>, NumChannels> tapTimesMS = [](){
			std::array<std::array<float, MaxNumTaps>, NumChannels> arr{};

			constexpr auto rangePerTap = static_cast<float>(MaxEarlyReflectionTimeMS) / static_cast<float>(MaxNumTaps);
			constexpr auto rangeScaled = juce::MathConstants<float>::pi * juce::MathConstants<float>::euler * juce::MathConstants<float>::sqrt2 * rangePerTap;
			for ( auto i = 0ul; i < NumChannels; ++i)
			{
				for (size_t tap = 0; tap < MaxNumTaps; ++tap)
				{
					arr[i][tap] = static_cast<float>(tap) * rangePerTap;
					auto frac = static_cast<float>(i + 1)*static_cast<float>(tap + 1) / static_cast<float>(MaxNumTaps + 1);
					frac *= rangeScaled;
					frac /= rangePerTap;
					frac -= static_cast<float>(static_cast<int>(frac));
					arr[i][tap] += frac * rangePerTap;
				}
			}
			return arr;
        }();

		std::array<std::array<float, MaxNumTaps>, NumChannels> tapTimes{};
	private:
		std::array<float, MaxNumTaps> tapAmplitudesPreCompute{};
    } parameters;



    void prepare (const juce::dsp::ProcessSpec& spec_)
    {
        spec = spec_;
        parameters.prepare(spec);
    	for ( auto& dl : delayLine)
    	{
    		dl.prepare(spec);
    		dl.setMaxDelayTimeMS(static_cast<float>(MaxEarlyReflectionTimeMS) *1.1f);
    	}
        reset();
    }

    void reset()
    {
    	parameters.checkForStateChange();
        parameters.reset();
    	for ( auto& dl : delayLine)
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

		for ( auto chan = 0ul; chan < numChannels; ++chan)
		{
			auto inSamples = inputBlock.getChannelPointer(chan);
			auto outSamples = outputBlock.getChannelPointer(chan);
			for (auto i = 0ul; i < numSamples; ++i)
			{
				outSamples[i] = [&](float x){
					delayLine[chan].writeSample(x);
					x = 0.0f;
					for ( auto tap = 0ul; tap < MaxNumTaps; ++tap)
						x += delayLine[chan].template readSample<sjf::interpolation::InterpolatorTypes::linear>(parameters.tapTimes[chan][tap])
								* parameters.tapAmplitudes[tap].currentValue;
					return x;
				}(inSamples[i]);
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
			for ( auto chan = 0ul; chan < numChannels; ++chan)
			{
				auto inSamples = inputBlock.getChannelPointer(chan);
				auto outSamples = outputBlock.getChannelPointer(chan);
				outSamples[i] = [&](float x){
					delayLine[chan].writeSample(x);
					x = 0.0f;
					for ( auto tap = 0ul; tap < MaxNumTaps; ++tap)
						x += delayLine[chan].template readSample<sjf::interpolation::InterpolatorTypes::linear>(parameters.tapTimes[chan][tap])
									* parameters.tapAmplitudes[tap].currentValue;
					return x;
				}(inSamples[i]);
			}
		}
	}


	std::array<helpers::DelayLine, NumChannels> delayLine;
	juce::dsp::ProcessSpec spec{};
};


}
