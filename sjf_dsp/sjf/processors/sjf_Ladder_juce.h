/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 23/09/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_ParameterFactory.h>

namespace sjf::dsp{
class LadderFilter
{
	// only included this so drive can be smoothed along with other parameters
	struct LadderWithProcessSample : public juce::dsp::LadderFilter<float>
	{
		float processSample_(float inputValue, size_t channelToUse)
		{
			return processSample(inputValue, channelToUse);
		}

		void updateSmoothers_()
		{
			updateSmoothers();
		}
	};
public:
    struct Parameters : public helpers::AudioParametersBase
    {
        FloatState  cutoff, resonance, drive;
        ChoiceState mode;

        std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) override
        {
            auto factory = helpers::ParameterFactory::create (factoryID, factoryName);
            createTrackedFrequencyParameter (*factory, cutoff, "Cutoff",  "Cutoff Freq", 20.0f, 20000.0f, 1000.0f, 1000.0f ,[&](const float x){ return jlimit(20.0f, static_cast<float>(spec.sampleRate * 0.4999), x);});
            createTrackedPercentParameter  (*factory, resonance, "Resonance", "Resonance");
            createTrackedPercentParameter  (*factory, drive, "Drive",  "Drive", 0.0f, 100.0f, 50.0f, 0.0f, [](const float x) { return juce::jmap((x*0.01f)*(x*0.01f), 1.0f, 10.0f);});
        	createTrackedParameter  (*factory, mode, "Mode",    "Filter Mode",  { "LPF12", "HPF12", "BPF12", "LPF24", "HPF24", "BPF24" }, 0);

            return factory;
        }
    } parameters;


    void prepare (const juce::dsp::ProcessSpec& spec_)
    {
        spec = spec_;
        parameters.prepare(spec);
    	filter.prepare(spec);
    	driveSmoother.reset(spec.sampleRate, 0.05f);
        reset();
    }

    void reset()
    {
        parameters.reset();
    	updateFilterParameters(true);
    	filter.reset();
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
        	updateFilterParameters(false);

        	for (auto i = 0u; i < numSamples; i++)
        	{
        		filter.setDrive(driveSmoother.getNextValue());
        		for (auto ch = 0ul; ch < numChannels; ++ch)
        			outputBlock.getChannelPointer (ch)[i] = filter.processSample_ (inputBlock.getChannelPointer (ch)[i], ch);
        	}
        }
    	else
    	{
    		filter.process(context);
    	}
    }

    std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName)
    {
        return parameters.createParameters (factoryID, factoryName);
    }

private:
	void updateFilterParameters(const bool resetDriveSmoother)
	{
		filter.setCutoffFrequencyHz(parameters.cutoff.currentValue);
		filter.setResonance(parameters.resonance.currentValue);
		filter.setMode(static_cast<juce::dsp::LadderFilterMode>(parameters.mode.currentValue));
		if (resetDriveSmoother)
		{
			driveSmoother.setCurrentAndTargetValue(parameters.drive.currentValue);
			filter.setDrive(driveSmoother.getNextValue());
		}
		else
		{
			driveSmoother.setTargetValue(parameters.drive.currentValue);
		}
	}

	LadderWithProcessSample filter;
	juce::LinearSmoothedValue<float> driveSmoother;
    juce::dsp::ProcessSpec spec{};
};

}


