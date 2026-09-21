/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 21/09/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_ParameterFactory.h>

namespace sjf::dsp{
class Reverb_juce
{
public:
    struct Parameters : public helpers::AudioParametersBase
    {
        FloatState  size, damping, width;

        std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) override
        {
            auto factory = helpers::ParameterFactory::create (factoryID, factoryName);
            createTrackedPercentParameter(*factory, size, "Size", "Size", 0.0f, 100.0f, 50.0f, 50.0f);
            createTrackedPercentParameter(*factory, damping, "Damp", "Damping", 0.0f, 100.0f, 50.0f, 50.0f);
            createTrackedPercentParameter(*factory, width, "Width", "Width");

            return factory;
        }
    } parameters;


    void prepare (const juce::dsp::ProcessSpec& spec_)
    {
        spec = spec_;
        parameters.prepare(spec);
    	tank.prepare(spec);
        reset();
    }

    void reset()
    {
        parameters.reset();
    	updateReverbParams();
    	tank.reset();
    }

    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
        if (parameters.checkForStateChange())
        {
        	parameters.reset();
        	updateReverbParams();
        }

    	tank.process (context);

    }

    std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName)
    {
        return parameters.createParameters (factoryID, factoryName);
    }


private:
	void updateReverbParams()
	{
		juce::dsp::Reverb::Parameters parameters_;
		parameters_.damping = parameters.damping.currentValue;
		parameters_.width = parameters.width.currentValue;
		parameters_.roomSize = parameters.size.currentValue;
		parameters_.dryLevel = 0.0f;
		parameters_.wetLevel = 1.0f;
		tank.setParameters (parameters_);
	}

	juce::dsp::Reverb	tank;
    juce::dsp::ProcessSpec spec{};
};

}


