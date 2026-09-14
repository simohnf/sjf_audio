/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 12/09/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_ParameterFactory.h>

namespace sjf::dsp{
class Gate
{
public:
    struct Parameters : public helpers::AudioParametersBase
    {
        FloatState  attack, release, threshold, ratio;

        std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) override
        {
            auto factory = helpers::ParameterFactory::create (factoryID, factoryName);
            createTrackedTimeParameter  (*factory, attack, "Att",  "Attack",  0.0f, 200.0f, 20.0f, 1.0f, {});
            createTrackedTimeParameter  (*factory, release, "Rel",  "Release",  1.0f, 2000.0f, 200.0f, 100.0f, {});
            createTrackedDecibelParameter(*factory, threshold, "Thr", "Threshold", -60.0f, 0.0f, -6.0f, -6.0f, {});
	        {
            	auto range = NormalisableRange<float>{1, 50, 0.01f};
            	range.setSkewForCentre(2);
            	const auto attributes = AudioParameterFloatAttributes().withLabel(": 1");
            	createTrackedParameter(*factory, ratio, "Rat", "Ratio", range, 50.0f, {}, attributes);
	        }
            return factory;
        }
    } parameters;


    void prepare (const juce::dsp::ProcessSpec& spec_)
    {
        spec = spec_;
        parameters.prepare(spec);
    	gate.prepare(spec);
        reset();
    }

    void reset()
    {
        parameters.reset();
    	updateGateParams();
    	gate.reset();
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
        	for (size_t i = 0; i < numSamples; ++i)
        	{
        		parameters.tickSmoothers();
        		updateGateParams();
        		for ( auto chan = 0ul; chan < numChannels; ++chan)
        			outputBlock.getChannelPointer(chan)[i] = gate.processSample(static_cast<int>(chan), inputBlock.getChannelPointer(chan)[i]);
        	}
        }
        else
        {
        	gate.process(context);
        }
    }

    std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName)
    {
        return parameters.createParameters (factoryID, factoryName);
    }
private:
	void updateGateParams()
    {
	    gate.setAttack(parameters.attack.currentValue);
    	gate.setRelease(parameters.release.currentValue);
    	gate.setRatio(parameters.ratio.currentValue);
    	gate.setThreshold(parameters.threshold.currentValue);
    }

	juce::dsp::NoiseGate<float> gate;
    juce::dsp::ProcessSpec spec{};
};

}


