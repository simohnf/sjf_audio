//
// Created by Simon Fay on 13/07/2026.
//

#pragma once
#include <JuceHeader.h>

#include <sjf/helpers/sjf_ParameterFactory.h>

namespace sjf::helpers
{

    namespace processor_sequence
    {
        /** Simple configuration structure for flat, non-nested processors */
        struct SubFactoryConfig
        {
            juce::String id;
            juce::String name;
        };

        /** A structural token that cleanly marks and holds nested configurations */
        template <typename... Args>
        struct NestedConfig
        {
            std::tuple<Args...> args;
            NestedConfig (Args... inputs) : args (std::move (inputs)...) {}
        };
    }

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

        // Unpack the processors and configurations in tandem using an index sequence
        createParametersInternal (mainFactory.get(),
                                  std::make_index_sequence<sizeof...(Processors)>{},
                                  std::forward<Configs> (subConfigs)...);

        return mainFactory;
    }

    //==============================================================================
    /**
        Executes a callable utility on each processor in strict sequential order.
        The lambda should take a single 'auto&' argument.
    */
    template <typename Callable>
    void forEach (Callable&& callable)
    {
        std::apply ([&callable](auto&... proc) {
            (callable (proc), ...);
        }, processors);
    }

    /** Const overload for read-only operations */
    template <typename Callable>
    void forEach (Callable&& callable) const
    {
        std::apply ([&callable](const auto&... proc) {
            (callable (proc), ...);
        }, processors);
    }

    //==============================================================================
    void prepare (const juce::dsp::ProcessSpec& spec)   { forEach ([&spec](auto& proc) { proc.prepare (spec); }); }
    void reset()                                        { forEach ([](auto& proc) { proc.reset(); }); }

    template <typename ProcessContextType>
    void process (const ProcessContextType& context)    { forEach ([&context](auto& proc) { proc.process (context); }); }

    //==============================================================================
    /** Access an individual processor by index at compile-time */
    template <size_t Index> [[nodiscard]] decltype(auto) get() noexcept       { return std::get<Index> (processors); }
    template <size_t Index> [[nodiscard]] decltype(auto) get() const noexcept { return std::get<Index> (processors); }

private:
    /** Internal dispatcher that pairs tuple indices with variadic function arguments */
    template <size_t... Indices, typename... Configs>
    void createParametersInternal (ParameterFactory* mainFactory,
                                   std::index_sequence<Indices...>,
                                   Configs&&... subConfigs)
    {
        // C++17 fold expression expanding elements in parallel
        (..., invokeCreateParameters (std::get<Indices> (processors),
                                     mainFactory,
                                     std::forward<Configs> (subConfigs)));
    }

    /** Case 1: The configuration is a flat SubFactoryConfig */
    template <typename ProcessorType, typename ConfigType>
    std::enable_if_t<std::is_same_v<std::decay_t<ConfigType>, processor_sequence::SubFactoryConfig>>
    invokeCreateParameters (ProcessorType& proc, ParameterFactory* parent, ConfigType&& config)
    {
        if (auto sub = proc.createParameters (config.id, config.name))
            parent->addChildFactory (std::move (sub));
    }

    /** Case 2: The configuration is wrapped in a 'NestedConfig' token */
    template <typename ProcessorType, typename... Args>
    void invokeCreateParameters (ProcessorType& proc, ParameterFactory* parent, const processor_sequence::NestedConfig<Args...>& nestedPackage)
    {
        // Unpack the nested elements as multi-argument parameters to the sub-processor
        std::apply ([&](auto&&... nestedArgs) {
            if (auto sub = proc.createParameters (std::forward<decltype (nestedArgs)> (nestedArgs)...))
                parent->addChildFactory (std::move (sub));
        }, nestedPackage.args);
    }

    //==============================================================================
    std::tuple<Processors...> processors;
};

}