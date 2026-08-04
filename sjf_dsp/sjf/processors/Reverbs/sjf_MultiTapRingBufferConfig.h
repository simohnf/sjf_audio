/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 04/08/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_DelayLine.h>

namespace sjf::dsp::ringbuffer_config
{
    enum class ProcessType
    {
        AllPass,
        Delay,
        FBComb,
        LowPass,
    	LPFBComb,
    };

    struct AllPass
    {
        static constexpr auto type = ProcessType::AllPass;

        template<sjf::interpolation::InterpolatorTypes InterpType>
        static forcedinline float process(sjf::helpers::MultiTapRingBuffer& delayLine, const float input,
										  const float offset, const float length, const float feedback) noexcept
        {
            return delayLine.applyAllPass<InterpType>(input, offset, length, feedback);
        }
    };

    struct Delay
    {
        static constexpr auto type = ProcessType::Delay;

        template<sjf::interpolation::InterpolatorTypes InterpType>
        static forcedinline float process(sjf::helpers::MultiTapRingBuffer& delayLine, const float input,
        								  const float offset, const float length) noexcept
        {
            return delayLine.applyDelay<InterpType>(input, offset, length);
        }
    };

    struct FBComb
    {
        static constexpr auto type = ProcessType::FBComb;

        template<sjf::interpolation::InterpolatorTypes InterpType>
        static forcedinline float process(sjf::helpers::MultiTapRingBuffer& delayLine, const float input,
										  const float offset, const float length, const float feedback) noexcept
        {
            return delayLine.applyDelay<InterpType>(input, offset, length, feedback);
        }
    };

    struct LowPass
    {
        static constexpr auto type = ProcessType::LowPass;

        template<sjf::interpolation::InterpolatorTypes InterpType>
        static forcedinline float process(sjf::helpers::MultiTapRingBuffer& delayLine, const float input,
										  const float offset, const float alpha) noexcept
        {
            return delayLine.applyLowPass(input, static_cast<size_t>(juce::roundToInt(offset)), alpha);
        }
    };


    struct LPFBComb
    {
        static constexpr auto type = ProcessType::LPFBComb;

        template<sjf::interpolation::InterpolatorTypes InterpType>
        static forcedinline float process(sjf::helpers::MultiTapRingBuffer& delayLine, const float input, const float offset,
        									const float length, const float feedback, const float alpha) noexcept
        {
            return delayLine.applyLPFBComb(input, offset, length, feedback, alpha);
        }
    };

    template<typename... Processes>
    struct StageConfig
    {
        static constexpr size_t numProcesses = sizeof...(Processes);
        using ProcessesTuple = std::tuple<Processes...>;

        static constexpr std::array<ProcessType, numProcesses> processTypes = { Processes::type... };
    };
}



//DUMMY_PLUGIN_SJF_MULTITAPRINGBUFFERCONFIG_H
