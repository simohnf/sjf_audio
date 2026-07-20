/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */

//
// Created by Simon Fay on 10/07/2026.
//

#pragma once

#include <JuceHeader.h>

namespace sjf::helpers
{
class PresetManager
{
    public:
        static ValueTree toValueTree(const MemoryBlock& mb)
        {
            juce::MemoryInputStream stream (mb, false);
            return juce::ValueTree::readFromStream (stream);
        }

        static ValueTree toValueTree(const void* data, const int sizeInBytes)
        {
            return toValueTree(juce::MemoryBlock(data, static_cast<size_t>(sizeInBytes)));
        }

        static MemoryBlock toMemoryBlock(const ValueTree& vt)
        {
            auto mb = juce::MemoryBlock();
            juce::MemoryOutputStream stream (mb, false);

            vt.writeToStream (stream);
            return mb;

        }
    private:
};
}