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
    namespace preset_manager::ids
    {
        const static juce::Identifier idId{"id"};
        const static juce::Identifier presetNameId{"preset"};
        const static juce::Identifier value{"value"};
        const static juce::Identifier param{"Param"};
    }

class PresetManager
{
    public:
        static juce::File getProjectWriteableRoot()
        {
            const static auto userMusicDir = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userMusicDirectory);
            const static auto projectDir = userMusicDir.getChildFile("sjf").getChildFile(JucePlugin_Name);
            ignoreUnused(projectDir.createDirectory());
            return projectDir;
        }

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

        static void savePreset(const juce::AudioProcessorParameterGroup& group, const juce::String& presetName, const juce::String& extension = ".sjf")
        {
            jassert(extension.startsWith ("."));

            const auto folderName = getGroupFolderName(group);
            auto targetDir = getProjectWriteableRoot().getChildFile(folderName);

            if (targetDir.createDirectory())
            {
                auto preset = getValueTree(group, true);
                preset.setProperty(preset_manager::ids::presetNameId, presetName, nullptr);

                auto presetFile = targetDir.getChildFile(presetName + extension);
                saveToFile(preset, presetFile);
            }
            else
            {
                jassertfalse;
            }
        }

        static void loadPreset(const juce::AudioProcessorParameterGroup& group, const juce::String& presetName, const juce::String& extension = ".sjf")
        {
            jassert(extension.startsWith ("."));

            const auto folderName = getGroupFolderName(group);
            auto presetFile = getProjectWriteableRoot().getChildFile(folderName).getChildFile(presetName + extension);

            if (presetFile.existsAsFile())
            {
                auto vt = loadFromFile(presetFile);
                if (vt.isValid())
                    loadFromVT(group, vt, true);
            }
            else
            {
                jassertfalse;
            }
        }

    private:
        // Helper to guarantee savePreset and loadPreset resolve the directory identically
        static juce::String getGroupFolderName(const juce::AudioProcessorParameterGroup& group)
        {
            auto id = ParameterFactory::getIDWithoutParentPrefix(group);
            if (id.isEmpty())
                id = juce::String(JucePlugin_Name).replace(" ", "");
            return id;
        }

        static ValueTree getValueTree(const AudioProcessorParameterGroup& group, bool recursive = true)
        {
            const auto id = ParameterFactory::getIDWithoutParentPrefix(group);
            auto vt = ValueTree{ id.isEmpty() ? juce::String(JucePlugin_Name).replace(" ", "") : id };

            for (const auto& param : group.getParameters(false))
            {
                if (const auto rangedParam = dynamic_cast<RangedAudioParameter*>(param))
                {
                    auto paramVT = ValueTree{ ParameterFactory::getIDWithoutParentPrefix(*rangedParam, group) };
                    paramVT.setProperty(preset_manager::ids::value, rangedParam->convertFrom0to1(rangedParam->getValue()), nullptr);
                    vt.addChild(paramVT, -1, nullptr);
                }
            }

            if (recursive)
            {
                for (const auto* child : group.getSubgroups(false))
                {
                    if (child != nullptr)
                        vt.addChild(getValueTree(*child, true), -1, nullptr);
                }
            }

            return vt;
        }

        static void loadFromVT(const AudioProcessorParameterGroup& group, const ValueTree& vt, bool recursive = true)
        {
            for (auto* node : group)
            {
                if (auto* parameter = node->getParameter())
                {
                    if (const auto ranged = dynamic_cast<RangedAudioParameter*>(parameter))
                        if (auto pVT = vt.getChildWithName(ParameterFactory::getIDWithoutParentPrefix(*ranged, group)); pVT.isValid())
                            if (pVT.hasProperty(preset_manager::ids::value))
                                ranged->setValueNotifyingHost(ranged->convertTo0to1(pVT.getProperty(preset_manager::ids::value)));
                }
                else if (recursive)
                {
                    if (const auto childGroup = node->getGroup())
                        if (auto childVT = vt.getChildWithName(ParameterFactory::getIDWithoutParentPrefix(*childGroup)); childVT.isValid())
                            loadFromVT(*childGroup, childVT, true);
                }
            }
        }

        static void saveToFile(const juce::ValueTree& vt, const juce::File& targetFile)
        {
            auto mb = toMemoryBlock(vt);
            targetFile.replaceWithData(mb.getData(), mb.getSize());
        }

        static juce::ValueTree loadFromFile(const juce::File& fileToLoad)
        {
            if (fileToLoad.existsAsFile())
            {
                juce::MemoryBlock mb;
                if (fileToLoad.loadFileAsData(mb))
                    return toValueTree(mb);
            }

            return {};
        }
    };
}