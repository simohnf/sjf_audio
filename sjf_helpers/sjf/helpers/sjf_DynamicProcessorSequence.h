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
namespace dynamic_processor_sequence
{
	namespace ids
	{
		static const Identifier sequenceTreeId{"Sequence"};
		static const Identifier sequencePropertyId{"Seq"};
	}
	// Sentinel value indicating an unused/empty slot in the sequence
	static constexpr size_t InactiveSlot = std::numeric_limits<int>::max();
}
template <typename... Processors>
class DynamicProcessorSequence : private juce::ValueTree::Listener
{
public:
    static_assert(sizeof...(Processors) > 0,
                  "DynamicProcessorSequence must be instantiated with at least one Processor type!");

    static constexpr size_t NumProcessors = sizeof...(Processors);

	static constexpr auto InactiveSlot = dynamic_processor_sequence::InactiveSlot;

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

    	auto mainFactory = ParameterFactory::createDynamicProcessorSequence (factoryID, factoryName, NumProcessors);

    	if (mainFactory == nullptr)
    	{
    		jassertfalse;
    		return nullptr;
    	}

    	auto configTuple = std::forward_as_tuple (std::forward<Configs> (subConfigs)...);

    	[&]<std::size_t... Is>(std::index_sequence<Is...>)
		{
			(processor_sequence::invokeCreateParameters (
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
    			stateTree.setProperty(dynamic_processor_sequence::ids::sequencePropertyId, sequenceToVar(newOrder), nullptr);
    		else
    			jassertfalse; // you need to call attachToState()
    	}
    	else if ( auto mm = MessageManager::getInstanceWithoutCreating())
    	{
    		mm->callAsync([newOrder, this, g = std::weak_ptr(guard)](){
    			if (!g.expired() && stateTree.isValid())
    				stateTree.setProperty(dynamic_processor_sequence::ids::sequencePropertyId, sequenceToVar(newOrder), nullptr);
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

		stateTree = parentTree.getOrCreateChildWithName(factoryId+dynamic_processor_sequence::ids::sequenceTreeId, nullptr);



    	if (MessageManager::existsAndIsCurrentThread())
    	{
    		apvtsTree.addListener(this);
    		publishSequenceUpdate();
    	}
    	else if (auto mm = MessageManager::getInstanceWithoutCreating())
    	{
    		mm->callAsync([this, g = std::weak_ptr(guard)](){
    			if (!g.expired() && apvtsTree.isValid() && stateTree.isValid())
    			{
    				apvtsTree.addListener(this);
    				publishSequenceUpdate();
    			}
    		});
    	}

    	sjf::helpers::functions::utilities::forEach (processors,[&](auto& proc){
			sjf::optional_calls::attachToState(proc, parentTree);
		});
	}

	// Helper functions to convert between SequenceOrder and juce::var (Array)
	static juce::var sequenceToVar (const SequenceOrder& order)
    {
    	auto ret = juce::StringArray{};
    	for (auto i : order)
    	{
    		if (i == InactiveSlot  || ret.contains(static_cast<juce::String>(i)))
    			break;

    		ret.add(static_cast<juce::String>(i));
    	}

    	while (ret.size() < static_cast<int>(NumProcessors))
    		ret.add(juce::var{static_cast<juce::String>(InactiveSlot)});

    	return juce::var{ret.joinIntoString("/")};
    }

	static SequenceOrder varToSequence (const juce::var& v)
    {
    	if (v.isString())
    	{
    		return [&v](){
    			std::array<bool, NumProcessors> alreadyAdded;
    			alreadyAdded.fill(false);
    			SequenceOrder ret{};
    			ret.fill(InactiveSlot);
    			const auto strArr = StringArray::fromTokens(v.toString(), "/", "");
    			for ( auto i = 0ul; i < jmin(static_cast<size_t>(strArr.size()), NumProcessors); ++i)
    			{
    				auto processor = static_cast<size_t>(strArr[static_cast<int>(i)].getIntValue());
    				if (processor == InactiveSlot)
    					break;

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

	static std::vector<size_t> varToVector (const juce::var& v)
    {
    	if (v.isArray())
    	{
    		return [&v](){
    			std::array<bool, NumProcessors> alreadyAdded;
    			alreadyAdded.fill(false);
    			std::vector<size_t> ret{};
    			ret.reserve(NumProcessors);
    			jassert(v.size() == NumProcessors);
    			const auto array = *v.getArray();
    			for ( auto i = 0ul; i < jmin(static_cast<size_t>(array.size()), NumProcessors); ++i)
    			{
    				auto processor = static_cast<size_t>(static_cast<int>(array[static_cast<int>(i)]));
    				if (processor == InactiveSlot)
    					break;

    				if (!alreadyAdded[processor])
    				{
    					ret.push_back(processor);
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

private:
	//==============================================================================
	// VALUETREE LISTENER OVERRIDES
	//==============================================================================
	void valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged,
								   const juce::Identifier& propertyId) override
	{
		if (treeWhosePropertyHasChanged == stateTree && propertyId == dynamic_processor_sequence::ids::sequencePropertyId)
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
    		if (const auto prop = stateTree.getPropertyPointer(dynamic_processor_sequence::ids::sequencePropertyId))
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
    			stateTree.setProperty(dynamic_processor_sequence::ids::sequencePropertyId, sequenceToVar(activeControlSequence), nullptr);
    		}
    	}
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



	using Callback = std::function<void()>;
	AsyncCallbackInvoker<Callback> asyncUpdater{[this](){
		publishSequenceUpdate();
	}};

	juce::dsp::ProcessSpec spec{};
	juce::AudioPlayHead::PositionInfo positionInfo{};
	std::atomic<int> latency{0};
	std::shared_ptr<int> guard = std::make_shared<int>(42);
};
}
