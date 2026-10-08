/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 28/09/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/dsp/sjf_Modulation.h>
#include <sjf/widgets/sjf_GenericEditor.h>

namespace sjf::gui::modulation{
	namespace ids
	{
		const static auto modulatedID = juce::Identifier{"modulated"};
	}


/**
 * @brief Manages GUI component binding, context menus, and visual modulation indicators for modulatable parameters.
 *
 * `ModulationManager` recurses through a JUCE `AudioProcessorEditor` component hierarchy to discover controls
 * mapped to `Modulatable` parameters in an `AudioProcessorValueTreeState` (APVTS). It attaches mouse listeners
 * to handle right-click context menus, allowing users to dynamically add, remove, or adjust modulation connection
 * depths with full `UndoManager` support.
 *
 * It updates component properties (such as setting `sjf::gui::modulation::ids::modulatedID`) to allow look-and-feel
 * classes or custom components to visually draw modulation rings, arcs, or indicators.
 *
 * @tparam Modulators Variadic parameter pack representing all concrete modulator types managed by the underlying system.
 * @see ModulationSystem, GenericEditorWithModulation
 */
template<typename ...Modulators>
class ModulationManager : private juce::MouseListener, juce::ValueTree::Listener
{
public:
	using ModulationSystem = sjf::dsp::modulation::ModulationSystem<Modulators...>;
	using Modulatable = ModulationSystem::Modulatable;
	using Modulator = ModulationSystem::Modulator;
	using Connection = ModulationSystem::Connection;

    ModulationManager (juce::AudioProcessorEditor& editor,
                        juce::AudioProcessorValueTreeState& apvts,
                        ModulationSystem& modSystem,
                        UndoManager* undoManager_)
        : globalAPVTS (apvts), system (modSystem), undoManager (undoManager_)
    {
        // 1. Recurse through the entire editor component tree
        attachToComponentTree (&editor);
    	globalAPVTS.state.addListener(this);
    }

    ~ModulationManager() override
    {
        for (auto* comp : attachedComponents)
            comp->removeMouseListener (this);

    	globalAPVTS.state.removeListener(this);
    }

	ModulationSystem& getModulationSystem ()
    {
	    return system;
    }

private:
	void valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged,
									   const juce::Identifier&) override
	{
		if (treeWhosePropertyHasChanged == globalAPVTS.state)
		{
			if (juce::MessageManager::existsAndIsCurrentThread())
				checkModulationStateOfComponents();
			else
				asyncUpdater.triggerUpdate();
		}
	}

	void valueTreeRedirected(ValueTree& treeWhichHasBeenChanged) override
	{
		if (treeWhichHasBeenChanged == globalAPVTS.state)
		{
			if (juce::MessageManager::existsAndIsCurrentThread())
				checkModulationStateOfComponents();
			else
				asyncUpdater.triggerUpdate();
		}
	}

	void checkModulationStateOfComponents()
	{
		jassert(juce::MessageManager::existsAndIsCurrentThread());
		for (auto& [comp, targetInfo] : targetMap)
		{
			if (auto targetComp = componentMap[targetInfo.paramID])
			{
				auto wasModulated = static_cast<bool>(targetComp->getProperties().getWithDefault(ids::modulatedID, false));
				auto isModulated = system.isModulated (targetInfo.target);
				if (isModulated != wasModulated)
				{
					targetComp->getProperties().set(ids::modulatedID,isModulated);
					targetComp->repaint();
				}
			}
		}
	}

    void attachToComponentTree (juce::Component* parent)
    {
        if (parent == nullptr) return;

        // Check if component has a tagged parameter ID
        juce::String paramID = parent->getProperties().getWithDefault ("parameterID", parent->getComponentID());

        if (paramID.isNotEmpty())
        {
            if (auto* rawParam = globalAPVTS.getParameter (paramID))
            {
                if (auto* modTarget = dynamic_cast<Modulatable*> (rawParam); modTarget && modTarget->isModulatable())
                {
                    // Map this component to its parameter and target interface
                    targetMap[parent] = { paramID, modTarget };

                    // Attach mouse listener with recursive child capture enabled!
                    parent->addMouseListener (this, true);
                    attachedComponents.push_back (parent);

                	componentMap[paramID] = parent;
                	if (auto modCount = system.isModulated(modTarget))
                	{
                		parent->getProperties().set(ids::modulatedID, true);
                		parent->repaint();
                	}
                }
            }
        }

        // Recurse into children
        for (int i = 0; i < parent->getNumChildComponents(); ++i)
            attachToComponentTree (parent->getChildComponent (i));
    }

