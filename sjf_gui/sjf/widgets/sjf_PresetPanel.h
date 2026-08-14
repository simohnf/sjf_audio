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

	PresetPanel(const juce::AudioProcessorParameterGroup& parameters_,
				const juce::String& extension_ = helpers::PresetManager::getDefaultExtension(),
				const AfterSave& afterSave_ = {},
				const AfterLoad& afterLoad_ = {},
				UndoManager* undoManager_ = nullptr)
	: parameters(parameters_)
	, extension(extension_.startsWith(".") ? extension_ : "."+extension_)
	, undoManager(undoManager_)
	, afterSave([this, afterSave_](ValueTree vt){
		if (afterSave_)
			afterSave_(vt);

		if (auto prop = vt.getPropertyPointer(sjf::helpers::preset_manager::ids::presetNameId))
		{
			auto str = prop->toString();
			if (!str.isEmpty())
				setText(str, dontSendNotification);
		}
	})
	, afterLoad([this, afterLoad_](ValueTree vt){
		if (afterLoad_)
			afterLoad_(vt);


		if (undoManager)
		{
			undoManager->setCurrentTransactionName("Loaded preset for " + helpers::ParameterFactory::getNameWithoutParentPrefix(parameters));

			struct UpdatePresetNameAction : juce::UndoableAction
			{
				UpdatePresetNameAction(SafePointer<PresetPanel> parent, const juce::String& nameBefore_, const juce::String& nameAfter_)
				: panel(parent), nameBefore(nameBefore_), nameAfter(nameAfter_)
				{}

				bool perform () override
				{
					if (panel)
						panel->setText(nameAfter);
					return true;
				}

				bool undo() override
				{
					if (panel)
						panel->setText(nameBefore);
					return true;
				}

				SafePointer<PresetPanel> panel;

				const juce::String nameBefore, nameAfter;
			};

			if (auto prop = vt.getPropertyPointer(sjf::helpers::preset_manager::ids::presetNameId))
			{
				auto str = prop->toString();
				if (!str.isEmpty())
					undoManager->perform( new UpdatePresetNameAction(this, text, str));
			}


			undoManager->beginNewTransaction();

			text = getText();
		}
		else if (auto prop = vt.getPropertyPointer(sjf::helpers::preset_manager::ids::presetNameId))
		{
			auto str = prop->toString();
			if (!str.isEmpty())
				setText(str, dontSendNotification);
		}
	})
	{}


	void showPopup() override
	{
		if (const auto m = getRootMenu())
			*m = sjf::helpers::PresetManager::getPresetPopupMenu(parameters, extension, SafePointer<ComboBox>(this), afterSave, afterLoad);
		ComboBox::showPopup();
	}

private:

	const juce::AudioProcessorParameterGroup& parameters;
	const juce::String extension;
	juce::String text;
	UndoManager* undoManager = nullptr;
	const AfterSave afterSave = {};
	const AfterLoad afterLoad = {};
};
}


