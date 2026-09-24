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
class PresetPanel : public juce::ComboBox, public ValueTree::Listener
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

		if (apvts.isValid())
		{
			if (paramVT.isValid())
				paramVT.setProperty(helpers::preset_manager::ids::presetNameId, vt.getProperty(helpers::preset_manager::ids::presetNameId).toString(), nullptr);
			else
				apvts.setProperty(helpers::preset_manager::ids::presetNameId, vt.getProperty(helpers::preset_manager::ids::presetNameId).toString(), nullptr);


			ValueTreeRecurser::afterSave(vt, apvts, parameters, nullptr);

		}
	})
	, afterLoad([this, afterLoad_](ValueTree vt){
		if (afterLoad_)
			afterLoad_(vt);


		if (apvts.isValid())
			ValueTreeRecurser::afterLoad(vt, apvts, parameters, undoManager);

		if (undoManager)
		{
			auto name_ = vt.getPropertyPointer(helpers::preset_manager::ids::presetNameId);
			auto name = name_ ? name_->toString() : "";
			auto paramsName =  helpers::ParameterFactory::getNameWithoutParentPrefix(parameters);
			paramsName = paramsName.isEmpty() ? juce::String(JucePlugin_Name) : paramsName;
			undoManager->setCurrentTransactionName("Load \"" + name + "\" preset for " + paramsName);
			// undoManager->beginNewTransaction();
		}
	})
	{
		juce::MessageManager::callAsync([&, safeThis = SafePointer(this)]()
		{
			if (!safeThis)
				return;
			if (auto apvts_ = findParentComponentOfClass<helpers::PresetManager::APVTSProvider>())
			{
				apvts = apvts_->getAPVTS().state;
				apvts.addListener(this);
				if (parameters.getID().isEmpty())
					return;
				if (auto child = apvts_->getAPVTS().state.getChildWithName(parameters.getID()); child.isValid())
				{
					child.addListener(this);
					paramVT = child;
					if (const auto prop = child.getPropertyPointer(helpers::preset_manager::ids::presetNameId))
						setText(prop->toString());
					else
						jassertfalse;
				}
				else
				{
					jassertfalse;
				}
			}
		});
	}

	~PresetPanel() override
	{
		if (apvts.isValid())
			apvts.removeListener(this);
		if (paramVT.isValid())
			paramVT.removeListener(this);
	}


	void showPopup() override
	{
		if (undoManager)
			undoManager->beginNewTransaction();
		if (const auto m = getRootMenu())
			*m = sjf::helpers::PresetManager::getPresetPopupMenu(parameters, extension, SafePointer<ComboBox>(this), afterSave, afterLoad);
		ComboBox::showPopup();
	}

private:
	struct ValueTreeRecurser
	{
		void static afterLoad(const ValueTree& presetTree,  ValueTree& apvts_, const juce::AudioProcessorParameterGroup& parameters_, UndoManager* undoManager_)
		{
			auto name_ = presetTree.getPropertyPointer(helpers::preset_manager::ids::presetNameId);
			auto name = name_ ? name_->toString() : "";
			if (parameters_.getID().isEmpty())
			{
				apvts_.setProperty(helpers::preset_manager::ids::presetNameId, name, undoManager_);
			}
			else
			{
				if (auto child = apvts_.getChildWithName(parameters_.getID()); child.isValid())
				{
					child.setProperty(helpers::preset_manager::ids::presetNameId, name, undoManager_);
				}
				else
				{
					jassertfalse;
				}
			}
			for (auto& child : parameters_.getSubgroups(false))
			{
				auto childId = helpers::ParameterFactory::getIDWithoutParentPrefix(*child);
				if (auto childVT = presetTree.getChildWithName(childId); childVT.isValid())
					afterLoad(childVT, apvts_, *child, undoManager_);
				else
					jassertfalse;
			}
		}

		void static afterSave(ValueTree& presetTree, const ValueTree& apvts_, const juce::AudioProcessorParameterGroup& parameters_, UndoManager* undoManager_)
		{
			const auto name = [&]() -> const var* {
				if (parameters_.getID().isEmpty())
				{
					return apvts_.getPropertyPointer( helpers::preset_manager::ids::presetNameId);
				}
				else
				{
					if (const auto child = apvts_.getChildWithName(parameters_.getID()); child.isValid())
						return child.getPropertyPointer( helpers::preset_manager::ids::presetNameId);
					jassertfalse;
				}
				return nullptr;
			}();

			presetTree.setProperty(helpers::preset_manager::ids::presetNameId, name ? name->toString() : "", undoManager_);
			for (auto& child : parameters_.getSubgroups(false))
			{
				auto childId = helpers::ParameterFactory::getIDWithoutParentPrefix(*child);
				if (auto childVT = presetTree.getChildWithName(childId); childVT.isValid())
					afterSave(childVT, apvts_, *child, undoManager_);
				else
					jassertfalse;
			}
		}
	};

	void valueTreePropertyChanged (ValueTree& vt, const Identifier& property) override
	{
		if (vt == paramVT)
		{
			if (property == helpers::preset_manager::ids::presetNameId)
			{
				setText(vt.getProperty(property).toString());
			}
		}
		else if (parameters.getID().isEmpty() && vt == apvts && property == helpers::preset_manager::ids::presetNameId)
		{
			setText(vt.getProperty(property).toString());
		}
	}

	void valueTreeRedirected(ValueTree& treeWhichHasBeenChanged) override
	{
		if (apvts.isValid())
		{
			if (apvts == treeWhichHasBeenChanged)
			{
				if (const auto newVT = apvts.getChildWithName(parameters.getID()); newVT.isValid())
				{
					if (newVT != paramVT)
					{
						paramVT = newVT;
						paramVT.addListener(this);
					}
				}
				else if (paramVT.isValid())
				{
					jassertfalse;
					paramVT.removeListener(this);
				}
			}
		}
		else
		{
			jassertfalse;
		}
	}

	const juce::AudioProcessorParameterGroup& parameters;
	const juce::String extension;
	UndoManager* undoManager = nullptr;
	const AfterSave afterSave = {};
	const AfterLoad afterLoad = {};
	ValueTree apvts, paramVT;
};
}


