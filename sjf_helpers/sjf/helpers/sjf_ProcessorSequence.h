/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */

//
// Created by Simon Fay on 13/07/2026.
//

#pragma once
#include <JuceHeader.h>

#include <sjf/helpers/sjf_ParameterFactory.h>

#include "sjf_HelperFunctions.h"
#include "sjf_OptionalCalls.h"

namespace sjf::helpers
{
namespace processor_sequence
{
    /**
     * @brief Configuration descriptor for standard, non-nested child processors.
     *
     * Used to pass single-level namespaces directly down to individual processors inside
     * the compile-time sequence layout.
     */
    struct SubFactoryConfig
    {
        juce::String id;   /**< The localized ID prefix for the target processor. */
        juce::String name; /**< The localized display label prefix for the target processor. */
    };

    /**
     * @brief A type-safe packaging container used to wrap multiple multi-tiered
     *        arguments for nested sub-processors.
     *
     * Use this structural token when a processor within your sequence is *itself* a compound
     * processor (like another nested `ProcessorSequence` or an `OversamplingWrapper`) that
     * expects complex construction parameters in its `createParameters` overload.
     *
     * @tparam Args Variadic types matching the signature expected by the target child processor.
     */
    template <typename... Args>
    struct NestedConfig
    {
    	static_assert(sizeof...(Args) > 2,
					 "NestedConfig must have at least 2 arguments (ID and Name).");

    	using TupleType = std::tuple<Args...>;

    	using Arg0 = std::tuple_element_t<0, TupleType>;
    	using Arg1 = std::tuple_element_t<1, TupleType>;

    	static_assert(std::is_constructible_v<juce::String, Arg0>,
					  "The first argument of NestedConfig must be constructible as a juce::String (ID).");

    	static_assert(std::is_constructible_v<juce::String, Arg1>,
					  "The second argument of NestedConfig must be constructible as a juce::String (Name).");

    	TupleType args;

        explicit NestedConfig (Args... inputs) : args (std::move (inputs)...)
        {}

    	/** Returns a copy of this NestedConfig with updated ID and Name, preserving all remaining arguments. */
    	[[nodiscard]] auto withNewIdAndName (juce::String newId, juce::String newName) const
        {
        	return std::apply ([&](const auto& /*oldId*/, const auto& /*oldName*/, const auto&... remainingArgs) {
				return NestedConfig<juce::String, juce::String, std::decay_t<decltype(remainingArgs)>...> (
					std::move (newId),
					std::move (newName),
					remainingArgs...
				);
			}, args);
        }

    	/** Returns a copy of this NestedConfig with parent prefixes applied to ID and Name. */
    	[[nodiscard]] auto withPrefixedIdAndName (const juce::String& parentId, const juce::String& parentName) const
        {
        	const auto& currentId   = std::get<0> (args);
        	const auto& currentName = std::get<1> (args);

        	const auto fullId   = parentId + currentId;
        	const auto fullName = parentName.isEmpty() ? currentName : parentName + " " + currentName;

        	return withNewIdAndName (fullId, fullName);
        }
    };



	// Case 1: The configuration is a flat SubFactoryConfig
	template <typename ProcessorType, typename ConfigType>
	requires std::is_same_v<std::decay_t<ConfigType>, processor_sequence::SubFactoryConfig>
	static void invokeCreateParameters (ProcessorType& proc, ParameterFactory* parent, ConfigType&& config)
	{
		if (auto sub = proc.createParameters (parent->getID() + config.id,parent->getName() + " " + config.name))
			parent->addChildFactory (std::move (sub));
	}

