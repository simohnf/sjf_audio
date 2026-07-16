//
// Created by Simon Fay on 10/07/2026.
//

#pragma once

#include <JuceHeader.h>

#include <sjf/helpers/sjf_ParameterFactory.h>

#include "sjf_OptionalCalls.h"

namespace sjf::helpers
{
template <typename Processor>
class ChunkedWrapper
{
    static constexpr size_t MAX_CHUNK_SIZE = 32;

public:
    template <typename... Args>
    ChunkedWrapper (Args&&... args) : processor (std::forward<Args> (args)...) {}

    /** Initialises the processor and internal specs. */
    void prepare (const juce::dsp::ProcessSpec& spec_)
    {
        spec = spec_;

        juce::dsp::ProcessSpec chunkSpec = spec_;
        chunkSpec.maximumBlockSize = MAX_CHUNK_SIZE;

        processor.prepare (chunkSpec);
    }

    /** Resets the internal state variables of the processor. */
    void reset()
    {
        processor.reset();
    }

    //==============================================================================
    /** Processes the input and output samples supplied in the processing context. */
    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
        const auto& inputBlock = context.getInputBlock();
        auto& outputBlock      = context.getOutputBlock();
        const auto numSamples  = inputBlock.getNumSamples();

        auto getSubContext = [](auto& inBlock, auto& outBlock)
        {
            if constexpr(ProcessContext::usesSeparateInputAndOutputBlocks())
                return ProcessContext(inBlock, outBlock);
            else
                return ProcessContext(outBlock);
        };

        size_t samplesProcessed = 0;

        // TODO: positionInfo needs to be updated for each subblock....
        while (samplesProcessed < numSamples)
        {
            const auto chunkSize = std::min (MAX_CHUNK_SIZE, numSamples - samplesProcessed);

            auto inputSubBlock  = inputBlock.getSubBlock (samplesProcessed, chunkSize);
            auto outputSubBlock = outputBlock.getSubBlock (samplesProcessed, chunkSize);

            auto subContext = getSubContext (inputSubBlock, outputSubBlock);

            processor.process (subContext);

            samplesProcessed += chunkSize;
        }
    }

    template <typename... Args>
    std::unique_ptr<ParameterFactory> createParameters (
        const juce::String& factoryID,
        const juce::String& factoryName,
        Args&&... configArgs)
    {
        // Simply unpacks everything and forwards the arguments down to the child processor
        return processor.createParameters (factoryID,
                                           factoryName,
                                           std::forward<Args> (configArgs)...);
    }

    Processor& getProcessor() { return processor; }
    const Processor& getProcessor() const { return processor; }

    void setPositionInfo(const Optional<juce::AudioPlayHead::PositionInfo>& positionInfo)
    {
        sjf::optional_calls::setPositionInfo(processor, positionInfo);
    }
private:
    Processor processor;
    juce::dsp::ProcessSpec spec;
};
}
