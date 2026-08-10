/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */

#pragma once
#include <JuceHeader.h>

#include <sjf/helpers/sjf_ParameterFactory.h>
#include <sjf/helpers/sjf_HelperFunctions.h>
#include <sjf/helpers/sjf_OptionalCalls.h>
#include <sjf/helpers/sjf_SPSCTripleBuffer.h>
#include <sjf/helpers/sjf_ProcessorSequence.h>

#include <sjf/helpers/sjf_AsyncCallbackInvoker.h>

namespace sjf::helpers
{
template <typename... Processors>
class DynamicProcessorSequence : private juce::ValueTree::Listener
{
public:
    static_assert(sizeof...(Processors) > 0,
                  "DynamicProcessorSequence must be instantiated with at least one Processor type!");

    static constexpr size_t NumProcessors = sizeof...(Processors);

    // Sentinel value indicating an unused/empty slot in the sequence
    static constexpr size_t InactiveSlot = std::numeric_limits<size_t>::max();

	static_assert(NumProcessors < InactiveSlot,
			  "DynamicProcessorSequence can not be instantiated that many processors!");

    // Fixed-capacity sequence payload matching maximum available tuple elements
    using SequenceOrder = std::array<size_t, NumProcessors>;

    ~DynamicProcessorSequence() override
    {
    	if (stateTree.isValid())
    		stateTree.removeListener(this);
    }

    //==============================================================================
    // EXISTING PUBLIC INTERFACE (Preserved from ProcessorSequence)
    //==============================================================================

    /** Creates parameter factories for host DAW integration. */
    template <typename... Configs>
    std::unique_ptr<ParameterFactory> createParameters(const juce::String& factoryID,
                                                      const juce::String& factoryName,
                                                      Configs&&... subConfigs)
    {
	    factoryId = factoryID;

    	static_assert (sizeof...(Configs) == sizeof...(Processors),
			"The number of configuration parameters must match the number of processors!");

    	auto mainFactory = ParameterFactory::create (factoryID, factoryName);

    	if (mainFactory == nullptr)
    	{
    		jassertfalse;
    		return nullptr;
    	}

    	auto configTuple = std::forward_as_tuple (std::forward<Configs> (subConfigs)...);

    	[&]<std::size_t... Is>(std::index_sequence<Is...>)
		{
			(invokeCreateParameters (
				std::get<Is> (processors),
				mainFactory.get(),
				std::forward<std::tuple_element_t<Is, decltype(configTuple)>> (std::get<Is> (configTuple))
			 ), ...);
		}(std::make_index_sequence<sizeof...(Processors)>{});

    	return mainFactory;
    }

    /** Prepares all underlying processors with the given DSP spec. */
    void prepare(const juce::dsp::ProcessSpec& spec_)
    {
    	spec = spec_;
    	sjf::helpers::functions::utilities::forEach (processors, [&](auto& proc) { proc.prepare (spec); });
    	reset();
    }

    /** Resets internal state across all underlying processors. */
    void reset()
    {
    	sjf::helpers::functions::utilities::forEach (processors, [](auto& proc) { proc.reset(); });
    }

    /** Processes context through processors matching the active sequence order. */
    template <typename ProcessContextType>
    void process(const ProcessContextType& context)
    {
    	const auto activeSequence = sequenceBuffer.getRead();
    	if (activeSequence[0]==InactiveSlot)
    	{
    		// nothing in sequence, just bypass
    		if constexpr (ProcessContextType::usesSeparateInputAndOutputBlocks())
    			context.getOutputBlock().copyFrom(context.getInputBlock());

    		return;
    	}

    	auto latency_ = 0;
    	auto processDispatch = [&, chunkSize = context.getInputBlock().getNumSamples()](auto& proc, size_t index){

    		const auto pos = sjf::helpers::functions::utilities::advancePositionInfo(positionInfo, latency_, spec);

    		sjf::optional_calls::setPositionInfo(proc, pos);

    		// Reset if processor was inactive in the previous audio block
    		if (pendingResets[index].exchange(false))
    			proc.reset();

    		proc.process(context);
    		latency_ += sjf::optional_calls::getLatencySamples(proc);
    	};

    	for (auto index : activeSequence)
    	{
    		if (index == InactiveSlot)
    			break;

    		dispatchIndex(index, processDispatch);
    	}

    	latency.store(latency_);
    }

