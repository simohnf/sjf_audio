/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_ParameterFactory.h>

namespace sjf::helpers
{

/**
    Converts a mono processor class into a multi-channel version by duplicating it
    and applying multichannel buffers across an array of instances.

    When the prepare method is called, it uses the specified number of channels to
    instantiate the appropriate number of instances, which it then uses in its
    process() method.

    @tags{DSP}
*/
template <typename MonoProcessorType>
struct ProcessorDuplicator
{
    struct Parameters : MonoProcessorType::Parameters
    {
        using MonoParams = typename MonoProcessorType::Parameters;
        using MonoParams::MonoParams;
    };

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        parameters.prepare (spec);

        auto sizeBefore = processors.size();

        processors.removeRange ((int) spec.numChannels, processors.size());

        while (static_cast<size_t> (processors.size()) < spec.numChannels)
            processors.add (new MonoProcessorType ());

        if ( processors.size() != sizeBefore )
            updateMasterParameters();

        auto monoSpec = spec;
        monoSpec.numChannels = 1;

        for (auto* p : processors)
            p->prepare (monoSpec);
    }

    void reset() noexcept
    {
        parameters.reset();

        for (auto* p : processors) p->reset();
    }

    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
        jassert ((int) context.getInputBlock().getNumChannels()  <= processors.size());
        jassert ((int) context.getOutputBlock().getNumChannels() <= processors.size());

        auto numChannels = static_cast<size_t> (jmin (context.getInputBlock().getNumChannels(),
                                                      context.getOutputBlock().getNumChannels()));


        const auto& inputBlock = context.getInputBlock();
        auto& outputBlock      = context.getOutputBlock();

        for (size_t chan = 0; chan < numChannels; ++chan)
        {
            auto inBlock = inputBlock.getSingleChannelBlock(chan);
            auto outBlock = outputBlock.getSingleChannelBlock(chan);
            auto monoContext = [&]()
            {
                if constexpr (ProcessContext::usesSeparateInputAndOutputBlocks())
                    return ProcessContext(inBlock, outBlock);
                else
                    return ProcessContext(outBlock);
            }();

            processors[(int) chan]->process (monoContext);
        }
    }

    std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName)
    {
        dummyParameterFactory = ParameterFactory::create("Dummy", "Dummy");

        id = factoryID;
        name = factoryName;

        auto factory = parameters.createParameters (factoryID, factoryName);
        updateMasterParameters();



        return factory;
    }

private:

    void updateMasterParameters()
    {
        for ( auto* p : processors)
        {
            dummyParameterFactory->addChildFactory(p->createParameters (id, name));
            p->parameters.setMasterAudioParameters(&parameters);
        }
    }

    juce::OwnedArray<MonoProcessorType> processors;
    Parameters parameters;
    std::unique_ptr<ParameterFactory> dummyParameterFactory;
    String id, name;
};

}

