//
// Created by Simon Fay on 06/08/2026.
//

#include <JuceHeader.h>
#include "sjf_GenericEditor.h"

#include "sjf/helpers/sjf_DynamicProcessorSequence.h"

namespace sjf::generic_editor
{
	namespace
	{
		class DeviceSelectorEditor : public AutoEditor
		{
		public:
			DeviceSelectorEditor(juce::AudioProcessorValueTreeState& apvts_,
					   const juce::AudioProcessorParameterGroup& group_,
					   const helpers::ParameterFactory::GroupMetadata& metadata_)
			: AutoEditor(apvts_, group_, metadata_)
			{

			}

			void resized() override
			{
				AutoEditor::resized();
				for (auto i = 0ul; i < childEditors.size(); ++i)
					childEditors[i]->setBounds(childEditors[i]->getBounds().withY(childEditors[0]->getY()));


				const auto selectorComboBox = dynamic_cast<juce::ComboBox*>(paramMap[dynamic_cast<const juce::AudioProcessorParameter*>(metadata.selectorParameter)]);
				jassert(selectorComboBox);
				if (!selectorComboBox->onChange)
				{
					selectorComboBox->onChange = [&](){
						auto selected = juce::jmin(static_cast<size_t>(metadata.selectorParameter->getIndex()), childEditors.size() - 1);
						for (auto i = 0ul; i < childEditors.size(); i++)
						{
							childEditors[i]->setVisible(i == selected && expanded);
							onLayoutChanged();
						}
						childEditors[selected]->setExpanded(true);
					};
					selectorComboBox->onChange();
				}
			}

			juce::Rectangle<int> getRequiredSize() const override
			{
				// will need additional logic if changing from single column...
				auto heightOfChildren = [&](){
					const auto selected = juce::jmin(static_cast<size_t>(metadata.selectorParameter->getIndex()), childEditors.size() - 1);
					return childEditors[selected]->getRequiredSize().getHeight() + VerticalSpacing;
				};


				auto w = getWidth();
				auto h = (!expanded ? 0 : static_cast<int>(sliders.size() + comboBoxes.size() + buttons.size()) * (ComponentHeight + VerticalSpacing))
									+ VerticalSpacing // extra spacing at bottom
									+ (presetPanel ? PresetPanelHeight + VerticalSpacing : 0)
									+ juce::jmax(titleLabel.getHeight(), collapseButton.getHeight()) + VerticalSpacing
									+ (expanded ? heightOfChildren() : 0);
				return {w, h};
			}

			void setExpanded(const bool shouldBeExpanded) override
			{
				expanded = shouldBeExpanded;
				for (const auto c : paramComponents)
					c->setVisible(expanded);

				for (const auto& l : paramNames)
					l->setVisible(expanded);

				jassert(childEditors.size() == static_cast<size_t>(metadata.selectorParameter->choices.size()));
				auto selected = static_cast<size_t>(metadata.selectorParameter->getIndex());
				for (auto i = 0ul; i < childEditors.size(); i++)
				{
					childEditors[i]->setVisible(i == selected && expanded);
				}

				onLayoutChanged();
			}
			JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DeviceSelectorEditor)
		};



		class SequenceItemComponent : public juce::Component
		{
		public:
			struct Listener
			{
				virtual ~Listener() = default;
				virtual void onItemClicked(SequenceItemComponent* item) = 0;
				virtual void onItemRemoveRequested(size_t processorID_) = 0;
				virtual void onItemSwapRequested(size_t processorID_, size_t newProcessorID_) = 0;
				virtual std::vector<std::pair<size_t, juce::String>> getAvailableSwapTypes() = 0;
			};

			SequenceItemComponent(juce::AudioProcessorValueTreeState&,
								  const juce::AudioProcessorParameterGroup& group_,
								  const helpers::ParameterFactory::GroupMetadata& metadata_,
								  const size_t processorID_,
								  Listener& listener_)
			: group(group_)
			, metadata(metadata_)
			, processorID(processorID_)
			, listener(listener_)
			{
				label.setText(sjf::helpers::ParameterFactory::getNameWithoutParentPrefix(group), dontSendNotification);
				label.setJustificationType(juce::Justification::centred);
				addAndMakeVisible(label);
				label.setInterceptsMouseClicks(false, false);

				setMouseCursor(MouseCursor::UpDownResizeCursor);
			}