	/** Accesses an individual processor by compile-time index. */
	template <size_t Index> [[nodiscard]] decltype(auto) get() noexcept       { return std::get<Index> (processors); }
	template <size_t Index> [[nodiscard]] decltype(auto) get() const noexcept { return std::get<Index> (processors); }


    /** Updates playhead info across active/inactive processors. */
	void setPositionInfo(const juce::AudioPlayHead::PositionInfo& positionInfo_)
    {
    	positionInfo = positionInfo_;
    }

    /** Returns the sum of latency samples across all processors in the sequence. */
    int getLatencySamples() const
    {
    	return latency.load();
    }

    //==============================================================================
    // NEW PUBLIC SEQUENCE MANIPULATION INTERFACE (Control Thread)
    //==============================================================================

    /** Replaces the active sequence with a completely new order payload.
     *
     *		DO NOT CALL THIS FROM THE AUDIO THREAD
     */
    void setSequence(const SequenceOrder& newOrder)
    {
    	if (newOrder == activeControlSequence)
    		return;

    	if (MessageManager::existsAndIsCurrentThread())
    	{
    		if (stateTree.isValid())
    			stateTree.setProperty(sequencePropertyId, sequenceToVar(newOrder), nullptr);
    		else
    			jassertfalse; // you need to call attachToState()
    	}
    	else if ( auto mm = MessageManager::getInstanceWithoutCreating())
    	{
    		mm->callAsync([newOrder, this, g = std::weak_ptr(guard)](){
    			if (!g.expired() && stateTree.isValid())
    				stateTree.setProperty(sequencePropertyId, sequenceToVar(newOrder), nullptr);
    		});
    	}
    }

    /** Returns the active sequence payload currently stored on the control thread. */
	[[nodiscard]] SequenceOrder getCurrentSequence() const noexcept
	{
		return activeControlSequence;
	}


	void attachToState (juce::ValueTree& parentTree)
	{
    	if (!parentTree.isValid())
    		return;

    	if (apvtsTree.isValid())
    		apvtsTree.removeListener(this);
    	if (stateTree.isValid() && stateTree != apvtsTree)
    		stateTree.removeListener(this);

    	apvtsTree = parentTree;
    	apvtsTree.addListener(this);

		stateTree = parentTree.getOrCreateChildWithName(factoryId+sequenceTreeId, nullptr);



    	if (MessageManager::existsAndIsCurrentThread())
    	{
    		stateTree.addListener(this);
    		publishSequenceUpdate();
    	}
    	else if (auto mm = MessageManager::getInstanceWithoutCreating())
    	{
    		mm->callAsync([this, g = std::weak_ptr(guard)](){
    			if (!g.expired() && stateTree.isValid())
    			{
    				stateTree.addListener(this);
    				publishSequenceUpdate();
    			}
    		});
    	}

    	sjf::helpers::functions::utilities::forEach (processors,[&](auto& proc){
			sjf::optional_calls::attachToState(proc, parentTree);
		});
	}
private:
	//==============================================================================
	// VALUETREE LISTENER OVERRIDES
	//==============================================================================
	void valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged,
								   const juce::Identifier& propertyId) override
	{
		const static auto sequencePropertyId_ = juce::Identifier(sequencePropertyId);
		if (treeWhosePropertyHasChanged == stateTree && propertyId == sequencePropertyId_)
		{
			if (MessageManager::existsAndIsCurrentThread())
				publishSequenceUpdate();
			else
				asyncUpdater.triggerUpdate();
		}
	}

	void valueTreeRedirected(ValueTree& treeWhichHasBeenChanged) override
	{
		if (treeWhichHasBeenChanged == apvtsTree)
			attachToState(treeWhichHasBeenChanged);
	}


	// Helper functions to convert between SequenceOrder and juce::var (Array)
	static juce::var sequenceToVar (const SequenceOrder& order)
	{
		auto ret = juce::Array<juce::var>{};
		for (auto i : order)
			ret.addIfNotAlreadyThere(juce::var{static_cast<int64>(i)});

		return juce::var{ret};
	}

	static SequenceOrder varToSequence (const juce::var& v)
	{
		if (v.isArray())
		{
			return [&v](){
							std::array<bool, NumProcessors> alreadyAdded;
							alreadyAdded.fill(false);
							SequenceOrder ret{};
							ret.fill(InactiveSlot);
							jassert(v.size() == NumProcessors);
							const auto array = *v.getArray();
							for ( auto i = 0ul; i < jmin(static_cast<size_t>(array.size()), NumProcessors); ++i)
							{
								auto processor = static_cast<size_t>(static_cast<int>(array[static_cast<int>(i)]));
								if (!alreadyAdded[processor])
								{
									ret[i] = processor;
									alreadyAdded[processor] = true;
								}
							}
							return ret;
						}();
		}
		else
		{
			jassertfalse;
			return {};
		}
	}

