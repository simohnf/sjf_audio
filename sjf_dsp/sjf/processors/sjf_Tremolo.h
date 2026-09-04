/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 04/09/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_ParameterFactory.h>
#include <sjf/oscillators/LFO/sjf_LFO.h>


namespace sjf::dsp
{
template<typename LFO>
class Tremolo
{
public:
	static_assert(sjf::helpers::functions::utilities::is_instantiation_of<sjf::dsp::oscillators::lfo::LFO, LFO>, "The LFO must be an instantiation of sjf::dsp::oscillators::lfo");

    struct Parameters : public helpers::AudioParametersBase
    {

        std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) override
        {
            auto factory = helpers::ParameterFactory::create (factoryID, factoryName, true, false);

            return factory;
        }
    } parameters;


    void prepare (const juce::dsp::ProcessSpec& spec_)
    {
        spec = spec_;
        parameters.prepare(spec);
    	lfo.prepare(spec);
        reset();
    }

    void reset()
    {
        parameters.reset();
    	lfo.reset();
    }

    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
        [[maybe_unused]] const auto& inputBlock = context.getInputBlock();
        [[maybe_unused]] auto& outputBlock      = context.getOutputBlock();
        [[maybe_unused]] const auto numChannels = outputBlock.getNumChannels();
        [[maybe_unused]] const auto numSamples  = outputBlock.getNumSamples();

        jassert (inputBlock.getNumChannels() == numChannels);
        jassert (inputBlock.getNumSamples() == numSamples);

        if (parameters.checkForStateChange())
        {
        	parameters.reset();
        	jassertfalse;
        }

    	if constexpr(ProcessContext::usesSeparateInputAndOutputBlocks())
    		outputBlock.copyFrom(inputBlock);

    	lfo.process(context);
    	auto lfoBlock = lfo.getLfoOutput().getSubBlock(0, numSamples);

    	for ( auto channel = 0ul; channel < numChannels; ++channel )
    	{
    		auto samples = outputBlock.getSingleChannelBlock(channel);
    		auto lfoChannel = lfoBlock.getSingleChannelBlock(channel % lfoBlock.getNumChannels());
    		samples.multiplyBy(lfoChannel);
    	}
    }

    std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName)
    {
    	return lfo.createParameters (factoryID, factoryName);
    }

    void setPositionInfo(const juce::AudioPlayHead::PositionInfo& positionInfo)
    {
    	lfo.setPositionInfo(positionInfo);
    }


private:
    juce::dsp::ProcessSpec spec{};
	LFO lfo;
};


namespace internal
{
	using TremoloLFO = sjf::dsp::oscillators::lfo::LFO<dsp::oscillators::lfo::LFOWaveformProvider<dsp::oscillators::lfo::Sine>,
												   dsp::oscillators::lfo::lfo_config::PhaseOffset,
												   dsp::oscillators::lfo::lfo_config::Smooth,
												   dsp::oscillators::lfo::lfo_config::Unipolar,
												   dsp::oscillators::lfo::lfo_config::Depth>;
}

using BasicTremolo = Tremolo<internal::TremoloLFO>;


}