    // Intercept mouse events across Sliders, ComboBoxes, and Buttons
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu())
        {
            // Find which registered component triggered the event
            auto* targetComp = e.eventComponent;
            while (targetComp != nullptr && !targetMap.contains(targetComp))
                targetComp = targetComp->getParentComponent();

            if (targetComp != nullptr)
            {
                const auto& info = targetMap[targetComp];
                showModulationMenu (info.paramID);
            }
        }
    }

    void showModulationMenu (const juce::String& paramID)
    {
    	if (!system.apvts)
    		return;

    	const auto ranged = dynamic_cast<RangedAudioParameter*>(system.apvts->getParameter(paramID));
    	if (!ranged)
    		return;

        juce::PopupMenu menu;
        menu.addSectionHeader ("Modulation: " + ranged->name);

        auto activeSources = system.modulators.getIndexToId();
    	modControlPanels.clear();
        for (auto i = 0ul; i < activeSources.size(); ++i)
        {
            const auto& modId = activeSources[i];
        	if (paramID.startsWith(modId))
        		continue;

        	auto connection = system.getConnection(modId, paramID);
			if (connection.target && connection.source)
        	{
        		auto sub = PopupMenu{};
        		modControlPanels.push_back(std::make_unique<ModControlPanel>());
				auto depth = connection.depth;
				auto bipolar = connection.bipolar;
        		modControlPanels.back()->depthSlider.setValue( depth * 100.0f);
				modControlPanels.back()->depthSlider.onMouseUp = [this, safeThis = std::weak_ptr(guard), depth, modId, paramID, slider = Component::SafePointer(&modControlPanels.back()->depthSlider)](){
					if (safeThis.expired())
						return;

					if (slider && !approximatelyEqual(static_cast<float>(slider->getValue()) *0.01f, depth))
					{
						auto connection_ = system.getConnection(modId, paramID);

						if (undoManager)
						{
							undoManager->beginNewTransaction();
						}
						system.addConnection(modId, paramID, static_cast<float>(slider->getValue()) * 0.01f, connection_.bipolar, undoManager);
						if (undoManager)
						{
							undoManager->setCurrentTransactionName("Changed depth of modulation connection: " + modId + " ==> " + paramID + " to: " + slider->getTextFromValue(slider->getValue()));
							undoManager->beginNewTransaction();
						}
					}
				};
				modControlPanels.back()->remove.onClick = [this, safeThis = std::weak_ptr(guard), modId, paramID, safeComp = Component::SafePointer(modControlPanels.back().get())](){
					if (safeThis.expired())
						return;

					if (safeComp)
					{
						if (undoManager)
						{
							undoManager->beginNewTransaction();
						}
						system.removeConnection(modId, paramID, undoManager);


						if (undoManager)
						{
							undoManager->setCurrentTransactionName("Removed modulation connection: " + modId + " ==> " + paramID );
							undoManager->beginNewTransaction();
						}


						juce::PopupMenu::dismissAllActiveMenus();
					}
				};
				modControlPanels.back()->polarity.setButtonText(bipolar ? "Bipolar" : "Unipolar");
				modControlPanels.back()->polarity.onClick = [this, safeThis = std::weak_ptr(guard), modId, paramID, button = Component::SafePointer(&modControlPanels.back()->polarity)] ()
				{
					if (safeThis.expired())
						return;

					if (button)
					{
						auto connection_ = system.getConnection(modId, paramID);
						auto bipolar_ = !connection_.bipolar;

						if (undoManager)
						{
							undoManager->beginNewTransaction();
						}
						system.addConnection(modId, paramID, connection_.depth, bipolar_, undoManager);
						if (undoManager)
						{
							undoManager->setCurrentTransactionName("Changed polarity of modulation connection: " + modId + " ==> " + paramID + " to: " + (bipolar_ ? "Bipolar" : "Unipolar"));
							undoManager->beginNewTransaction();
						}
						button->setButtonText(bipolar_ ? "Bipolar" : "Unipolar");
					}
				};

        		sub.addCustomItem(1, *modControlPanels.back(), 400, 20+(30*3), false, nullptr, modId);
        		menu.addSubMenu(modId, sub);
        	}
        	else
        	{
        		menu.addItem (modId, true, false, [this, safeThis = std::weak_ptr(guard), modId, paramID](){
        			if (safeThis.expired() || system.isConnected(modId, paramID))
        				return;


        			if (undoManager)
        			{
        				undoManager->beginNewTransaction();
					}
					system.addConnection (modId, paramID, 1.0f, true, undoManager);

        			if (undoManager)
        			{
        				undoManager->setCurrentTransactionName("Added modulation connection: " + modId + " ==> " + paramID );
        				undoManager->beginNewTransaction();
        			}
				});
        	}

        }

        menu.showMenuAsync ({});
    }

	struct ModControlPanel : juce::Component
    {
    	ModControlPanel()
    	{
    		addAndMakeVisible(remove);
    		remove.setButtonText("Remove");

    		addAndMakeVisible(behindSlider);
    		behindSlider.setInterceptsMouseClicks(false, false);
    		addAndMakeVisible(depthSlider);
    		depthSlider.setRange(-100.0f , 100.0f, 0.01f);
    		depthSlider.setDoubleClickReturnValue(true, 0.0f);
    		depthSlider.setTextValueSuffix("%");
    		depthSlider.setColour(juce::Slider::backgroundColourId, findColour(juce::TextButton::buttonColourId));
    		depthSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::black.withAlpha(0.0f));

    		addAndMakeVisible(polarity);
    	}

    	void resized() override
    	{
    		remove.setBounds(5, 5, getWidth() - 10, 30);
    		polarity.setBounds(5, remove.getBottom() + 5, getWidth() - 10, 30);
    		behindSlider.setBounds(remove.getX(), polarity.getBottom() + 5, remove.getWidth(),  30);
    		depthSlider.setBounds(behindSlider.getX()+5, behindSlider.getY(), behindSlider.getWidth()-5, behindSlider.getHeight());
    	}

    	struct DepthSlider : public juce::Slider
    	{
    		DepthSlider()
    		: juce::Slider(juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextBoxRight)
    		{}

    		void mouseUp(const MouseEvent& e) override
    		{
    			juce::Slider::mouseUp(e);
    			if (onMouseUp)
					onMouseUp();
    		}
    		std::function<void()> onMouseUp;
    	};


    	juce::TextButton remove, polarity, behindSlider;
    	DepthSlider depthSlider;
    };

	std::vector<std::unique_ptr<ModControlPanel>> modControlPanels;

    struct TargetInfo { juce::String paramID; Modulatable* target; };
    std::unordered_map<juce::Component*, TargetInfo> targetMap;
    std::unordered_map<juce::String, juce::Component::SafePointer<juce::Component>> componentMap;
    std::vector<juce::Component*> attachedComponents;
	ModulationSystem::Connections activeModulations;

    juce::AudioProcessorValueTreeState& globalAPVTS;
    ModulationSystem& system;
	UndoManager* undoManager{nullptr};
	std::shared_ptr<int> guard = std::make_shared<int>(42);
	using Callback = std::function<void()>;
	sjf::helpers::AsyncCallbackInvoker<Callback> asyncUpdater{[this, safeThis = std::weak_ptr(guard)](){
		if (!safeThis.expired())
			checkModulationStateOfComponents();
	}};
};