			void setSelected(const bool shouldBeSelected)
			{
				isSelected = shouldBeSelected;
				repaint();
			}

			size_t getProcessorID() const noexcept { return processorID; }
			const juce::AudioProcessorParameterGroup& getGroup() const noexcept { return group; }

			void paint(Graphics& g) override
			{
				g.setColour(colours::borderColour);
				g.drawRoundedRectangle(getLocalBounds().toFloat(), 2, 1.0f);
			}
			void mouseDown(const juce::MouseEvent& e) override
			{
				// Select on click
				listener.onItemClicked(this);

				if (e.mods.isPopupMenu())
				{
					showContextMenu();
				}
			}

			void mouseDrag(const juce::MouseEvent& e) override
			{
				if (e.mods.isPopupMenu())
					return;

				// Only start drag if dragging past threshold and initiated over the grip/row
				if (e.getDistanceFromDragStart() > 5)
				{
					if (auto* dragContainer = juce::DragAndDropContainer::findParentDragContainerFor(this))
					{
						// Description passed to DragAndDropTarget identifies the source index
						const juce::var dragData(static_cast<int>(processorID));

						// Create snapshot image for dragging cursor
						const auto snapshot = createComponentSnapshot(this->getLocalBounds());

						dragContainer->startDragging(dragData, this, ScaledImage{snapshot});
					}
				}
			}

			void resized() override
			{
				label.setBounds(getLocalBounds().reduced(AutoEditor::HorizontalSpacing, AutoEditor::VerticalSpacing));
			}
		private:
			void showContextMenu()
			{
				juce::PopupMenu menu;

				// 1. Swap Submenu
				juce::PopupMenu swapMenu;
				const auto availableTypes = listener.getAvailableSwapTypes();

				if (availableTypes.empty())
				{
					swapMenu.addItem(juce::PopupMenu::Item("No Inactive Processors").setTicked(false).setEnabled(false));
				}
				else
				{
					for (const auto& [typeId, name] : availableTypes)
					{
						swapMenu.addItem(name, true, false, [this, id = typeId, safeThis = SafePointer(this)]() {
							if (!safeThis) return;
							listener.onItemSwapRequested(processorID, id);
						});
					}
				}

				menu.addSubMenu("Swap With", swapMenu, !availableTypes.empty());
				menu.addSeparator();

				// 2. Remove / Deactivate Option
				menu.addItem("Remove", [this, safeThis = SafePointer(this)]() {
					if (!safeThis) return;
					listener.onItemRemoveRequested(processorID);
				});

				menu.showMenuAsync(juce::PopupMenu::Options{});
			}

			const juce::AudioProcessorParameterGroup& group;
			const helpers::ParameterFactory::GroupMetadata metadata;
			const size_t processorID{ 0 };
			bool isSelected{ false };
			juce::Label label;

