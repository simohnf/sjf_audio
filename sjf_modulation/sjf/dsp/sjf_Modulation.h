/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 27/09/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_ParameterFactory.h>
#include <sjf/helpers/sjf_ProcessorSequence.h>
#include <sjf/helpers/sjf_SPSCTripleBuffer.h>
#include <sjf/helpers/sjf_AsyncCallbackInvoker.h>

namespace sjf::gui::modulation
{
	template<typename ...Modulators>
	class ModulationManager;
}

namespace sjf::dsp::modulation{
	namespace ids
	{
		const static auto modulationConnectionsID = juce::Identifier("ModulationConnections");
		const static auto connectionID = juce::Identifier("Connection");
		const static auto modulatorID = juce::Identifier("Modulator");
		const static auto modulatableID = juce::Identifier("Modulatable");
		const static auto depthID = juce::Identifier("Depth");
		const static auto seperator1 = juce::String{"|"};
		const static auto seperator2 = juce::String{"/"};
	}

	/**
	 * @brief Abstract base class representing a single modulation source.
	 *
	 * `ModulatorBase` defines the pure virtual interface required for any modulation source
	 * (such as LFOs, Envelopes, Step Sequencers, or Macro Controls) to interface with the
	 * `ModulationSystem` and `ModulatorChain`.
	 *
	 * Implementations must provide a unique identifier for state serialization and target
	 * connection lookup, as well as a lock-free sample retrieval method invoked during the DSP loop.
	 *
	 * @see ModulatorChain, ModulationSystem
	 */
	struct ModulatorBase
	{
		virtual ~ModulatorBase() = default;
		[[nodiscard]] virtual juce::String getModulatorID() const = 0;
		[[nodiscard]] virtual float getModulationSample() const noexcept = 0;
	};


	/**
	 * @brief A compile-time sequence container for processing a chain of `ModulatorBase` instances.
	 *
	 * The `ModulatorChain` class encapsulates a compile-time variadic pack of modulator DSP blocks
	 * managed internally by a `helpers::ProcessorSequence<Modulators...>`. It bridges runtime string-based
	 * lookup (`getModulator`) to compile-time sequence element access using modern C++ template fold expressions.
	 *
	 * ### Key Responsibilities:
	 * * **DSP Execution:** Advances all configured modulators synchronously inside a dummy audio block context.
	 * * **Parameter Factory Integration:** Exposes a parameter factory hierarchy for APVTS/GUI construction and maps child subgroup IDs to internal tuple indices.
	 * * **Fast Lookup:** Provides $O(1)$ mapping from string IDs to internal sequence indices, unrolling parameter access without dynamic dispatch overhead in the DSP thread.
	 *
	 * ### Example Configuration:
	 * @code
	 * using MyModulatorChain = sjf::dsp::modulation::ModulatorChain<
	 *     sjf::dsp::modulation::lfo::BasicLFO,
	 *     sjf::dsp::modulation::lfo::BasicLFO
	 * >;
	 *
	 * MyModulatorChain chain;
	 * chain.prepare(processSpec);
	 * @endcode
	 *
	 * NOTE: Every element of the template pack must derive from ModulatorBase
	 *
	 * @tparam Modulators A variadic pack of concrete modulator types inheriting from `ModulatorBase`.
	 * @see ModulatorBase, ModulationSystem
	 */
	template <typename ...Modulators>
	struct ModulatorChain
	{
		using Sequence = helpers::ProcessorSequence<Modulators...>;
		using Modulator = ModulatorBase;


		static_assert((std::is_base_of_v<Modulator, Modulators> && ...),
				  "All types in ModulatorChain must derive from ModulatorBase!");

		void prepare (const juce::dsp::ProcessSpec& spec_)
		{
			dummyBuffer.setSize(static_cast<int>(spec_.numChannels), static_cast<int>(spec_.maximumBlockSize));
			seq.prepare(spec_);
			reset();
		}

		void reset()
		{
			seq.reset();
		}

		template <typename ProcessContext>
		void process(const ProcessContext& context)
		{
			auto block = juce::dsp::AudioBlock<float>(dummyBuffer).getSubBlock(0, context.getOutputBlock().getNumSamples());
			auto modContext = juce::dsp::ProcessContextReplacing<float>(block);
			seq.process(modContext);
		}

