/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */

//
// Created by Simon Fay on 15/07/2026.
//
#pragma once
#include <JuceHeader.h>
namespace sjf::optional_calls
{

/**
 * @brief Internal SFINAE implementation details for compile-time method checking.
 *
 * @warning The contents of this namespace are architectural implementation details
 *          and are **not intended for direct use by user code**. These functions
 *          rely on specific template overload priorities that are safely managed by
 *          the public API wrappers below.
 */
namespace internal
{
    /** @brief Compiles and executes the member function call if the target processor supports it. */
    template <typename T>
    auto setPositionInfo(T& processor, const Optional<juce::AudioPlayHead::PositionInfo>& info, int)
    -> decltype(processor.setPositionInfo(info), void())
    {
        processor.setPositionInfo(info);
    }

    /** @brief Fallback pass-through that silences compilation errors if the target processor lacks the method. */
    template <typename T>
    void setPositionInfo(T&, const Optional<juce::AudioPlayHead::PositionInfo>&, long)
    {
        // Do nothing
    }
}

/**
 * @brief Template utility that conditionally routes host timing metadata into a target
 *        processor without causing compile errors if the method is missing.
 *
 * This wrapper acts as a compile-time safety boundary. It allows a container class
 * (like `ProcessorSequence`) to push playhead changes down its child nodes
 * even if some of those nodes are simple, static components (like filters or gain utilities)
 * that have no need for timeline positions or BPM sync.
 *
 * @note To opt-in to receiving playhead data, the child processor `T` must implement
 *       the following exact public member function signature:
 *       @code
 *       void setPositionInfo (const Optional<juce::AudioPlayHead::PositionInfo>& positionInfo);
 *       @endcode
 *
 * @tparam T The type of the audio processor.
 * @param processor The active processor block instance.
 * @param info An optional layout containing the host's playhead state details.
 */
template <typename T>
auto setPositionInfo(T& processor, const Optional<juce::AudioPlayHead::PositionInfo>& info)
{
    optional_calls::internal::setPositionInfo(processor, info, 0);
}

}