/**
 * @brief Extension of `GenericEditor` providing automated modulation GUI binding and preset state handling.
 *
 * `GenericEditorWithModulation` wraps `GenericEditor` and initializes an internal `ModulationManager`.
 * It automatically injects modulation connection state properties into the preset `ValueTree` during serialization
 * and restores active connection maps upon preset loading.
 *
 * @tparam Modulators Variadic parameter pack representing all concrete modulator types managed by the system.
 * @see ModulationManager, sjf::generic_editor::GenericEditor
 */
template<typename ...Modulators>
struct GenericEditorWithModulation : sjf::generic_editor::GenericEditor
{

	GenericEditorWithModulation(juce::AudioProcessorValueTreeState& apvts_, juce::AudioProcessor& processor_,
								 const helpers::ParameterFactory::GroupMetadata& metadata_,
								 dsp::modulation::ModulationSystem<Modulators...>& modSystem_,
								 UndoManager* undoManager_)
	: GenericEditor(apvts_, processor_, metadata_, undoManager_,
	[this](ValueTree vt){
		const auto& connections = modulationManager.getModulationSystem().getCurrentConnectionsAsValueTree();
		auto newVT = ValueTree{connections.getType()};
		newVT.copyPropertiesAndChildrenFrom(connections, nullptr);
		vt.addChild(newVT, -1, nullptr);
		},
[this](ValueTree vt){
		auto connections = vt.getChildWithName(modulationManager.getModulationSystem().getFactoryID() + sjf::dsp::modulation::ids::modulationConnectionsID);
		auto edit = modulationManager.getModulationSystem().getCurrentConnectionsAsValueTree();
		edit.copyPropertiesAndChildrenFrom(connections, nullptr);
	})
	, modulationManager(*this, apvts_, modSystem_, undoManager_)
	{}


	ModulationManager<Modulators...> modulationManager;
};
}


