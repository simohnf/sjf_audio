/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 11/08/2026.
//

#pragma once
#include <JuceHeader.h>
#include <ranges>
#include <sjf/helpers/sjf_ParameterFactory.h>

namespace sjf::dsp
{
/// just a wrapper around the juce::dsp::Limiter class
class Limiter
{
	static constexpr auto ThresholdMin = -60.0f;
	static constexpr auto ThresholdMax = 0.0f;
public:
    struct Parameters : public helpers::AudioParametersBase
    {
        /// NOTE: You need to ensure all TrackedState objects of a given type are declared consecutively!!!
        FloatState  threshold, release;


        std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) override
        {
            auto factory = helpers::ParameterFactory::create (factoryID, factoryName);
            {
                 auto range = NormalisableRange<float>{ThresholdMin, ThresholdMax, 0.01f};
                 range.setSkewForCentre(-6.0f);
                 const auto attributes = AudioParameterFloatAttributes().withLabel("dB");
                 createTrackedParameter(*factory, threshold, "Thr", "Threshold", range, ThresholdMax, {}, attributes);
            }
            {
                auto range = NormalisableRange<float>{0, 200, 0.01f};
                range.setSkewForCentre(5);
                const auto attributes = AudioParameterFloatAttributes().withLabel("ms");
                createTrackedParameter(*factory, release, "Rel", "Release", range, 10, {}, attributes);
            }

            return factory;
        }
    } parameters;




    void prepare (const juce::dsp::ProcessSpec& spec_)
    {
        spec = spec_;
        parameters.prepare(spec);
        limiter.prepare(spec);
        reset();
    }

    void reset()
    {
        parameters.reset();
        limiter.setRelease(parameters.release.currentValue);
        limiter.setThreshold(parameters.threshold.currentValue);
        limiter.reset();
    }

    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
    	   if (parameters.checkForStateChange())
    	   {
    	   		parameters.reset();
    	   		limiter.setRelease(parameters.release.currentValue);
				limiter.setThreshold(parameters.threshold.currentValue);
    	   }
        limiter.process(context);
    }


    std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName)
    {
        return parameters.createParameters (factoryID, factoryName);
    }

private:
    juce::dsp::Limiter<float> limiter;
    juce::dsp::ProcessSpec spec{};
};
}



//DUMMY_PLUGIN_SJF_LIMITER_JUCE_H
