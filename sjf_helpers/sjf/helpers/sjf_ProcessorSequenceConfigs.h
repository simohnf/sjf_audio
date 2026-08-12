/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 12/08/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_ParameterFactory.h>

namespace sjf::helpers::processor_sequence
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
     * @tparam ID FactoryID
     * @tparam NAME FactoryName
     * @tparam SubConfigs SubFactoryConfig/NestedConfig for each nested processor
     */
	template <typename ID, typename NAME, typename... SubConfigs>
	struct NestedConfig
	{
		static_assert (std::is_constructible_v<juce::String, ID>,
					   "The first argument of NestedConfig must be constructible as a juce::String (ID).");

		static_assert (std::is_constructible_v<juce::String, NAME>,
					   "The second argument of NestedConfig must be constructible as a juce::String (Name).");

		// Fold directly over the Tail parameter pack!
		static_assert (((std::is_constructible_v<SubFactoryConfig, SubConfigs> ||
						sjf::helpers::functions::utilities::is_instantiation_of<NestedConfig, SubConfigs> ) && ...),
					   "Every argument after ID and Name in NestedConfig must be a SubFactoryConfig or an instantiation of NestedConfig.");

		using TupleType = std::tuple<ID, NAME, SubConfigs...>;
		TupleType args;

		explicit NestedConfig (ID id, NAME name, SubConfigs... subConfigs)
		: args (std::move (id), std::move (name), std::move (subConfigs)...)
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




//DUMMY_PLUGIN_SJF_PROCESSORSEQUENCECONFIGS_H
