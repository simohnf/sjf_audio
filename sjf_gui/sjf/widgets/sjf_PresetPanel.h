/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 21/07/2026.
//

#pragma once
#include <sjf/helpers/sjf_PresetManager.h>

namespace sjf::gui
{
class PresetPanel : public juce::ComboBox
{
public:
	using AfterSave = helpers::PresetManager::AfterSaveCallback;
	using AfterLoad = helpers::PresetManager::AfterLoadCallback;



	PresetPanel(const juce::AudioProcessorParameterGroup& parameters_, const juce::String& extension_ = helpers::PresetManager::getDefaultExtension(), const AfterSave& afterSave_ = {}, const AfterLoad& afterLoad_ = {})
	: parameters(parameters_)
	, extension(extension_.startsWith(".") ? extension_ : "."+extension_)
	, afterSave(afterSave_)
	, afterLoad(afterLoad_)
	{}

private:
	void timerCallback()
	{
		if (getText() == helpers::preset_manager::strings::savePreset)
			setText(text);
		else
			text = getText();
		
		if (const auto m = getRootMenu())
		{
			*m = sjf::helpers::PresetManager::getPresetPopupMenu(parameters, extension, SafePointer<ComboBox>(this), afterSave, afterLoad);
			if (const auto saved = getProperties().getWithDefault(helpers::preset_manager::ids::savedPreset, {}); !saved.isVoid())
			{
				setText(saved.toString());
				getProperties().remove(helpers::preset_manager::ids::savedPreset);
			}
		}
	}

	const juce::AudioProcessorParameterGroup& parameters;
	const juce::String extension;
	juce::String text;
	const AfterSave afterSave = {};
	const AfterLoad afterLoad = {};
	juce::VBlankAttachment vBlankAttachment{this, [&](){timerCallback();}};
};
}


