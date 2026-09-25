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
#include <sjf/helpers/sjf_AsyncCallbackInvoker.h>

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

			checkForPresetChange();
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

		checkForPresetChange();
	})
	{
		juce::MessageManager::callAsync([&, safeThis = SafePointer(this)]()
		{
			if (!safeThis)
				return;

			apvtsProvider = findParentComponentOfClass<helpers::PresetManager::APVTSProvider>();
			if (apvtsProvider)
			{
				apvts = apvtsProvider->getAPVTS().state;
				apvts.addListener(this);
				if (parameters.getID().isEmpty())
					return;
				if (auto child = apvtsProvider->getAPVTS().state.getChildWithName(parameters.getID()); child.isValid())
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

			checkForPresetChange();


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
		const static auto idID = juce::Identifier("id");
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
		else if (apvtsProvider)
		{
			auto forThisGroup = vt.getType().toString().startsWith(parameters.getID());
			forThisGroup = forThisGroup | (vt.hasProperty(idID) && vt.getProperty(idID).toString().startsWith(parameters.getID()));
			if (!forThisGroup)
				return;

			// ensure all params are properly updated before trying to trigger
			// also means we only update once per batch updat of parameters (e.g. changing preset)
			asyncUpdate.triggerUpdate();
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


	void checkForPresetChange()
	{
		if (!apvtsProvider)
			return;

		auto name = getText().upToLastOccurrenceOf("*", false, false);
		if (name.isEmpty())
			return;

		if (helpers::PresetManager::presetChanged(apvtsProvider->getAPVTS(), parameters, name))
			name += "*";

		if (getText() != name)
			setText(name);
	}

	const juce::AudioProcessorParameterGroup& parameters;
	const juce::String extension;
	UndoManager* undoManager = nullptr;
	const AfterSave afterSave = {};
	const AfterLoad afterLoad = {};
	ValueTree apvts, paramVT;
	helpers::PresetManager::APVTSProvider* apvtsProvider{nullptr};
	using Callback = std::function<void()>;
	helpers::AsyncCallbackInvoker<Callback> asyncUpdate{ [safeThis = SafePointer(this), this](){
		if (!safeThis) return;

		checkForPresetChange();
	}};
};
}