    //==============================================================================
    // INTERNAL DISPATCH AND HELPERS
    //==============================================================================

    /** Compile-time loop helper to bridge runtime uint8_t index to std::get<I>(processors). */
	template <size_t I = 0, typename Func>
	void dispatchIndex(size_t runtimeIndex, Func&& func)
	{
		if constexpr (I < NumProcessors)
		{
			if (runtimeIndex == I)
			{
				func(std::get<I>(processors), I);
				return;
			}

			dispatchIndex<I + 1>(runtimeIndex, std::forward<Func>(func));
		}
	}


    void publishSequenceUpdate()
    {
    	jassert(MessageManager::existsAndIsCurrentThread());
    	if (stateTree.isValid())
    	{
    		if (const auto prop = stateTree.getPropertyPointer(sequencePropertyId))
    		{
    			std::array<bool, NumProcessors> needsReset;
    			needsReset.fill(true);

    			for (auto i = 0ul; i < NumProcessors; ++i)
    			{
    				if (activeControlSequence[i]==InactiveSlot)
    					break;
    				needsReset[activeControlSequence[i]] = false;
    			}
    			for (auto i = 0ul; i < NumProcessors; ++i)
    				pendingResets[i] = needsReset[i];

    			activeControlSequence = varToSequence(*prop);
    			sequenceBuffer.editData([this](SequenceOrder& seq){seq = activeControlSequence;});
    		}
    		else
    		{
    			stateTree.setProperty(sequencePropertyId, sequenceToVar(activeControlSequence), nullptr);
    		}
    	}
    }

    // Invokes createParameters on concrete types (same implementation as ProcessorSequence)
	// Case 1: The configuration is a flat SubFactoryConfig
	template <typename ProcessorType, typename ConfigType>
	requires std::is_same_v<std::decay_t<ConfigType>, processor_sequence::SubFactoryConfig>
	void invokeCreateParameters (ProcessorType& proc, ParameterFactory* parent, ConfigType&& config)
	{
		if (auto sub = proc.createParameters (parent->getID() + config.id,parent->getName() + " " + config.name))
			parent->addChildFactory (std::move (sub));
	}

	// Case 2: The configuration is wrapped in a 'NestedConfig' token
	template <typename ProcessorType, typename... Args>
	void invokeCreateParameters (ProcessorType& proc, ParameterFactory* parent, const processor_sequence::NestedConfig<Args...>& nestedPackage)
	{
		std::apply ([&](auto&&... nestedArgs) {
			if (auto sub = proc.createParameters (std::forward<decltype (nestedArgs)> (nestedArgs)...))
				parent->addChildFactory (std::move (sub));
		}, nestedPackage.args);
	}

    //==============================================================================
    // STORAGE & LOCK-FREE BUFFER
    //==============================================================================

    // All processors stored contiguously in memory
    std::tuple<Processors...> processors;

    // Control-thread shadow copy of the active sequence order
    SequenceOrder activeControlSequence = [&](){
	    SequenceOrder tmp{};
    	std::fill(tmp.begin(), tmp.end(), InactiveSlot);
    	return tmp;
    }();

    // Lock-free triple buffer used to safely publish sequence updates to the audio thread
    SPSCTripleBuffer<SequenceOrder> sequenceBuffer{activeControlSequence};
	std::array<std::atomic<bool>, NumProcessors> pendingResets{};

	juce::ValueTree stateTree, apvtsTree;

	String factoryId{};
	const String sequenceTreeId{"Sequence"}, sequencePropertyId{"Seq"};



	using Callback = std::function<void()>;
	AsyncCallbackInvoker<Callback> asyncUpdater{[this](){
		publishSequenceUpdate();
	}};

	juce::dsp::ProcessSpec spec{};
	juce::AudioPlayHead::PositionInfo positionInfo{};
	std::atomic<int> latency{0};
	std::shared_ptr<int> guard = std::make_shared<int>(42);
};