		template <typename... Configs>
		std::unique_ptr<helpers::ParameterFactory> createParameters (
			const juce::String& factoryID,
			const juce::String& factoryName,
			Configs&&... subConfigs)
		{
			auto mainFactory = seq.createParameters(factoryID, factoryName, std::forward<Configs>(subConfigs)...);

			{
				auto i = 0ul;
				for (auto child : mainFactory->getSubgroups(false))
				{
					if (i >= sizeof...(Modulators))
					{
						jassertfalse; // More parameter subgroups created than Modulators in the pack!
						break;
					}
					auto id = child->getID();
					idToTupleIndexMap[id] = i;
					indexToID[i] = id;
					i++;
				}
			}
			return mainFactory;
		}

		void setPositionInfo(const juce::AudioPlayHead::PositionInfo& positionInfo_)
		{
			seq.setPositionInfo(positionInfo_);
		}

		int getLatencySamples()
		{
			return seq.getLatencySamples();
		}


		void attachToState (juce::ValueTree& parentTree)
		{
			seq.attachToState(parentTree);
		}

		Sequence& getSequence()
		{
			return seq;
		}

		const std::unordered_map<juce::String, size_t>& getIdToIndex()
		{
			return idToTupleIndexMap;
		}

		const std::array<juce::String, sizeof...(Modulators)>& getIndexToId()
		{
			return indexToID;
		}


		Modulator* getModulator(const juce::String& modID)
		{
			if (const auto it = idToTupleIndexMap.find(modID); it != idToTupleIndexMap.end())
			{
				const auto indx = it->second;

				return [this, indx]<size_t... Is>(std::index_sequence<Is...>) -> Modulator*
				{
					Modulator* result = nullptr;

					// Folds across all indices; short-circuits on match
					(void)((Is == indx ? (result = &seq.template get<Is>(), true) : false) || ...);

					return result;
				}(std::make_index_sequence<sizeof...(Modulators)>{});
			}

			jassertfalse;
			return nullptr;
		}

	private:
		Sequence seq;
		juce::AudioBuffer<float> dummyBuffer;

		std::unordered_map<juce::String, size_t> idToTupleIndexMap;
		std::array<juce::String, sizeof...(Modulators)> indexToID;
	};

	/**
	 * @brief Central manager class for handling thread-safe, lock-free parameter modulation and state tracking.
	 *
	 * `ModulationSystem` coordinates the lifecycle, active connections, and runtime sum accumulation
	 * between `ModulatorBase` sources managed by an internal `ModulatorChain` and `Modulatable` parameter targets.
	 *
	 * ### Critical Execution Order Constraints:
	 * @warning **`prepare()`, `reset()`, and `process()` MUST be executed BEFORE any audio processors that are modulated by this system!**
	 *
	 * Because `ModulationSystem::process()` advances all internal modulators, computes weighted modulation sums,
	 * and sets the additive modulation offsets on target `Modulatable` parameters, invoking processor DSP code
	 * before `ModulationSystem::process()` will result in processing with stale or un-cleared modulation values from the previous block.
	 *
	 * ### Thread-Safety & State Synchronization Architecture:
	 * * **Audio Thread:** Executes lock-free. Consumes updated active connection vectors via `SPSCTripleBuffer` and processes parameter removals lock-free via `RemovedConnectionsFIFO`.
	 * * **Message Thread:** Intercepts APVTS state tree mutations (`juce::ValueTree::Listener`), serializes connections into readable strings (`SourceID|ParamID|Depth`), and safely updates connection buffers asynchronously.
	 *
	 * ### Example Audio Loop Integration:
	 * @code
	 * void prepareToPlay(double sampleRate, int samplesPerBlock) override
	 * {
	 *     juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, (juce::uint32) getTotalNumOutputChannels() };
	 *
	 *     // 1. Prepare Modulation System FIRST
	 *     modulationSystem.prepare(spec);
	 *
	 *     // 2. Prepare downstream processors AFTER
	 *     mainFilter.prepare(spec);
	 * }
	 *
	 * void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override
	 * {
	 *     juce::dsp::AudioBlock<float> block(buffer);
	 *     juce::dsp::ProcessContextReplacing<float> context(block);
	 *
	 *     // 1. Process Modulation System FIRST to update all parameter offsets
	 *     modulationSystem.process(context);
	 *
	 *     // 2. Process downstream processors SECOND (parameters will evaluate with updated offsets)
	 *     mainFilter.process(context);
	 * }
	 * @endcode
	 *
	 * @tparam Modulators Variadic parameter pack representing all concrete modulator types managed by the system.
	 * @see ModulatorChain, ModulatorBase
	 */
	template <typename ...Modulators>
	class ModulationSystem : private juce::ValueTree::Listener
	{
	public:
		using Modulator = ModulatorBase;
		using Modulatable = helpers::parameters::ParameterBase;

