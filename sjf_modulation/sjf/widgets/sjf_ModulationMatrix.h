/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 12/10/2026.
//

#pragma once
#include <JuceHeader.h>

namespace sjf::gui::modulation{

template<typename ...Modulators>
class ModulationMatrix :	public juce::Component,
							public juce::TableListBoxModel,
							public ValueTree::Listener
{
	using ModulationSystem = dsp::modulation::ModulationSystem<Modulators...>;
	using Modulator = ModulationSystem::Modulator;
	using Modulatable = ModulationSystem::Modulatable;
	using Connections = ModulationSystem::Connections;
	using Connection = ModulationSystem::Connection;

	struct SourceBox : juce::ComboBox
	{
		SourceBox(juce::AudioProcessorValueTreeState& apvts_, ModulationSystem& modSystem_, UndoManager* undoManager_, const int connectionNumber, Connections& connections_)
		: apvts(apvts_), modSystem(modSystem_), undoManager(undoManager_), number(connectionNumber), connections(connections_)
		{
			juce::ComboBox::setTooltip("Select a modulator");
			if (static_cast<size_t>(number) >= connections.size())
			{
				setText("Select modulator...");
			}
			else
			{
				if (auto modulator = connections[static_cast<size_t>(number)].source)
				{
					setText(modulator->getModulatorID());
				}
				else
				{
					jassertfalse;
				}
			}
		}


		void mouseDown(const MouseEvent& event) override
		{
			auto popup = buildPopup();
			juce::PopupMenu::MenuItemIterator it{popup, true};
			{
				auto itemID = 1;
				while (it.next())
				{
					it.getItem().itemID = itemID++;
				}
			}
			(*getRootMenu()) = popup;

			juce::ComboBox::mouseDown(event);
		}

		juce::PopupMenu buildPopup()
		{
			juce::PopupMenu menu{};

			auto defaultConnection = [&](){
				if (auto matrix = findParentComponentOfClass<ModulationMatrix>())
					return &matrix->newConnection;
				return static_cast<Connection*>(nullptr);
			};

			auto currentConnection = static_cast<size_t>(number) < connections.size() ? &connections[static_cast<size_t>(number)] : defaultConnection();


			for (auto modID : modSystem.modulators.getIndexToId())
			{
				auto modulator = modSystem.modulators.getModulator(modID);

				auto isSameGroup = [this, modID, currentConnection](){
					if (!currentConnection || !currentConnection->target) return false;
					auto param = dynamic_cast<juce::RangedAudioParameter*>(currentConnection->target);
					auto groups = apvts.processor.getParameterTree().getGroupsForParameter(param);
					return std::find_if(groups.begin(), groups.end(), [modID](auto& grp){ return grp->getID() == modID;}) != groups.end();
				};


				auto enabled = !currentConnection || !currentConnection->target || !(modSystem.isConnected(modSystem.modulators.getModulator(modID), currentConnection->target) || isSameGroup());


				menu.addItem(modID, enabled, false, [this, modulator, modID, safeThis = SafePointer(this)](){
					if (!safeThis) return;

					if (static_cast<size_t>(number) >= connections.size())
					{
						if (auto matrix = findParentComponentOfClass<ModulationMatrix>())
						{

							if (auto& newConnection_ = matrix->newConnection; newConnection_.target)
							{
								if (!modSystem.isConnected(modulator, newConnection_.target))
								{
									newConnection_.source = modulator;
									newConnection_.depth = 1.0f;

									auto ranged = dynamic_cast<juce::RangedAudioParameter*>(newConnection_.target);
									jassert(ranged);
									auto paramID = ranged->getParameterID();

									if (undoManager)
									{
										undoManager->beginNewTransaction();
									}
									modSystem.addConnection(newConnection_, undoManager);
									if (undoManager)
									{
										undoManager->setCurrentTransactionName("Added modulation connection: " + modID + " ==> " + paramID );
										undoManager->beginNewTransaction();
									}
									newConnection_.source = nullptr;
									newConnection_.target = nullptr;
								}
							}
							else
							{
								newConnection_.source = modulator;
								setText(modID);
							}
						}
						else
						{
							jassertfalse;
						}
					}
					else
					{
						auto oldConnection_ = connections[static_cast<size_t>(number)];
						auto newConnection_ = oldConnection_;
						newConnection_.source = modulator;

						auto oldModID = oldConnection_.source->getModulatorID();
						auto ranged = dynamic_cast<juce::RangedAudioParameter*>(newConnection_.target);

						if (undoManager)
						{
							undoManager->beginNewTransaction();
						}
						modSystem.removeConnection(oldConnection_, undoManager);
						modSystem.addConnection(newConnection_, undoManager);
						if (undoManager)
						{
							undoManager->setCurrentTransactionName("Changed modulation source from " + oldModID + " to " + modID + "(target: " + ranged->getParameterID() + ")");
							undoManager->beginNewTransaction();
						}
					}
				});
			}

			return menu;
		}

		juce::AudioProcessorValueTreeState& apvts;
		ModulationSystem& modSystem;
		UndoManager* undoManager{nullptr};
		const int number;
		Connections& connections;
	};

	struct TargetBox : juce::ComboBox
	{
		TargetBox(juce::AudioProcessorValueTreeState& apvts_, ModulationSystem& modSystem_, UndoManager* undoManager_, const int connectionNumber, Connections& connections_)
		: apvts(apvts_), modSystem(modSystem_), undoManager(undoManager_), number(connectionNumber), connections(connections_)
		{
			juce::ComboBox::setTooltip("Select a modulatable parameter");
			if (static_cast<size_t>(number) >= connections.size())
			{
				setText("Select modulatable parameter...");
			}
			else
			{
				auto modulatable = connections[static_cast<size_t>(number)].target;
				if (auto ranged = dynamic_cast<juce::RangedAudioParameter*>(modulatable))
				{
					setText(ranged->getName(1000));
				}
				else
				{
					jassertfalse;
				}
			}
		}


		void mouseDown(const MouseEvent& event) override
		{
			auto defaultConnection = [&](){
				if (auto matrix = findParentComponentOfClass<ModulationMatrix>())
					return &matrix->newConnection;
				return static_cast<Connection*>(nullptr);
			};


			auto popup = buildPopup(apvts.processor.getParameterTree(), static_cast<size_t>(number) < connections.size() ? &connections[static_cast<size_t>(number)] : defaultConnection());
			juce::PopupMenu::MenuItemIterator it{popup, true};
			{
				auto itemID = 1;
				while (it.next())
				{
					it.getItem().itemID = itemID++;
				}
			}
			(*getRootMenu()) = popup;

			juce::ComboBox::mouseDown(event);
		}

		juce::PopupMenu buildPopup(const AudioProcessorParameterGroup& group, const Connection* connection)
		{
			juce::PopupMenu menu{};
			if (connection && connection->source && connection->source->getModulatorID() == group.getID())
				return menu;

			for ( auto child : group.getSubgroups(false))
			{
				menu.addSubMenu(helpers::ParameterFactory::getNameWithoutParentPrefix(*child), buildPopup(*child, connection));
			}

			for (auto param: group.getParameters(false))
			{
				if (auto modulatable = dynamic_cast<Modulatable*>(param); modulatable && modulatable->isModulatable())
				{

					auto ranged = dynamic_cast<juce::RangedAudioParameter*>(param);
					jassert(ranged);
					auto modulator = connection ? connection->source : nullptr;

					auto enabled = !(connection && connection->target == modulatable);
					auto ticked = !enabled;
					menu.addItem(helpers::ParameterFactory::getNameWithoutParentPrefix(*ranged, group), enabled, ticked, [this, modulator, param, ranged, modulatable, safeThis = SafePointer(this)](){
						if (!safeThis) return;


						if (static_cast<size_t>(number) >= connections.size())
						{
							if (auto matrix = findParentComponentOfClass<ModulationMatrix>())
							{
								if (auto& newConnection_ = matrix->newConnection; newConnection_.source)
								{
									auto isSameGroup = [&](){
										if (!modulator) return false;
										auto groups = apvts.processor.getParameterTree().getGroupsForParameter(param);
										return std::find_if(groups.begin(), groups.end(), [modID = modulator->getModulatorID()](auto& grp){ return grp->getID() == modID;}) != groups.end();
									};

									if (!(modSystem.isConnected(matrix->newConnection.source, modulatable) || isSameGroup()))
									{
										newConnection_.target = modulatable;
										newConnection_.depth = 1.0f;

										auto modID = modulator->getModulatorID();
										auto paramID = ranged->getParameterID();

										if (undoManager)
										{
											undoManager->beginNewTransaction();
										}
										modSystem.addConnection(newConnection_, undoManager);
										if (undoManager)
										{
											undoManager->setCurrentTransactionName("Added modulation connection: " + modID + " ==> " + paramID );
											undoManager->beginNewTransaction();
										}
										newConnection_.source = nullptr;
										newConnection_.target = nullptr;
									}
								}
								else
								{
									newConnection_.target = modulatable;
									setText(ranged->getName(1000));
								}
							}
							else
							{
								jassertfalse;
							}
						}
						else
						{
							auto oldConnection_ = connections[static_cast<size_t>(number)];
							auto newConnection_ = oldConnection_;
							newConnection_.target = modulatable;

							auto oldParam = dynamic_cast<RangedAudioParameter*>(oldConnection_.target);
							jassert(oldParam);
							auto oldParamID = oldParam->getParameterID();
							auto modID = modulator->getModulatorID();
							auto paramID = ranged->getParameterID();

							if (undoManager)
							{
								undoManager->beginNewTransaction();
							}
							modSystem.removeConnection(oldConnection_, undoManager);
							modSystem.addConnection(newConnection_, undoManager);
							if (undoManager)
							{
								undoManager->setCurrentTransactionName("Changed target of modulation from " + oldParamID + " to " + paramID + "(source: " + modID + ")");
								undoManager->beginNewTransaction();
							}
						}
					});
				}
			}
			return menu;
		}

		juce::AudioProcessorValueTreeState& apvts;
		ModulationSystem& modSystem;
		UndoManager* undoManager{nullptr};
		const int number;
		Connections& connections{};
	};

	struct DepthSlider : public juce::Slider
	{
		DepthSlider(ModulationSystem& modSystem_, UndoManager* undoManager_, Connection connection_)
		: juce::Slider(juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextBoxRight)
		, modSystem(modSystem_), undoManager(undoManager_), connection(connection_)
		{
			juce::Slider::setTooltip("Set the depth of the modulation.");
			setRange(-100.0f, 100.0f, 0.01f);
			setSkewFactorFromMidPoint(0.0f);

			setValue(connection.depth * 100.0f);

			setTextValueSuffix("%");
		}

		void mouseUp(const MouseEvent& e) override
		{
			juce::Slider::mouseUp(e);

			jassert(connection.isValid());
			if (!connection.isValid())
				return;

			auto param = dynamic_cast<RangedAudioParameter*>(connection.target);
			jassert(param);
			auto paramID = param->getParameterID();
			auto modID = connection.source->getModulatorID();

			connection.depth = static_cast<float>(getValue()) * 0.01f;
			if (undoManager)
			{
				undoManager->beginNewTransaction();
			}
			modSystem.addConnection(connection, undoManager);
			if (undoManager)
			{
				undoManager->setCurrentTransactionName("Changed depth of modulation connection: " + modID + " ==> " + paramID + " to: " + juce::String(connection.depth));
				undoManager->beginNewTransaction();
			}
		}

		ModulationSystem& modSystem;
		UndoManager* undoManager{nullptr};
		Connection connection;
	};

	enum class TableHeaders { source = 1, target, depth, polarity, remove, COUNT };

public:
	ModulationMatrix(juce::AudioProcessorValueTreeState& apvts_, ModulationSystem& modSystem_,
						UndoManager* undoManager_) : apvts(apvts_), modSystem(modSystem_), undoManager(undoManager_)
	{
		constexpr auto notSortable = juce::TableHeaderComponent::ColumnPropertyFlags::notSortable;
		table.getHeader().addColumn("Source", static_cast<int>(TableHeaders::source), 125, 125, -1, notSortable);
		table.getHeader().addColumn("Target", static_cast<int>(TableHeaders::target), 125, 125, -1, notSortable);
		table.getHeader().addColumn("Depth", static_cast<int>(TableHeaders::depth), 200, 200, -1, notSortable);
		table.getHeader().addColumn("Polarity", static_cast<int>(TableHeaders::polarity), 50, 50, 50, notSortable);
		table.getHeader().addColumn("Remove", static_cast<int>(TableHeaders::remove), 50, 50, 50, notSortable);

		table.setColour(juce::ListBox::backgroundColourId, table.findColour(juce::ListBox::backgroundColourId).withAlpha(0.2f));
		addAndMakeVisible(table);
		apvts.state.addListener(this);
		id = juce::Identifier(modSystem.getFactoryID() + sjf::dsp::modulation::ids::modulationConnectionsID);

		currentConnections = getCurrentConnections();
	}

	~ModulationMatrix() override
	{
		apvts.state.removeListener(this);
		table.setModel(nullptr);
	}

	void resized() override
	{
		{
			auto widths = std::vector<int>{};
			widths.resize(static_cast<size_t>(table.getHeader().getNumColumns(true)));

			for (auto i = 0ul; i < widths.size(); ++i)
			{
				const auto w = table.getHeader().getColumnWidth(static_cast<int>(i+1));
				widths[i] = i == 0 ? w : w + widths[i-1];
			}

			const auto W = static_cast<float>(getWidth());
			const auto oldW = static_cast<float>(widths.back());
			for (auto i = 0ul; i < widths.size(); ++i)
			{
				const auto w = i == 0ul ? static_cast<float>(widths[0]) : static_cast<float>(widths[i] - widths[i-1]);
				table.getHeader().setColumnWidth(static_cast<int>(i+1), roundToInt((w/oldW)*W));
			}
		}

		table.setBounds(getLocalBounds());
	}

	int getNumRows() override
	{
		auto connections = getCurrentConnections();
		return static_cast<int>(connections.size()) + 1;
	}

	void paint(Graphics& /*g*/) override
	{
		// g.fillAll(juce::Colours::magenta);
	}

	void paintRowBackground (Graphics& g, int /*rowNumber*/, int /*width*/, int /*height*/, bool /*rowIsSelected*/) override
	{
		const auto lnf4 = dynamic_cast<juce::LookAndFeel_V4*>(&getLookAndFeel());
		jassert(lnf4 != nullptr);
		auto col = lnf4->getCurrentColourScheme().getUIColour(juce::LookAndFeel_V4::ColourScheme::UIColour::windowBackground).withAlpha(0.5f);
		g.fillAll(col);

		auto border = lnf4->getCurrentColourScheme().getUIColour(juce::LookAndFeel_V4::ColourScheme::UIColour::outline);
		g.setColour(border);
		g.drawRect(g.getClipBounds());
	}

	void paintCell (Graphics& /*g*/, int /*rowNumber*/, int /*columnId*/, int /*width*/, int /*height*/, bool /*rowIsSelected*/) override
	{
		// g.fillAll(juce::Colours::black);
	}

	Component* refreshComponentForCell (int rowNumber, int columnId, bool /*isRowSelected*/, Component* existingComponentToUpdate) override
	{
		if (existingComponentToUpdate)
			delete existingComponentToUpdate;

		if (!apvts.state.isValid() || !apvts.state.getChildWithName(id).isValid())
		{
			return nullptr;
		}


		if (columnId == static_cast<int>(TableHeaders::source))
			return new SourceBox(apvts, modSystem, undoManager, rowNumber, currentConnections);

		if (columnId == static_cast<int>(TableHeaders::target))
			return new TargetBox(apvts, modSystem, undoManager, rowNumber, currentConnections);

		if (rowNumber < static_cast<int>(currentConnections.size())) // not yet a connection
		{
			if (columnId == static_cast<int>(TableHeaders::depth))
				return new DepthSlider(modSystem, undoManager, currentConnections[static_cast<size_t>(rowNumber)]);

			if (columnId == static_cast<int>(TableHeaders::polarity))
			{
				auto polarityButton = new TextButton{};
				polarityButton->setTooltip("Set whether the modulation is bipolar or unipolar.");
				auto bipolar = currentConnections[static_cast<size_t>(rowNumber)].bipolar;
				polarityButton->setButtonText(bipolar ? "Bipolar" : "Unipolar");
				polarityButton->onClick = [this, connection = currentConnections[static_cast<size_t>(rowNumber)], safeThis = SafePointer(this)](){
					if (!safeThis) return;

					auto param = dynamic_cast<RangedAudioParameter*>(connection.target);
					jassert(param);
					auto paramID = param->getParameterID();
					auto modID = connection.source->getModulatorID();

					auto alteredConnection = connection;
					alteredConnection.bipolar = !connection.bipolar;
					if (undoManager)
					{
						undoManager->beginNewTransaction();
					}
					modSystem.addConnection(alteredConnection, undoManager);
					if (undoManager)
					{
						undoManager->setCurrentTransactionName("Changed polarity of modulation connection: " + modID + " ==> " + paramID + " to: " + (alteredConnection.bipolar ? "Bipolar" : "Unipolar"));
						undoManager->beginNewTransaction();
					}
				};

				return polarityButton;
			}

			if (columnId == static_cast<int>(TableHeaders::remove))
			{
				auto removeButton = new TextButton{};
				removeButton->setTooltip("Remove modulation connection.");
				removeButton->setButtonText("X");
				removeButton->onClick = [this, connection = currentConnections[static_cast<size_t>(rowNumber)], safeThis = SafePointer(this)](){
					if (!safeThis) return;

					auto param = dynamic_cast<RangedAudioParameter*>(connection.target);
					jassert(param);
					auto paramID = param->getParameterID();
					auto modID = connection.source->getModulatorID();

					if (undoManager)
					{
						undoManager->beginNewTransaction();
					}
					modSystem.removeConnection(connection, undoManager);
					if (undoManager)
					{
						undoManager->setCurrentTransactionName("Removed modulation connection: " + modID + " ==> " + paramID );
						undoManager->beginNewTransaction();
					}
				};

				return removeButton;
			}

			jassertfalse;
			return nullptr;
		}
		return nullptr;
	}
private:

	Connections getCurrentConnections()
	{
		auto connectionsVT_ = apvts.state.getChildWithName(id);
		if (connectionsVT_.isValid())
			return modSystem.stringToConnections(connectionsVT_.getProperty(dsp::modulation::ids::connectionID));

		return {};
	}

	void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier&) override
	{
		if (treeWhosePropertyHasChanged.getType() == id)
		{
			if (MessageManager::existsAndIsLockedByCurrentThread())
			{
				currentConnections = getCurrentConnections();
				table.updateContent();
			}
			else
			{
				asyncUpdate.triggerUpdate();
			}
		}
	}

	juce::TableListBox table { {}, this };
	juce::Identifier id;

	juce::AudioProcessorValueTreeState& apvts;
	ModulationSystem& modSystem;
	UndoManager* undoManager{nullptr};
	Connection newConnection;
	Connections currentConnections{};
	std::vector<int> rowToConnection{};

	using Callback = std::function<void()>;
	sjf::helpers::AsyncCallbackInvoker<Callback> asyncUpdate {[safeThis = SafePointer(this), this](){
		if (!safeThis)
			return;
		currentConnections = getCurrentConnections();
		table.updateContent();
	}};

	friend struct SourceBox;
	friend struct TargetBox;

};

}