// template <typename... Processors>
// class DynamicProcessorSequence
// {
// public:
//     static_assert (sizeof...(Processors) > 0, "DynamicProcessorSequence must be instantiated with at least one Processor type!");
//
//     static constexpr size_t MaxProcessors = sizeof...(Processors);
//
//     DynamicProcessorSequence()
//     {
//         [&]<std::size_t... Is>(std::index_sequence<Is...>)
//         {
//             (wrapProcessor<Is>(), ...);
//         }(std::make_index_sequence<MaxProcessors>{});
//
//         for (auto& wrapper : processorWrappers)
//             wrapper->isActive = false;
//     }
//
//     template <typename... Configs>
//     std::unique_ptr<ParameterFactory> createParameters (
//         const juce::String& factoryID,
//         const juce::String& factoryName,
//         Configs&&... subConfigs)
//     {
//         static_assert (sizeof...(Configs) == sizeof...(Processors),
//             "The number of configuration parameters must match the number of processors!");
//
//         auto mainFactory = ParameterFactory::create (factoryID, factoryName);
//
//         if (mainFactory == nullptr)
//         {
//             jassertfalse;
//             return nullptr;
//         }
//
//         auto configTuple = std::forward_as_tuple (std::forward<Configs> (subConfigs)...);
//
//         [&]<std::size_t... Is>(std::index_sequence<Is...>)
//         {
//             (invokeCreateParameters (
//                 std::get<Is> (processors),
//                 mainFactory.get(),
//                 std::forward<std::tuple_element_t<Is, decltype(configTuple)>> (std::get<Is> (configTuple))
//              ), ...);
//         }(std::make_index_sequence<sizeof...(Processors)>{});
//
//         return mainFactory;
//     }
//
//     void prepare (const juce::dsp::ProcessSpec& spec)
//     {
//         prepareSpec = spec;
//         sjf::helpers::functions::utilities::forEach (processors, [&spec](auto& proc) { proc.prepare (spec); });
//     }
//
//     void reset()
//     {
//         sjf::helpers::functions::utilities::forEach (processors,[](auto& proc) { proc.reset(); });
//     }
//
//     template <typename ProcessContextType>
//     void process (const ProcessContextType& context)
//     {
//         updateActiveSequenceIfNeeded();
//
//         const auto* sequence = activeSequence.load(std::memory_order_acquire);
//
//         if (sequence->size() == 0)
//         {
//             if (context.isBypassed)
//                 return;
//
//             if (ProcessContextType::usesSeparateInputAndOutputBlocks())
//                 context.getOutputBlock().copyFrom(context.getInputBlock());
//
//             return;
//         }
//
//         for (size_t i = 0; i < sequence->size(); ++i)
//         {
//             auto* wrapper = (*sequence)[i];
//             if (wrapper && wrapper->isActive)
//                 wrapper->processTyped(context);
//         }
//     }
//
//     template <size_t Index>
//     [[nodiscard]] decltype(auto) get() noexcept
//     {
//         return std::get<Index> (processors);
//     }
//
//     template <size_t Index>
//     [[nodiscard]] decltype(auto) get() const noexcept
//     {
//         return std::get<Index> (processors);
//     }
//
//     void setPositionInfo(const juce::AudioPlayHead::PositionInfo& positionInfo)
//     {
//         sjf::helpers::functions::utilities::forEach (processors,[&](auto& proc){
//             sjf::optional_calls::setPositionInfo(proc, positionInfo);
//         });
//     }
//
//     int getLatencySamples()
//     {
//         auto sum = 0;
//         const auto* sequence = activeSequence.load(std::memory_order_acquire);
//
//         for (size_t i = 0; i < sequence->size(); ++i)
//         {
//             auto* wrapper = (*sequence)[i];
//             if (wrapper && wrapper->isActive)
//                 sum += wrapper->getLatencySamples();
//         }
//
//         return sum;
//     }
//
//     void addProcessor(unsigned long processorID)
//     {
//         jassert(processorID < MaxProcessors);
//         if (processorID >= MaxProcessors)
//             return;
//
//         std::lock_guard<std::mutex> lock(pendingMutex);
//
//         auto& wrapper = processorWrappers[processorID];
//         if (!wrapper->isActive)
//         {
//             wrapper->needsReset = true;
//             wrapper->isActive = true;
//
//             bool found = false;
//             for (size_t i = 0; i < pendingSequence.size(); ++i)
//             {
//                 if (pendingSequence[i] == wrapper.get())
//                 {
//                     found = true;
//                     break;
//                 }
//             }
//
//             if (!found)
//                 pendingSequence.push_back(wrapper.get());
//
//             sequenceNeedsUpdate.store(true, std::memory_order_release);
//         }
//     }
//
//     void removeProcessor(unsigned long processorID)
//     {
//         jassert(processorID < MaxProcessors);
//         if (processorID >= MaxProcessors)
//             return;
//
//         std::lock_guard<std::mutex> lock(pendingMutex);
//
//         auto& wrapper = processorWrappers[processorID];
//         if (wrapper->isActive)
//         {
//             wrapper->isActive = false;
//
//             for (size_t i = 0; i < pendingSequence.size(); ++i)
//             {
//                 if (pendingSequence[i] == wrapper.get())
//                 {
//                     pendingSequence.erase(pendingSequence.begin() + static_cast<long>(i));
//                     break;
//                 }
//             }
//
//             sequenceNeedsUpdate.store(true, std::memory_order_release);
//         }
//     }
//
//     void moveProcessor(unsigned long processorID, size_t newPosition)
//     {
//         jassert(processorID < MaxProcessors);
//         if (processorID >= MaxProcessors)
//             return;
//
//         std::lock_guard<std::mutex> lock(pendingMutex);
//
//         auto& wrapper = processorWrappers[processorID];
//         if (!wrapper->isActive)
//             return;
//
//         size_t currentPos = pendingSequence.size();
//         for (size_t i = 0; i < pendingSequence.size(); ++i)
//         {
//             if (pendingSequence[i] == wrapper.get())
//             {
//                 currentPos = i;
//                 break;
//             }
//         }
//
//         if (currentPos >= pendingSequence.size())
//             return;
//
//         jassert(newPosition < pendingSequence.size());
//         if (newPosition >= pendingSequence.size())
//             return;
//
//         if (currentPos == newPosition)
//             return;
//
//         auto* proc = pendingSequence[currentPos];
//         pendingSequence.erase(pendingSequence.begin() + static_cast<long>(currentPos));
//         pendingSequence.insert(pendingSequence.begin() + static_cast<long>(newPosition), proc);
//
//         sequenceNeedsUpdate.store(true, std::memory_order_release);
//     }
//
//     void swapProcessor(unsigned long oldProcessorID, unsigned long newProcessorID)
//     {
//         jassert(oldProcessorID < MaxProcessors);
//         jassert(newProcessorID < MaxProcessors);
//         if (oldProcessorID >= MaxProcessors || newProcessorID >= MaxProcessors)
//             return;
//
//         std::lock_guard<std::mutex> lock(pendingMutex);
//
//         auto& oldWrapper = processorWrappers[oldProcessorID];
//         auto& newWrapper = processorWrappers[newProcessorID];
//
//         if (!oldWrapper->isActive)
//             return;
//
//         size_t position = pendingSequence.size();
//         for (size_t i = 0; i < pendingSequence.size(); ++i)
//         {
//             if (pendingSequence[i] == oldWrapper.get())
//             {
//                 position = i;
//                 break;
//             }
//         }
//
//         if (position >= pendingSequence.size())
//             return;
//
//         oldWrapper->isActive = false;
//         newWrapper->isActive = true;
//         newWrapper->needsReset = true;
//
//         pendingSequence[position] = newWrapper.get();
//
//         sequenceNeedsUpdate.store(true, std::memory_order_release);
//     }
//
// private:
//     struct ProcessorWrapperBase
//     {
//         virtual ~ProcessorWrapperBase() = default;
//
//         template <typename ProcessContextType>
//         void processTyped(const ProcessContextType& context)
//         {
//             resetIfNeeded();
//             processImpl(context);
//         }
//
//         virtual void resetIfNeeded() = 0;
//         virtual int getLatencySamples() = 0;
//
//         std::atomic<bool> isActive{false};
//         std::atomic<bool> needsReset{false};
//
//     protected:
//         virtual void processImpl(const juce::dsp::ProcessContextReplacing<float>& context) = 0;
//         virtual void processImpl(const juce::dsp::ProcessContextNonReplacing<float>& context) = 0;
//     };
//
//     template <typename ProcessorType>
//     struct ProcessorWrapper : ProcessorWrapperBase
//     {
//         ProcessorWrapper(ProcessorType& proc) : processor(proc) {}
//
//         void resetIfNeeded() override
//         {
//             if (ProcessorWrapperBase::needsReset.exchange(false, std::memory_order_acq_rel))
//                 processor.reset();
//         }
//
//         int getLatencySamples() override
//         {
//             return sjf::optional_calls::getLatencySamples(processor);
//         }
//
//     protected:
//         void processImpl(const juce::dsp::ProcessContextReplacing<float>& context) override
//         {
//             processor.process(context);
//         }
//
//         void processImpl(const juce::dsp::ProcessContextNonReplacing<float>& context) override
//         {
//             processor.process(context);
//         }
//
//
//         ProcessorType& processor;
//     };
//
//     template <size_t Index>
//     void wrapProcessor()
//     {
//         processorWrappers[Index] = std::make_unique<ProcessorWrapper<std::tuple_element_t<Index, decltype(processors)>>>(
//             std::get<Index>(processors)
//         );
//     }
//
//     void updateActiveSequenceIfNeeded()
//     {
//         if (!sequenceNeedsUpdate.load(std::memory_order_acquire))
//             return;
//
//         {
//             std::lock_guard<std::mutex> lock(pendingMutex);
//
//             auto* targetSequence = (activeSequence.load(std::memory_order_relaxed) == &sequenceA) ? &sequenceB : &sequenceA;
//
//             targetSequence->clear();
//             targetSequence->reserve(pendingSequence.size());
//
//             for (auto* wrapper : pendingSequence)
//             {
//                 if (wrapper && wrapper->isActive)
//                     targetSequence->push_back(wrapper);
//             }
//
//             activeSequence.store(targetSequence, std::memory_order_release);
//             sequenceNeedsUpdate.store(false, std::memory_order_release);
//         }
//     }
//
//     template <typename ProcessorType, typename ConfigType>
//     requires std::is_same_v<std::decay_t<ConfigType>, processor_sequence::SubFactoryConfig>
//     void invokeCreateParameters (ProcessorType& proc, ParameterFactory* parent, ConfigType&& config)
//     {
//         if (auto sub = proc.createParameters (parent->getID() + config.id, parent->getName() + " " + config.name))
//             parent->addChildFactory (std::move (sub));
//     }
//
//     template <typename ProcessorType, typename... Args>
//     void invokeCreateParameters (ProcessorType& proc, ParameterFactory* parent, const processor_sequence::NestedConfig<Args...>& nestedPackage)
//     {
//         std::apply ([&](auto&&... nestedArgs) {
//             if (auto sub = proc.createParameters (std::forward<decltype (nestedArgs)> (nestedArgs)...))
//                 parent->addChildFactory (std::move (sub));
//         }, nestedPackage.args);
//     }
//
//     std::tuple<Processors...> processors;
//     std::array<std::unique_ptr<ProcessorWrapperBase>, MaxProcessors> processorWrappers;
//
//     std::vector<ProcessorWrapperBase*> sequenceA;
//     std::vector<ProcessorWrapperBase*> sequenceB;
//     std::atomic<std::vector<ProcessorWrapperBase*>*> activeSequence{&sequenceA};
//
//     std::mutex pendingMutex;
//     std::vector<ProcessorWrapperBase*> pendingSequence;
//     std::atomic<bool> sequenceNeedsUpdate{false};
//
//     juce::dsp::ProcessSpec prepareSpec;
// };



	// template<typename Callback>
	// struct ThreadSafeAsyncUpdater : public Timer
	// {
	// 	explicit ThreadSafeAsyncUpdater(Callback&& c)
	// 	: callback(std::forward<Callback>(c))
	// 	{
	// 		startTimerHz(60);
	// 	}
	//
	// 	~ThreadSafeAsyncUpdater() override
	// 	{
	// 		stopTimer();
	// 	}
	//
	// 	void timerCallback() override
	// 	{
	// 		if (sendUpdate.exchange(false))
	// 			if (auto mm = MessageManager::getInstanceWithoutCreating())
	// 			{
	// 				if (MessageManager::existsAndIsCurrentThread())
	// 					callback();
	// 				else
	// 					mm->callAsync([cb = callback, g = std::weak_ptr<int>(guard)](){
	// 						if (!g.expired())
	// 							cb();
	// 					});
	// 			}
	// 	}
	//
	// 	void triggerUpdate()
	// 	{
	// 		sendUpdate.store(true);
	// 	}
	// private:
	// 	std::shared_ptr<int> guard = std::make_shared<int>(42);
	// 	std::atomic<bool> sendUpdate{false};
	// 	Callback callback;
	// };
}