		class Connection
		{
		public:
			Modulator* source	= nullptr;
			Modulatable* target	= nullptr;
			float depth         = 0.0f;
			bool bipolar = true;

			bool isValid() const
			{
				return source && target && dynamic_cast<RangedAudioParameter*>(target);
			}
		};

		using Connections = std::vector<Connection>;

		ModulationSystem() = default;

		~ModulationSystem() override
		{
			apvts->state.removeListener(this);
		}

		void prepare (const juce::dsp::ProcessSpec& spec_)
		{
			modulators.prepare(spec_);
			reset();
		}

		void reset()
		{
			modulators.reset();
			if (apvts)
			{
				for ( auto& p : apvts->processor.getParameters())
				{
					if ( auto modulatable = dynamic_cast<Modulatable*>(p))
						modulatable->clearModulationOffset();
				}
			}
		}

		template <typename ProcessContext>
		void process(const ProcessContext& context)
		{

			jassert(apvts);
			// 1. Advance modulators in chain
			modulators.process(context);

			auto& connections = connectionsBuffer.getRead();

			// 2a. Clear previous offsets on all non-active targets
			connectionRemover.removeAll();
			// 2b. Clear previous offsets on all active targets
			for (const auto& conn : connections)
				if (conn.target != nullptr)
					conn.target->setModulationOffset(0.0f);

			// 3. Accumulate weighted modulation sums
			auto calculateOffset = [](float val, const float depth, const bool bipolar)
			{
				if (!bipolar)
					val = (val+1.0f)*0.5f;
				return val * depth;
			};

			for (const auto& conn : connections)
			{
				if (conn.source != nullptr && conn.target != nullptr)
				{
					const float currentOffset = conn.target->getModulationOffset();
					const float addedOffset   = calculateOffset(conn.source->getModulationSample(), conn.depth, conn.bipolar);

					conn.target->setModulationOffset(currentOffset + addedOffset);
				}
			}
		}

		template <typename... Configs>
		std::unique_ptr<helpers::ParameterFactory> createParameters (
			const juce::String& factoryID,
			const juce::String& factoryName,
			Configs&&... subConfigs)
		{
			factoryId = factoryID;
			id = juce::Identifier(factoryId + ids::modulationConnectionsID);
			auto factory = modulators.createParameters(factoryID, factoryName, std::forward<Configs>(subConfigs)...);

			return factory;
		}

		void setPositionInfo(const juce::AudioPlayHead::PositionInfo& positionInfo_)
		{
			modulators.setPositionInfo(positionInfo_);
		}

		int getLatencySamples()
		{
			return modulators.getLatencySamples();
		}

		void attachAPVTS(juce::AudioProcessorValueTreeState& apvts_)
		{
			apvts = &apvts_;
			attachToState(apvts->state);

			connectionRemover.resize(static_cast<size_t>(apvts->processor.getParameters().size()));

			auto stateTree = apvts->state.getOrCreateChildWithName(id, nullptr);

			if (MessageManager::existsAndIsCurrentThread())
			{
				apvts->state.addListener(this);
				publishConnectionsUpdate();
			}
			else if (auto mm = MessageManager::getInstanceWithoutCreating())
			{
				mm->callAsync([this, g = std::weak_ptr(guard)](){
					if (!g.expired() && apvts->state.isValid())
					{
						apvts->state.addListener(this);
						publishConnectionsUpdate();
					}
				});
			}
		}

		void attachToState (juce::ValueTree& parentTree)
		{
			jassert(apvts);
			modulators.attachToState(parentTree);

		}

		const juce::String& getFactoryID()
		{
			return factoryId;
		}

		ValueTree getCurrentConnectionsAsValueTree() const
		{
			jassert(apvts);
			return apvts->state.getChildWithName(id);
		}

				//==============================================================================
		// HELPERS FOR CONVERTING TO AND FROM VALUE TREE
		//==============================================================================