			Listener& listener;
			JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequenceItemComponent)

		};



		class SequenceListView :	public juce::Component,
									public juce::DragAndDropTarget,
									public SequenceItemComponent::Listener,
									public juce::ValueTree::Listener

		{
		public:
			struct Listener
			{
				virtual ~Listener() = default;

				virtual void onItemSelectionChanged(size_t processorID) = 0;
			};

			SequenceListView(juce::AudioProcessorValueTreeState& apvts_,
							 const juce::AudioProcessorParameterGroup& group_,
							 const helpers::ParameterFactory::GroupMetadata& metadata_,
							 Listener& listener_)
			: listener(listener_)
			, apvts(apvts_), group(group_), metadata(metadata_), state(apvts_.state)
			, sequenceID(group.getID() + sjf::helpers::dynamic_processor_sequence::ids::sequenceTreeId)
			{
				const auto& subgroups = group.getSubgroups(false);
				masterPool.reserve(static_cast<size_t>(subgroups.size()));

				for (auto i = 0ul; i < jmin(static_cast<size_t>(subgroups.size()), metadata.numProcessorsInDynamicSequence); ++i)
				{
					if (auto* subgroup = subgroups[static_cast<int>(i)])
					{
						// Construct the item directly into the master pool
						auto item = std::make_unique<SequenceItemComponent>(apvts, *subgroup, metadata.children[i], i, *this);

						addAndMakeVisible(*item);
						item->setVisible(false); // Initially hidden until setActiveSequence is called

						masterPool.push_back(std::move(item));
					}
				}

				activeStates.resize(metadata.numProcessorsInDynamicSequence, false);

				// Configure and reveal the Add Processor button
				addButton.onClick = [this]() { showAddProcessorMenu(); };
				addAndMakeVisible(addButton);

				if (state.isValid())
				{
					state.addListener(this);
					valueTreeUpdated();
				}
			}

			~SequenceListView() override
			{
				if (state.isValid())
					state.removeListener(this);
			}

			// Sets active order (updates item visibility & layout bounds)
			void setActiveSequence(const std::vector<size_t>& activeProcessorIDs)
			{
				activeSequence.clear();
				std::fill(activeStates.begin(), activeStates.end(), false);

				for (const auto & comp : masterPool)
					comp->setVisible(false);

				for (const auto activeProcessorID : activeProcessorIDs)
				{
					if (activeProcessorID < masterPool.size())
					{
						activeSequence.push_back(masterPool[activeProcessorID].get());
						activeSequence.back()->setVisible(true);
						activeStates[activeProcessorID] = true;
					}
					else
					{
						break;
					}
				}

				const auto viewport = dynamic_cast<Viewport*>(getParentComponent());
				const auto viewPos = viewport ? viewport->getViewPosition() : juce::Point<int>(0, 0);
				setSize(getWidth(), getCalculatedHeight());
				if (viewport)
					viewport->setViewPosition(viewPos);

				resized();
			}

			[[nodiscard]] int getCalculatedHeight() const noexcept
			{
				constexpr int itemHeight = AutoEditor::ComponentHeight; // or a fixed row height, e.g., 30
				constexpr int spacing    = AutoEditor::VerticalSpacing;  // e.g., 4
				const int numItems   = static_cast<int>(activeSequence.size());

				// Height of all active items + addButton + spacing after each element + top/bottom padding
				const int totalItemRows = numItems + 1; // active items + 1 row for addButton
				return totalItemRows * itemHeight + (totalItemRows + 1) * spacing;
			}

			void resized() override
			{
				const auto x = AutoEditor::HorizontalSpacing;
				const auto w = getWidth() - AutoEditor::HorizontalSpacing*2;
				auto y = AutoEditor::VerticalSpacing;
				for (const auto & item : activeSequence)
				{
					item->setBounds(x, y, w, AutoEditor::ComponentHeight);
					y += AutoEditor::ComponentHeight + AutoEditor::VerticalSpacing;
				}
				addButton.setBounds(x, y, w, AutoEditor::ComponentHeight);

				if (!activeSequence.empty() && selectedId > masterPool.size())
				{
					// just make sure we open the first editor to begin with
					MessageManager::callAsync([&, safeThis = SafePointer(this)](){
						if (safeThis)
							onItemClicked(activeSequence[0]);
					});
				}
			}

			void paintOverChildren(juce::Graphics& g) override
			{
				if (!insertionIndex.has_value())
				{
					g.setColour(colours::borderColour);
					g.drawRect(getLocalBounds());

					return;
				}

				const auto index = *insertionIndex;

				int lineY = 0;

				if (index < activeSequence.size())
				{
					lineY = activeSequence[index]->getY() - (AutoEditor::VerticalSpacing / 2);
				}
				else if (!activeSequence.empty())
				{
					lineY = activeSequence.back()->getBottom() + (AutoEditor::VerticalSpacing / 2);
				}
				else
				{
					lineY = addButton.getY() - (AutoEditor::VerticalSpacing / 2);
				}

				g.setColour(colours::insertionIndexColour);
				g.fillRect(0, jmax(0, lineY - 1), getWidth(), 3);

				g.setColour(colours::borderColour);
				g.drawRect(getLocalBounds());

			}

			// DragAndDropTarget Overrides
			void itemDragEnter(const SourceDetails& dragSourceDetails) override
			{
				if (dynamic_cast<SequenceItemComponent*>(dragSourceDetails.sourceComponent.get()))
					itemDragMove(dragSourceDetails);
			}

			void itemDragMove(const SourceDetails& dragSourceDetails) override
			{
				if (dynamic_cast<SequenceItemComponent*>(dragSourceDetails.sourceComponent.get()))
				{
					const auto newInsertionIndex = calculateDropIndex(dragSourceDetails.localPosition.y);

					if (insertionIndex != newInsertionIndex)
					{
						insertionIndex = newInsertionIndex;
						repaint();
					}
				}
			}

			void itemDragExit(const SourceDetails& dragSourceDetails) override
			{
				if (dynamic_cast<SequenceItemComponent*>(dragSourceDetails.sourceComponent.get()))
				{
					insertionIndex.reset();
					repaint();
				}
			}

			void itemDropped(const SourceDetails& dragSourceDetails) override
			{
				if (const auto* item = dynamic_cast<SequenceItemComponent*>(dragSourceDetails.sourceComponent.get()))
				{
					const auto targetIndex = insertionIndex.value_or(calculateDropIndex(dragSourceDetails.localPosition.y));
					insertionIndex.reset();
					repaint();

					onItemMoveRequested(item->getProcessorID(), targetIndex);
				}
			}

			bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override
			{
				return dynamic_cast<SequenceItemComponent*>(dragSourceDetails.sourceComponent.get()) != nullptr;
			}

		private:
			void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged,
											   const Identifier& property) override
			{
				if (treeWhosePropertyHasChanged.hasType(sequenceID) &&
					property == sjf::helpers::dynamic_processor_sequence::ids::sequencePropertyId)
				{
					if (MessageManager::existsAndIsCurrentThread())
					{
						valueTreeUpdated();
					}
					else
					{
						asyncUpdater.triggerUpdate();
					}
				}
			}

			void valueTreeRedirected(ValueTree& treeWhichHasBeenChanged) override
			{
				if (treeWhichHasBeenChanged == state)
				{
					state.removeListener(this);
					state = apvts.state;
					if (state.isValid())
					{
						state.addListener(this);
						valueTreeUpdated();
					}

				}

			}

			void valueTreeUpdated()
			{
				auto xml = state.toXmlString();

				if (const auto vt = state.getChildWithName(sequenceID); vt.isValid())
				{
					if (const auto propPointer = vt.getPropertyPointer(sjf::helpers::dynamic_processor_sequence::ids::sequencePropertyId))
					{
						if (propPointer->isString())
						{
							const auto seq_ = juce::StringArray::fromTokens(propPointer->toString(), "/", "");
							std::vector<size_t> updatedSequence;
							updatedSequence.reserve(masterPool.size());
							for (const auto& i : seq_)
								updatedSequence.push_back(static_cast<size_t>(i.getIntValue()));
							setActiveSequence(updatedSequence);
						}
					}
				}

				auto selectedStillActive = false;
				for (auto item : activeSequence)
				{
					if (item->getProcessorID() == selectedId)
					{
						selectedStillActive = true;
						break;
					}
				}

				if (!selectedStillActive)
					onItemClicked(activeSequence.empty() ? nullptr : activeSequence[0]);

				addButton.setEnabled(activeSequence.size() < masterPool.size());
			}

			void updateValueTree(const std::vector<size_t>& updatedSequence) const
			{
				auto ret = juce::StringArray{};
				for (const auto i : updatedSequence)
					ret.add(static_cast<juce::String>(i));

				auto xml = state.toXmlString();
				if (auto seq = state.getChildWithName(sequenceID); seq.isValid())
				{
					seq.setProperty(sjf::helpers::dynamic_processor_sequence::ids::sequencePropertyId, ret.joinIntoString("/"), nullptr);
				}
			}

			// SequenceItemComponent::Listener Overrides
			void onItemClicked(SequenceItemComponent* item) override
			{
				if (item != nullptr)
					selectedId = item->getProcessorID();
				else
					selectedId = sjf::helpers::dynamic_processor_sequence::InactiveSlot;

				listener.onItemSelectionChanged(selectedId);
			}

			void onItemRemoveRequested(const size_t processorID) override
			{
				std::vector<size_t> updatedSequence{};
				updatedSequence.reserve(masterPool.size());
				for (const auto & i : activeSequence)
				{
					if (i->getProcessorID() != processorID)
						updatedSequence.push_back(i->getProcessorID());
				}

				updateValueTree(updatedSequence);
			}

			void onItemSwapRequested(const size_t targetProcessorID, const size_t newProcessorTypeID) override
			{
				std::vector<size_t> updatedSequence{};
				updatedSequence.reserve(masterPool.size());
				for (const auto & i : activeSequence)
				{
					if (i->getProcessorID() == targetProcessorID)
						updatedSequence.push_back(newProcessorTypeID);
					else
						updatedSequence.push_back(i->getProcessorID());
				}
				onItemClicked(masterPool[newProcessorTypeID].get());
				updateValueTree(updatedSequence);
			}


			void onItemAddRequested(const size_t newProcessorTypeID)
			{
				std::vector<size_t> updatedSequence{};
				updatedSequence.reserve(masterPool.size());
				for (const auto & i : activeSequence)
					updatedSequence.push_back(i->getProcessorID());
				updatedSequence.push_back(newProcessorTypeID);
				onItemClicked(masterPool[newProcessorTypeID].get());
				updateValueTree(updatedSequence);
			}

			void onItemMoveRequested(const size_t processorID, const size_t newIndex) const
			{
				std::vector<size_t> updatedSequence{};
				updatedSequence.reserve(masterPool.size());
				for (auto i = 0ul; i < activeSequence.size(); ++i)
					if (activeSequence[i]->getProcessorID() != processorID)
						updatedSequence.push_back(activeSequence[i]->getProcessorID());

				const auto targetPos = static_cast<std::vector<size_t>::difference_type>(juce::jmin(updatedSequence.size(), newIndex));
				updatedSequence.insert(updatedSequence.begin() + targetPos, processorID);

				updateValueTree(updatedSequence);
			}

			std::vector<std::pair<size_t, juce::String>> getAvailableSwapTypes() override
			{
				const auto& subgroups = group.getSubgroups(false);
				jassert(masterPool.size() == activeStates.size());
				jassert(masterPool.size() <= static_cast<size_t>(subgroups.size()));

				std::vector<std::pair<size_t, juce::String>> ret{};

				ret.reserve(masterPool.size());

				for ( auto i = 0ul; i < masterPool.size(); ++i)
					if (!activeStates[i])
						ret.emplace_back(i, sjf::helpers::ParameterFactory::getNameWithoutParentPrefix(*subgroups[static_cast<int>(i)]));

				return ret;
			}

			// Internal Helpers
			[[nodiscard]] size_t calculateDropIndex(const int dropY) const noexcept
			{
				if (activeSequence.empty())
					return 0;

				for (size_t i = 0; i < activeSequence.size(); ++i)
				{
					const auto itemBounds = activeSequence[i]->getBounds();
					const auto itemMidY   = itemBounds.getY() + (itemBounds.getHeight() / 2);

					if (dropY < itemMidY)
						return i;
				}

				return activeSequence.size();
			}


			void showAddProcessorMenu()
			{
				const auto subGroups = group.getSubgroups(false);
				jassert(subGroups.size() >= static_cast<int>(masterPool.size()));
				PopupMenu menu;
				const auto availableSwapTypes = getAvailableSwapTypes();
				for (const auto& [typeID, name] : availableSwapTypes)
				{
					menu.addItem(name, [&, typeID, safeThis = SafePointer(this)](){
						if (!safeThis)
							return;

						if (juce::MessageManager::existsAndIsCurrentThread())
						{
							onItemAddRequested(typeID);
						}
						else
						{
							juce::MessageManager::callAsync([this, typeID, safeThis]() {
								if (safeThis)
									onItemAddRequested(typeID);
							});
						}
					});
				}
				menu.showMenuAsync({});
			}

			Listener& listener;

			// OWNERSHIP: Built once directly in constructor by iterating group.getSubgroups()
			std::vector<std::unique_ptr<SequenceItemComponent>> masterPool;

			// VIEW: Active items currently visible in the sequence
			std::vector<SequenceItemComponent*> activeSequence;
			std::vector<bool> activeStates;

			juce::TextButton addButton{ "Add Processor" };
			std::optional<size_t> insertionIndex;
			juce::AudioProcessorValueTreeState& apvts;
			const juce::AudioProcessorParameterGroup& group;
			const helpers::ParameterFactory::GroupMetadata& metadata;

			ValueTree state;

			using Callback = std::function<void()>;
			sjf::helpers::AsyncCallbackInvoker<Callback> asyncUpdater{[this](){
				valueTreeUpdated();
			}};

			size_t selectedId = sjf::helpers::dynamic_processor_sequence::InactiveSlot;

			const Identifier sequenceID;

			JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequenceListView)
		};


		class DynamicProcessorSequenceEditor :	public AutoEditor,
												public SequenceListView::Listener,
												public juce::DragAndDropContainer
		{
		public:
			DynamicProcessorSequenceEditor(juce::AudioProcessorValueTreeState& apvts_,
					   const juce::AudioProcessorParameterGroup& group_,
					   const helpers::ParameterFactory::GroupMetadata& metadata_)
			: AutoEditor(apvts_, group_, metadata_)
			, sequenceListView(apvts, parameterGroup, metadata, *this)
			{
				viewport.setViewedComponent(&sequenceListView, false);
				addAndMakeVisible(viewport);
			}

			void resized() override
			{
				AutoEditor::resized();
				if (childEditors.empty())
					return;

				if (childSequenceEditors.size() + extraChildEditors.size() < childEditors.size())
				{
					for (auto i = 0ul; i < jmin(metadata.numProcessorsInDynamicSequence, childEditors.size()); i++)
						childSequenceEditors.push_back(childEditors[i].get());
					for (auto i = metadata.numProcessorsInDynamicSequence; i < childEditors.size(); i++)
						extraChildEditors.push_back(childEditors[i].get());
				}


				auto y = childEditors[0]->getBounds().getY();
				for (auto ce : extraChildEditors)
				{
					ce->setVisible(true);
					ce->setBounds(ce->getBounds().withY(y).withHeight(ce->getRequiredSize().getHeight()));
					y += ce->getHeight() + AutoEditor::VerticalSpacing;
				}

				if (mainEditor)
				{
					const auto editorWidth = 3*getWidth()/4 - AutoEditor::HorizontalSpacing;
					const auto x = childEditors[0]->getBounds().getX();
					mainEditor->setBounds(mainEditor->getBounds().withX(x).withY(y).withWidth(editorWidth));

					viewport.setBounds(mainEditor->getRight() + AutoEditor::HorizontalSpacing, mainEditor->getY(), getWidth()-mainEditor->getWidth()-2*HorizontalSpacing, mainEditor->getHeight());

					y += mainEditor->getHeight() + AutoEditor::VerticalSpacing;
				}
				else
				{
					viewport.setBounds(childEditors[0]->getBounds().withHeight(sequenceListView.getCalculatedHeight() + AutoEditor::VerticalSpacing).withY(y));
					y += viewport.getHeight() + AutoEditor::VerticalSpacing;
				}

				sequenceListView.setBounds(juce::Rectangle<int>{}.withHeight(sequenceListView.getCalculatedHeight()).withWidth(viewport.getWidth()).reduced(AutoEditor::HorizontalSpacing, 0));

				for (const auto ce : childSequenceEditors)
					ce->setVisible(expanded && ce==mainEditor);
			}

			juce::Rectangle<int> getRequiredSize() const override
			{
				auto heightOfSequenceChildren = [&](){
					if (mainEditor)
						return mainEditor->getRequiredSize().getHeight();
					else
						return AutoEditor::ComponentHeight*2  + AutoEditor::VerticalSpacing * 2;
				};

				auto heightOfChildren = [&, heightOfSequenceChildren](){
					auto sum = 0;
					for (auto ce : extraChildEditors)
						sum += ce->getRequiredSize().getHeight();
					return sum + heightOfSequenceChildren();
				};

				auto heightOfListComponents = [&](){
					return 0;
				}();


				auto w = getWidth();
				auto h = (!expanded ? 0 : static_cast<int>(sliders.size() + comboBoxes.size() + buttons.size()) * (ComponentHeight + VerticalSpacing))
									+ VerticalSpacing // extra spacing at bottom
									+ (presetPanel ? PresetPanelHeight + VerticalSpacing : 0)
									+ juce::jmax(titleLabel.getHeight(), collapseButton.getHeight()) + VerticalSpacing
									+ jmax((expanded ? heightOfChildren() : 0), heightOfListComponents);
				return {w, h};
			}

			void setExpanded(const bool shouldBeExpanded) override
			{
				expanded = shouldBeExpanded;
				for (const auto c : paramComponents)
					c->setVisible(expanded);

				for (const auto& l : paramNames)
					l->setVisible(expanded);


				for (const auto & childEditor : childEditors)
					childEditor->setVisible(childEditor.get() == mainEditor && expanded);



				onLayoutChanged();
			}

			void onItemSelectionChanged(const size_t processorID) override
			{
				if (processorID < childEditors.size())
					mainEditor = childEditors[processorID].get();
				else
					mainEditor = nullptr;

				onLayoutChanged();
			}

		private:
			AutoEditor* mainEditor{nullptr}; // non owning, points to selected child from child editors
			Viewport viewport;
			std::vector<AutoEditor*> childSequenceEditors;
			std::vector<AutoEditor*> extraChildEditors;
			SequenceListView sequenceListView;
		};

	}

	void AutoEditor::buildChildEditors()
	{
		for (auto& child : parameterGroup.getSubgroups(false))
		{
			jassert(child);
			auto childMetaData = [&](){
				for (auto& c : metadata.children)
					if (c.groupID == child->getID()) return c;

				jassertfalse;
				return helpers::ParameterFactory::GroupMetadata{};
			}();
			if (childMetaData.isSelectorGroup())
			{
				childEditors.push_back(std::make_unique<DeviceSelectorEditor>(apvts, *child, childMetaData));
			}
			else if (childMetaData.isDynamicProcessorSequenceGroup())
			{
				childEditors.push_back(std::make_unique<DynamicProcessorSequenceEditor>(apvts, *child, childMetaData));
			}
			else
			{
				childEditors.push_back(std::make_unique<AutoEditor>(apvts, *child, childMetaData));
			}

			addAndMakeVisible(childEditors.back().get());
			childEditors.back()->buildChildEditors();
		}
		onLayoutChanged();
	}

	void GenericEditor::initialiseMainEditor(juce::AudioProcessorValueTreeState& apvts,
											const juce::AudioProcessorParameterGroup& parameterGroup,
											const helpers::ParameterFactory::GroupMetadata& metadata)
	{
		if (metadata.isSelectorGroup())
		{
			mainEditor = std::make_unique<DeviceSelectorEditor>(apvts, parameterGroup, metadata);
		}
		else if (metadata.isDynamicProcessorSequenceGroup())
		{
			mainEditor = std::make_unique<DynamicProcessorSequenceEditor>(apvts, parameterGroup, metadata);
		}
		else
		{
			mainEditor = std::make_unique<AutoEditor>(apvts, parameterGroup, metadata);
		}
	}

}