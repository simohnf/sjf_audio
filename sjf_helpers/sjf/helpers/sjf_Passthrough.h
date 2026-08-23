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

namespace sjf::helpers{
/**
 * @brief A lightweight no-op (no-operation) dummy audio processor used as a base leaf node for DSP wrapper chains.
 *
 * This class implements the standard sjf_audio DSP processor interface (`prepare`, `reset`, `process`, `createParameters`)
 * as empty, zero-cost inline calls. It serves as an un-modified passthrough terminus at the core of nested template
 * wrapper structures (such as `sjf::dsp::Utility`), allowing surrounding wrappers to process audio in-place without
 * requiring a functional inner processor.
 */
class Passthrough
{
public:
    struct Parameters : public helpers::AudioParametersBase
    {
        std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) override
        {
            auto factory = helpers::ParameterFactory::create (factoryID, factoryName);

            return factory;
        }
    } parameters;


    void prepare (const juce::dsp::ProcessSpec&){}

    void reset() {}

    template <typename ProcessContext>
    void process (const ProcessContext&) noexcept {}

    std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName)
    {
        return parameters.createParameters (factoryID, factoryName);
    }

};

}