		juce::String connectionAsString(Connection& connection)
		{

			if (connection.isValid())
			{
				auto ret = juce::StringArray{};
				ret.add(connection.source->getModulatorID());
				ret.add(dynamic_cast<RangedAudioParameter*>(connection.target)->paramID);
				ret.add(juce::String(connection.depth));
				ret.add(juce::String(connection.bipolar ? "Bipolar" : "Unipolar"));

				return ret.joinIntoString(ids::seperator1);
			}
			else
			{
				jassertfalse;
				return {};
			}
		}


		juce::String connectionsAsString(Connections& connections)
		{
			auto ret = juce::StringArray{};
			for ( auto& c : connections)
				ret.add(connectionAsString(c));

			return ret.joinIntoString(ids::seperator2);
		}

		Connection stringToConnection( const juce::String& connectionString)
		{
			const auto arr = juce::StringArray::fromTokens(connectionString, ids::seperator1, "");


			Connection result{};

			if (arr.size() < 3 || !apvts)
				return result;

			result.source = modulators.getModulator(arr[0]);
			result.target = dynamic_cast<Modulatable*>(apvts->getParameter(arr[1]));
			result.depth = arr[2].getFloatValue();

			result.bipolar = arr.size() < 4 ? true : arr[3]=="Bipolar";

			if (!result.target)
			{
				jassertfalse;
				return {};
			}

			if (!result.source)
			{
				jassertfalse;
				return {};
			}

			return result;
		}


		Connections stringToConnections(const juce::String& connectionsString)
		{
			Connections ret{};
			const auto arr = juce::StringArray::fromTokens(connectionsString, ids::seperator2, "");
			ret.reserve(static_cast<size_t>(arr.size()));
			for (const auto& str : arr)
			{
				if (auto connection = stringToConnection(str); connection.isValid())
					ret.push_back(connection);
			}

			return ret;
		}

		void addConnection(const juce::String& modId, const juce::String& paramId, const float depth, bool bipolar, UndoManager* undoManager)
		{
			if (apvts)
			{
				if (paramId.startsWith(modId))
					return; // don't allow attaching mods to their own parameters

				auto mod = modulators.getModulator(modId);
				auto param = apvts->getParameter(paramId);
				if (mod && param && dynamic_cast<Modulatable*>(param))
				{
					addConnection(Connection{mod, dynamic_cast<Modulatable*>(param), depth, bipolar}, undoManager);
				}
				else
				{
					jassertfalse;
				}
			}
			else
			{
				jassertfalse;
			}
		}

		void addConnection(const Connection& connection, UndoManager* undoManager)
		{
			if (apvts)
			{
				auto stateTree = apvts->state.getChildWithName(id);
				auto str = stateTree.getProperty(ids::connectionID, "").toString();
				auto connections = str.isEmpty() ? Connections{} : stringToConnections(str);
				if (auto pos = std::find_if(connections.begin(), connections.end(),[&](auto& c){ return c.source == connection.source && c.target == connection.target; }); pos != connections.end())
				{
					pos->depth = connection.depth;
					pos->bipolar = connection.bipolar;
				}
				else
				{
					connections.push_back(connection);
				}

				stateTree.setProperty(ids::connectionID, connectionsAsString(connections), undoManager);
			}
			else
			{
				jassertfalse;
			}
		}

		void removeConnection(const juce::String& modId, const juce::String& paramId, UndoManager* undoManager)
		{
			if (apvts)
			{
				auto stateTree = apvts->state.getChildWithName(id);
				auto mod = modulators.getModulator(modId);
				auto param = apvts->getParameter(paramId);

				if (mod && param && dynamic_cast<Modulatable*>(param))
				{
					removeConnection(Connection{mod, dynamic_cast<Modulatable*>(param), 0.0f}, undoManager);
				}
				else
				{
					jassertfalse;
				}
			}
			else
			{
				jassertfalse;
			}
		}

		void removeConnection(const Connection& connection, UndoManager* undoManager)
		{
			if (apvts)
			{
				auto stateTree = apvts->state.getChildWithName(id);
				auto str = stateTree.getProperty(ids::connectionID, "").toString();
				auto connections = str.isEmpty() ? Connections{} : stringToConnections(str);
				if (auto pos = std::find_if(connections.begin(), connections.end(),[&](auto& c){ return c.source == connection.source && c.target == connection.target; }); pos != connections.end())
				{
					connections.erase(pos);
					stateTree.setProperty(ids::connectionID, connectionsAsString(connections), undoManager);
				}
				else
				{
					jassertfalse;
				}
			}
			else
			{
				jassertfalse;
			}
		}


