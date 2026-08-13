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
#include <sjf/helpers/sjf_ParameterFactory.h>
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
    	using AfterSaveCallback = std::function<void(juce::ValueTree)>;
    	using AfterLoadCallback = std::function<void(juce::ValueTree)>;

    	static juce::String getDefaultExtension()
    	{
    		const static auto defaultExtension =  juce::String{".sjf"};
    		return defaultExtension;
    	}

        static juce::File getProjectWriteableRoot()
        {
            const static auto userMusicDir = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userMusicDirectory);
            const static auto projectDir = userMusicDir.getChildFile("sjf").getChildFile(JucePlugin_Name);
            ignoreUnused(projectDir.createDirectory());
            return projectDir;
        }

        static juce::ValueTree toValueTree(const juce::MemoryBlock& mb)
        {
            juce::MemoryInputStream stream (mb, false);
            return juce::ValueTree::readFromStream (stream);
        }

        static juce::ValueTree toValueTree(const void* data, const int sizeInBytes)
        {
            return toValueTree(juce::MemoryBlock(data, static_cast<size_t>(sizeInBytes)));
        }

        static juce::MemoryBlock toMemoryBlock(const juce::ValueTree& vt)
        {
            auto mb = juce::MemoryBlock();
            juce::MemoryOutputStream stream (mb, false);
            vt.writeToStream (stream);
            return mb;
        }



        static void savePreset(const juce::AudioProcessorParameterGroup& group, const juce::String& presetName, const juce::String& extension,const AfterSaveCallback& afterSave)
        {
            jassert(extension.startsWith ("."));

            const auto folderName = getGroupNameWithNoSpaces(group);
            auto targetDir = getProjectWriteableRoot().getChildFile(folderName);

            if (targetDir.createDirectory())
            {
                auto preset = saveToVT(group, true);
            	if (afterSave)
            		afterSave(preset);

                preset.setProperty(preset_manager::ids::presetNameId, presetName, nullptr);

                auto presetFile = targetDir.getChildFile(presetName + extension);
                saveToFile(preset, presetFile);
            }
            else
            {
                jassertfalse;
            }
        }

        static void loadPreset(const juce::AudioProcessorParameterGroup& group, const juce::String& presetName, const juce::String& extension, const AfterLoadCallback& afterLoad)
        {
            jassert(extension.startsWith ("."));

            const auto folderName = getGroupNameWithNoSpaces(group);
            auto presetFile = getProjectWriteableRoot().getChildFile(folderName).getChildFile(presetName + extension);

            if (presetFile.existsAsFile())
            {
                auto vt = loadFromFile(presetFile);
                if (vt.isValid())
                {
	                loadFromVT(group, vt, true);
                	if (afterLoad)
						afterLoad(vt);
                }
            }
            else
            {
                jassertfalse;
            }
        }
        
        
        static juce::PopupMenu getPresetPopupMenu(const juce::AudioProcessorParameterGroup& group, const juce::String& extension = getDefaultExtension(), juce::Component::SafePointer<juce::ComboBox> parent = nullptr, const AfterSaveCallback& afterSave = {}, const AfterLoadCallback& afterLoad = {})
        {
            juce::PopupMenu m;

            auto itemId = populatePresetPopupMenu(1, m, group, extension, afterLoad);
            m.addSeparator();
            auto save = juce::PopupMenu::Item();
            save.itemID = itemId;
            save.text = preset_manager::strings::savePreset;
    		auto test = afterSave;
            save.isEnabled = true;
            save.isTicked = false;
            save.action = [&, ext = extension, parent, as = afterSave]()
            {
                auto* alert = new juce::AlertWindow (preset_manager::strings::savePreset, "Please enter a name for your preset:", juce::MessageBoxIconType::NoIcon);

                alert->addTextEditor ("presetName", "My Preset", "Preset Name:");
                alert->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
                alert->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

                alert->enterModalState (true, juce::ModalCallbackFunction::create (
                    [alert, &group, e = ext, parent, as] (int result)
                    {
                        if (result == 1) // User clicked Save
                        {
                            auto presetName = alert->getTextEditorContents ("presetName");
                            PresetManager::savePreset(group, presetName, e, as);
                        	if (parent)
                        		parent->getProperties().set(preset_manager::ids::savedPreset, presetName);
                        }
                        delete alert;
                    }));
            };
            m.addItem(save);

            itemId ++;
            return m;
        }

    	static juce::ValueTree saveToVT(const juce::AudioProcessorParameterGroup& group, bool recursive = true)
        {
            const auto id = getGroupIDWithNoSpaces(group);
            auto vt = juce::ValueTree{id};

            for (const auto& param : group.getParameters(false))
            {
                if (const auto rangedParam = dynamic_cast<juce::RangedAudioParameter*>(param))
                {
                    auto paramVT = juce::ValueTree{ ParameterFactory::getIDWithoutParentPrefix(*rangedParam, group) };
                    paramVT.setProperty(preset_manager::ids::value, rangedParam->convertFrom0to1(rangedParam->getValue()), nullptr);
                    vt.addChild(paramVT, -1, nullptr);
                }
            }

            if (recursive)
            {
                for (const auto* child : group.getSubgroups(false))
                {
                    if (child != nullptr)
                        vt.addChild(saveToVT(*child, true), -1, nullptr);
                }
            }

            return vt;
        }

        static void loadFromVT(const juce::AudioProcessorParameterGroup& group, const juce::ValueTree& vt, bool recursive = true)
        {
            for (auto* node : group)
            {
                if (auto* parameter = node->getParameter())
                {
                    if (const auto ranged = dynamic_cast<juce::RangedAudioParameter*>(parameter))
                        if (auto pVT = vt.getChildWithName(ParameterFactory::getIDWithoutParentPrefix(*ranged, group)); pVT.isValid())
                            if (pVT.hasProperty(preset_manager::ids::value))
                                ranged->setValueNotifyingHost(ranged->convertTo0to1(pVT.getProperty(preset_manager::ids::value)));
                }
                else if (recursive)
                {
                    if (const auto childGroup = node->getGroup())
                        if (auto childVT = vt.getChildWithName(getGroupIDWithNoSpaces(*childGroup)); childVT.isValid())
                            loadFromVT(*childGroup, childVT, true);
                }
            }
        }

		// Helper to guarantee savePreset and loadPreset resolve the directory identically
        static juce::String getGroupNameWithNoSpaces(const juce::AudioProcessorParameterGroup& group)
        {
            auto id = ParameterFactory::getNameWithoutParentPrefix(group).replace(" ", "");
            if (id.isEmpty())
                id = juce::String(JucePlugin_Name).replace(" ", "");
            return id;
        }

    	static juce::String getGroupIDWithNoSpaces(const juce::AudioProcessorParameterGroup& group)
        {
        	auto id = ParameterFactory::getIDWithoutParentPrefix(group).replace(" ", "");
        	if (id.isEmpty())
        		id = juce::String(JucePlugin_Name).replace(" ", "");
        	return id;
        }

    private:



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


        static int populatePresetPopupMenu(int startId, juce::PopupMenu& m, const juce::AudioProcessorParameterGroup& group, const juce::String& extension, const AfterLoadCallback& afterLoad)
        {
            auto id = getGroupNameWithNoSpaces(group);
            auto dir = getProjectWriteableRoot().getChildFile(id);
            if (dir.isDirectory())
            {
                auto whatToLookFor = juce::File::TypesOfFileToFind::findFiles | juce::File::TypesOfFileToFind::ignoreHiddenFiles;
                auto arr = dir.findChildFiles(whatToLookFor, false, "*"+extension, juce::File::FollowSymlinks::noCycles);
                for (const auto& file : arr)
                {
                    auto item = juce::PopupMenu::Item();
                    item.action = [&, name = file.getFileNameWithoutExtension(), e = extension, al = afterLoad](){
                        loadPreset(group, name, e, al);
                    };
                    item.itemID = startId;
                    item.text = file.getFileNameWithoutExtension();
                    item.isTicked = false;
                    item.isEnabled = true;
                    m.addItem(item);

                    startId++;

                }

                for ( const auto& subDir : dir.findChildFiles(juce::File::TypesOfFileToFind::findDirectories, false, "*"+extension, juce::File::FollowSymlinks::noCycles))
                {
                    auto subMenu = juce::PopupMenu();
                    startId = populatePresetPopupMenu(startId, subMenu, group, extension, afterLoad);
                    m.addSubMenu(subDir.getFileName(), subMenu);
                }
            }
            return startId;
        }
    };
}