	// Case 2: The configuration is wrapped in a 'NestedConfig' token
	template <typename ProcessorType, typename... Args>
	static void invokeCreateParameters (ProcessorType& proc, ParameterFactory* parent, const processor_sequence::NestedConfig<Args...>& nestedPackage)
	{
		const auto prefixedNestedArgs = nestedPackage.withPrefixedIdAndName(parent->getID(), parent->getName());

		std::apply ([&](auto&&... nestedArgs) {
			if (auto sub = proc.createParameters (std::forward<decltype (nestedArgs)> (nestedArgs)...))
				parent->addChildFactory (std::move (sub));
		}, prefixedNestedArgs.args);
	}

}


/**
 * @brief A static, compile-time variadic chain container that sequences multiple execution
 *        processors into a unified DSP pipeline.
 *
 * This class eliminates runtime allocation overheads by packing all underlying modules into a
 * contiguous `std::tuple`. Lifecycle functions (`prepare`, `reset`, `process`, and `setPositionInfo`)
 * are executed sequentially across all nodes using unrolled compile-time loop expansions.
 *
 * It provides a powerful parameter reflection driver (`createParameters`) that matches the
 * sequential type signature of your processors, dynamically assembling an interconnected
 * hierarchy of nested parameter trees for the host DAW.
 *
 * @tparam Processors Variadic list of class types to compose into the serial audio lane.
 *                    Each type must expose standard JUCE-style `prepare`, `reset`, `process`,
 *                    and `createParameters` interfaces.
 */
template <typename... Processors>
class ProcessorSequence
{
public:
    // Enforce that the sequence must contain at least one processor module
    static_assert (sizeof...(Processors) > 0, "ProcessorSequence must be instantiated with at least one Processor type!");



    /**
        Creates a parent ParameterFactory and recursively builds nested sub-factories
        for each processor in the sequence.

        Accepts any combination of SubFactoryConfig objects or nested NestedConfig tokens
        matching the architectural tree of your processors.
    */
    template <typename... Configs>
    std::unique_ptr<ParameterFactory> createParameters (
        const juce::String& factoryID,
        const juce::String& factoryName,
        Configs&&... subConfigs)
    {
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
            (processor_sequence::invokeCreateParameters (
                std::get<Is> (processors),
                mainFactory.get(),
                std::forward<std::tuple_element_t<Is, decltype(configTuple)>> (std::get<Is> (configTuple))
             ), ...);
        }(std::make_index_sequence<sizeof...(Processors)>{});

        return mainFactory;
    }

    //==============================================================================
    void prepare (const juce::dsp::ProcessSpec& spec_)
    {
    	spec = spec_;
	    sjf::helpers::functions::utilities::forEach (processors, [&](auto& proc) { proc.prepare (spec); }); reset();
    }

    void reset()
    {
	    sjf::helpers::functions::utilities::forEach (processors,[](auto& proc) { proc.reset(); });
    }

    template <typename ProcessContextType>
    void process (const ProcessContextType& context)
    {
    	auto latency_ = 0;

	    sjf::helpers::functions::utilities::forEach (processors,[&](auto& proc){
	    	const auto pos = sjf::helpers::functions::utilities::advancePositionInfo(positionInfo, latency_, spec);

	    	sjf::optional_calls::setPositionInfo(proc, pos);

	    	latency_ += sjf::optional_calls::getLatencySamples(proc);
		    proc.process (context);
	    });
    }

    //==============================================================================
    /** Access an individual processor by index at compile-time */
    template <size_t Index> [[nodiscard]] decltype(auto) get() noexcept       { return std::get<Index> (processors); }
    template <size_t Index> [[nodiscard]] decltype(auto) get() const noexcept { return std::get<Index> (processors); }

    void setPositionInfo(const juce::AudioPlayHead::PositionInfo& positionInfo_)
    {
    	positionInfo = positionInfo_;
    }

	int getLatencySamples()
    {
    	auto sum = 0;
    	sjf::helpers::functions::utilities::forEach (processors,[&](auto& proc){ sum += sjf::optional_calls::getLatencySamples(proc); });
    	return sum;
    }


	void attachToState (juce::ValueTree& parentTree)
    {
    	sjf::helpers::functions::utilities::forEach (processors,[&](auto& proc){
    		sjf::optional_calls::attachToState(proc, parentTree);
    	});
    }

private:


    //==============================================================================
    std::tuple<Processors...> processors;

	juce::AudioPlayHead::PositionInfo positionInfo{};
	juce::dsp::ProcessSpec spec{};
};

}