		void removeAllConnections(UndoManager* undoManager)
		{
			if (apvts)
			{
				auto stateTree = apvts->state.getChildWithName(id);
				stateTree.setProperty(ids::connectionID, "", undoManager);
			}
			else
			{
				jassertfalse;
			}
		}

		void disconnect(Modulatable& modulatable, UndoManager* undoManager)
		{
			jassert(MessageManager::existsAndIsLockedByCurrentThread());

			if (apvts)
			{
				if (!isModulated(&modulatable))
					return;

				auto stateTree = apvts->state.getChildWithName(id);

				const auto str = stateTree.getProperty(ids::connectionID, "").toString();
				auto connections = str.isEmpty() ? Connections{} : stringToConnections(str);
				auto newConnections = Connections{};
				newConnections.reserve(connections.size());
				for (auto connection : connections)
				{
					if (connection.target != &modulatable)
						newConnections.push_back(connection);
				}
				stateTree.setProperty(ids::connectionID, connectionsAsString(newConnections), undoManager);
			}
			else
			{
				jassertfalse;
			}
		}

		bool isConnected(const juce::String& modulatorID, const juce::String& modulatableId)
		{
			if (apvts)
			{
				auto stateTree = apvts->state.getChildWithName(id);
				auto modulator = modulators.getModulator(modulatorID);
				auto modulatable = dynamic_cast<Modulatable*>(apvts->getParameter(modulatableId));
				return isConnected(modulator, modulatable);
			}

			jassertfalse;
			return false;
		}

		bool isConnected(const Modulator* modulator, const Modulatable* modulatable)
		{
			if (apvts)
			{
				auto stateTree = apvts->state.getChildWithName(id);
				if (!(modulator && modulatable))
					return false;
				const auto str = stateTree.getProperty(ids::connectionID, "").toString();
				auto connections = str.isEmpty() ? Connections{} : stringToConnections(str);
				return std::find_if(connections.begin(), connections.end(),[&](auto& c){ return c.source == modulator && c.target == modulatable; }) != connections.end();
			}

			jassertfalse;
			return false;
		}

		bool isModulated(const Modulatable* modulatable)
		{
			if (apvts)
			{
				if (!modulatable)
					return false;
				auto stateTree = apvts->state.getChildWithName(id);
				const auto str = stateTree.getProperty(ids::connectionID, "").toString();
				auto connections = str.isEmpty() ? Connections{} : stringToConnections(str);
				return std::find_if(connections.begin(), connections.end(),[&](auto& c){ return c.target == modulatable; }) != connections.end();
			}

			jassertfalse;
			return false;
		}


		Connection getConnection(const juce::String& modulatorID, const juce::String& modulatableId)
		{
			if (apvts)
			{
				auto stateTree = apvts->state.getChildWithName(id);
				auto modulator = modulators.getModulator(modulatorID);
				auto modulatable = dynamic_cast<Modulatable*>(apvts->getParameter(modulatableId));
				return getConnection(modulator, modulatable);
			}

			jassertfalse;
			return {};
		}

		Connection getConnection(const Modulator* modulator, const Modulatable* modulatable)
		{
			if (apvts)
			{
				auto stateTree = apvts->state.getChildWithName(id);
				const auto str = stateTree.getProperty(ids::connectionID, "").toString();
				auto connections = str.isEmpty() ? Connections{} : stringToConnections(str);
				if (auto conn = std::find_if(connections.begin(), connections.end(),[&](auto& c){ return c.source == modulator && c.target == modulatable; }); conn != connections.end())
				{
					return *conn;
				}
				return {};
			}

			jassertfalse;
			return {};
		}


		ModulatorChain<Modulators...> modulators;

	private:
		void publishConnectionsUpdate()
		{
			jassert(MessageManager::existsAndIsCurrentThread());
			auto stateTree = apvts->state.getChildWithName(id);
			if (stateTree.isValid())
			{
				auto xml = stateTree.toXmlString();
				jassert(stateTree.getType() == id);
				auto& connections = connectionsBuffer.getWrite();

				connections = stringToConnections(stateTree.getProperty(ids::connectionID, "").toString());

				connectionRemover.updateActiveConnections(connections);
				connectionsBuffer.updateWriteIndex();
			}
			else
			{
				jassertfalse;
				connectionRemover.updateActiveConnections({});
			}
		}

