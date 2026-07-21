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
#include "sjf/helpers/sjf_PresetManager.h"

namespace sjf::gui
{
class PresetPanel : public juce::ComboBox
{
public:
	PresetPanel(const AudioProcessorParameterGroup& parameters_, const String& extension_ = ".sjf")
	: parameters(parameters_)
	, extension(extension_.startsWith(".") ? extension_ : "."+extension_)
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
			*m = sjf::helpers::PresetManager::getPresetPopupMenu(parameters, extension, SafePointer<ComboBox>(this));
			if (const auto saved = getProperties().getWithDefault(helpers::preset_manager::ids::savedPreset, {}); !saved.isVoid())
			{
				setText(saved.toString());
				getProperties().remove(helpers::preset_manager::ids::savedPreset);
			}
		}
	}

	const AudioProcessorParameterGroup& parameters;
	const String extension;
	String text;
	VBlankAttachment vBlankAttachment{this, [&](){timerCallback();}};
};
}


