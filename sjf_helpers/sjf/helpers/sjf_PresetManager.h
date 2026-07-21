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
        const static juce::Identifier savedPreset{"SavedPreset"};
    }

	namespace preset_manager::strings
    {
    	const static juce::String savePreset = "Save Preset";
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

            const auto folderName = getGroupIDWithNoSpaces(group);
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

            const auto folderName = getGroupIDWithNoSpaces(group);
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
        
        
        static juce::PopupMenu getPresetPopupMenu(const juce::AudioProcessorParameterGroup& group, const juce::String& extension = ".sjf")
        {
            juce::PopupMenu m;

            auto itemId = populatePresetPopupMenu(1, m, group, extension);
            m.addSeparator();
            auto save = PopupMenu::Item();
            save.itemID = itemId;
            save.text = preset_manager::strings::savePreset;
            save.isEnabled = true;
            save.isTicked = false;
            save.action = [&, ext = extension]()
            {
                auto* alert = new juce::AlertWindow (preset_manager::strings::savePreset, "Please enter a name for your preset:", juce::MessageBoxIconType::NoIcon);

                alert->addTextEditor ("presetName", "My Preset", "Preset Name:");
                alert->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
                alert->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

                alert->enterModalState (true, juce::ModalCallbackFunction::create (
                    [alert, &group, e = ext] (int result)
                    {
                        if (result == 1) // User clicked Save
                        {
                            auto presetName = alert->getTextEditorContents ("presetName");
                            PresetManager::savePreset(group, presetName, e);
                        }
                        delete alert;
                    }));
            };
            m.addItem(save);

            itemId ++;
            return m;
        }

    private:
        // Helper to guarantee savePreset and loadPreset resolve the directory identically
        static juce::String getGroupIDWithNoSpaces(const juce::AudioProcessorParameterGroup& group)
        {
            auto id = ParameterFactory::getIDWithoutParentPrefix(group);
            if (id.isEmpty())
                id = juce::String(JucePlugin_Name).replace(" ", "");
            return id;
        }

        static ValueTree getValueTree(const AudioProcessorParameterGroup& group, bool recursive = true)
        {
            const auto id = getGroupIDWithNoSpaces(group);
            auto vt = ValueTree{id};

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


        static int populatePresetPopupMenu(int startId, PopupMenu& m, const juce::AudioProcessorParameterGroup& group, const juce::String& extension = ".sjf")
        {
            auto id = getGroupIDWithNoSpaces(group);
            auto dir = getProjectWriteableRoot().getChildFile(id);
            if (dir.isDirectory())
            {
                auto whatToLookFor = juce::File::TypesOfFileToFind::findFiles | juce::File::TypesOfFileToFind::ignoreHiddenFiles;
                auto arr = dir.findChildFiles(whatToLookFor, false, "*"+extension, File::FollowSymlinks::noCycles);
                for (const auto& file : arr)
                {
                    auto item = PopupMenu::Item();
                    item.action = [&, name = file.getFileNameWithoutExtension(), e = extension](){
                        loadPreset(group, name, e);
                    };
                    item.itemID = startId;
                    item.text = file.getFileNameWithoutExtension();
                    item.isTicked = false;
                    item.isEnabled = true;
                    m.addItem(item);

                    startId++;

                }

                for ( const auto& subDir : dir.findChildFiles(juce::File::TypesOfFileToFind::findDirectories, false, "*"+extension, File::FollowSymlinks::noCycles))
                {
                    auto subMenu = PopupMenu();
                    startId = populatePresetPopupMenu(startId, subMenu, group, extension);
                    m.addSubMenu(subDir.getFileName(), subMenu);
                }
            }
            return startId;
        }
    };
}