		//==============================================================================
		// VALUETREE LISTENER OVERRIDES
		//==============================================================================
		void valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged,
									   const juce::Identifier&) override
		{
			if (treeWhosePropertyHasChanged.getType() == id || treeWhosePropertyHasChanged.getType() == ids::connectionID)
			{
				if (MessageManager::existsAndIsCurrentThread())
					publishConnectionsUpdate();
				else
					asyncUpdater.triggerUpdate();
			}
		}

		void valueTreeChildAdded (ValueTree& parentTree,
								  ValueTree&) override
		{
			if (parentTree.getType() == id)
			{
				if (MessageManager::existsAndIsCurrentThread())
					publishConnectionsUpdate();
				else
					asyncUpdater.triggerUpdate();
			}
		}

		void valueTreeChildRemoved (ValueTree& parentTree,
									ValueTree&,
									int) override
		{
			if (parentTree.getType() == id)
			{
				if (MessageManager::existsAndIsCurrentThread())
					publishConnectionsUpdate();
				else
					asyncUpdater.triggerUpdate();
			}
		}

		void valueTreeRedirected(ValueTree& treeWhichHasBeenChanged) override
		{
			if (treeWhichHasBeenChanged == apvts->state)
			{
				attachToState(treeWhichHasBeenChanged);
				if (MessageManager::existsAndIsCurrentThread())
					publishConnectionsUpdate();
				else
					asyncUpdater.triggerUpdate();
			}
		}


		//==============================================================================
		// THREAD SAFE UPDATING
		//==============================================================================

		/// handles updates of active connections between threads
		helpers::SPSCTripleBuffer<Connections> connectionsBuffer;

		/// lock free queue for de-connecting
		struct RemovedConnectionsFIFO
		{
			RemovedConnectionsFIFO() : fifo(128)
			{
				modulatables.resize(128, nullptr);
			}

			void updateActiveConnections(const Connections& connections)
			{
				for (auto& c : activeConnections)
				{
					if (auto con = std::find_if(connections.begin(), connections.end(),[&c](auto& x){
							return c.target == x.target && c.source == x.source;
						}); con == connections.end())
						addForRemoval(c.target);
				}
				activeConnections = connections;
			}

			void removeAll()
			{
				auto scope = fifo.read(fifo.getNumReady());
				if (scope.blockSize1 > 0)
				{
					for (auto i = static_cast<size_t>(scope.startIndex1); i < static_cast<size_t>(scope.startIndex1 + scope.blockSize1); ++i)
					{
						if (modulatables[i])
						{
							modulatables[i]->clearModulationOffset();
						}
						else
						{
							jassertfalse;
						}
					}
				}
				if (scope.blockSize2 > 0)
				{
					for (auto i = static_cast<size_t>(scope.startIndex2); i < static_cast<size_t>(scope.startIndex2 + scope.blockSize2); ++i)
					{
						if (modulatables[i])
						{
							modulatables[i]->clearModulationOffset();
						}
						else
						{
							jassertfalse;
						}
					}
				}
			}

			void resize(const size_t size)
			{
				modulatables.resize(size, nullptr);
				fifo.setTotalSize(static_cast<int>(size));
			}
		private:
			void addForRemoval(Modulatable* modulatableToDisconnect)
			{
				auto scope = fifo.write(1);
				if (scope.blockSize1 > 0)
					modulatables[static_cast<size_t>(scope.startIndex1)] = modulatableToDisconnect;

				if (scope.blockSize2 > 0)
					modulatables[static_cast<size_t>(scope.startIndex2)] = modulatableToDisconnect;
			}

			/// stores last set connections so that we can disable connections safely when removed
			Connections activeConnections;
			std::vector<Modulatable*> modulatables; /// previously connected modulatables that need to be reset
			juce::AbstractFifo fifo;
		} connectionRemover;

		/// thread safe async callback for triggering connections update if not called from Message Thread
		std::shared_ptr<int> guard = std::make_shared<int>(42);
		using Callback = std::function<void()>;
		helpers::AsyncCallbackInvoker<Callback> asyncUpdater{[this, g = std::weak_ptr(guard)]{
			if (!g.expired())
				publishConnectionsUpdate();
		}};


		juce::AudioProcessorValueTreeState* apvts{nullptr};
		juce::String factoryId{};
		juce::Identifier id;

		friend class sjf::gui::modulation::ModulationManager<Modulators...>;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationSystem)
	};
}


