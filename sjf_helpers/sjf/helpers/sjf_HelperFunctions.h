//
// Created by Simon Fay on 15/07/2026.
//
#pragma once
#include <JuceHeader.h>

#include "sjf_Waveshapers.h"

namespace sjf::helpers::functions
{
namespace utilities
{
    forcedinline float floor( const float value )
    {
        return static_cast<float>(static_cast<int>( value ) );
    }


    template <typename Tuple, typename Callable>
    forcedinline void forEach(Tuple&& tuple, Callable&& func)
    {
        std::apply([&]<typename... T0>(T0&&... items) {
            (func(std::forward<T0>(items)), ...);
        }, std::forward<Tuple>(tuple));
    }


    // template parameter pack helpers

    struct DummyStruct{};

    template <typename TargetConfig, typename... Configurations>
    forcedinline constexpr bool configurationAvailable = (std::is_same_v<Configurations, TargetConfig> || ...);

    // 1. Base case: By default, a type T is NOT an instantiation of the template
    template <template <typename...> class Template, typename T>
    struct is_instantiation_of : std::false_type {};

    // 2. Specialization: If the type matches the structure Template<Args...>, it IS an instantiation
    template <template <typename...> class Template, typename... Args>
    struct is_instantiation_of<Template, Template<Args...>> : std::true_type {};

    // Helper alias for convenience (similar to std::is_same_v)
    template <template <typename...> class Template, typename T>
    inline constexpr bool is_instantiation_of_v = is_instantiation_of<Template, T>::value;

    template <template <typename...> class TargetTemplate, typename... Configurations>
    inline constexpr bool has_any_instantiation = (is_instantiation_of_v<TargetTemplate, std::decay_t<Configurations>> || ...);

    // 1. Base case: If the pack is empty, return the Default type
    template <template <typename...> class Template, typename Default, typename... Types>
    struct find_instantiation_of
    {
        using type = Default;
    };

    // 2. Recursive case: Check the Head of the pack; if it matches, return it. Otherwise, search the Tail.
    template <template <typename...> class Template, typename Default, typename Head, typename... Tail>
    struct find_instantiation_of<Template, Default, Head, Tail...>
    {
        using type = std::conditional_t<
            is_instantiation_of_v<Template, std::decay_t<Head>>,
            Head,
            typename find_instantiation_of<Template, Default, Tail...>::type
        >;
    };

    // Helper alias for clean syntax (yields the actual type directly)
    template <template <typename...> class Template, typename Default, typename... Types>
    using find_instantiation_of_t = typename find_instantiation_of<Template, Default, Types...>::type;

}

namespace waveforms
{
    /**
     * Wraps input phase to 0-->1. Not a waveform, but useful to avoid retyping.
     * @param phase value must be >= 0
     * @return phase 0-->1
     */

    forcedinline float wrapPhase( const float phase )
    {
        jassert ( phase >= 0 );
        return phase - utilities::floor(phase );
    }

    /**
     * Wraps input phase to 0-->1. Not a waveform, but useful to avoid retyping.
     * @param phase value must be >= 0
     * @param shift amount to shift the phase by. must be >=0
     * @return phase 0-->1
     */

    forcedinline float wrapPhase( float phase, const float shift )
    {
        jassert ( shift >= 0 );
        jassert ( phase >= 0 );
        phase += shift;
        return phase - utilities::floor(phase);
    }

    /**
     * Basic sawtooth waveform
     *
     * @param phase input phase 0-->1
     * @param startingPoint changes the relationship between phase input and output of the wave. The default 0.5 means a phase of 0 outputs 0
     * @return sawtooth wave -1-->1
     */

    forcedinline float getSaw( float phase, const float startingPoint = 0.5f )
    {
        phase += startingPoint;
        phase -= utilities::floor( phase );
        phase *= 2.0f;
        phase -= 1.0f;
        jassert( phase >= -1.0f );
        jassert( phase <= 1.0f );
        return phase;
    }

    /**
     * Basic sawtooth waveform
     *
     * @param phase input phase 0-->1
     * @param startingPoint changes the relationship between phase input and output of the wave. The default 0.5 means a phase of 0 outputs 0
     * @return sawtooth wave 1-->-1
     */

    forcedinline float getSawDescending(const float phase, const float startingPoint = 0.5 )
    {
        return getSaw(1.0f - phase, startingPoint);
    }

    /**
     *  WARNING: Due to inaccuracies in FastMathApproximations this may return values that are slightly above 1 or below minus 1!!!!
     *
     * @param phase input phase 0-->1
     * @return sin( 2 * pi * phase )
     */

    forcedinline float getSin( const float phase )
    {
        return juce::dsp::FastMathApproximations::sin( getSaw(phase) * juce::MathConstants<float>::pi);
    }

    /**
     *  WARNING: Due to inaccuracies in FastMathApproximations this may return values that are slightly above 1 or below minus 1!!!!
     *
     * @param phase input phase 0-->1
     * @return cos( 2 * pi * phase )
     */

    forcedinline float getCos( const float phase )
    {
        return juce::dsp::FastMathApproximations::cos( getSaw(phase) * juce::MathConstants<float>::pi);
    }

    /**
     * Basic triangle waveform (no duty cycle parameter...)
     *
     * @param phase input phase 0-->1
     * @return triangle wave between -1-->1
     */

    forcedinline float getTriangle( const float phase ) // no duty cycle .... need to add that!!!
    {
        return abs( phase - utilities::floor(phase + 0.75f)  + 0.25f ) * 4.0f- 1.0f;
    }

    /**
     * Square waveform but with a slope between the max and min values.
     *
     * @param phase input phase 0-->1
     * @param squareness the higher this value the shorter the slope, i.e. the more square the resulting waveform
     * @return -1 or 1, with ramp between values
     */

    forcedinline float getSlopedSquare( const float phase, const float squareness )
    {
        jassert( squareness >= 0 );
        const auto p = getTriangle(phase) * (1 + squareness);
        return Waveshapers::Clippers::hard(p);
    }

    /**
     * Square waveform but with a rounded slope between the max and min values.
     *
     * @param phase input phase 0-->1
     * @param squareness the higher this value the shorter the slope, i.e. the more square the resulting waveform
     * @return -1 or 1, with ramp between values
     */

    forcedinline float getRoundedSquare( const float phase, const float squareness )
    {
        return juce::dsp::FastMathApproximations::cos( 0.5f*(getSlopedSquare(phase, squareness) + 1.0f) * MathConstants<float>::pi);
    }

    /**
     * Basic square waveform. If input phase is below 0.5 returns -1, if equal to or above 0.5 returns 1
     *
     * @param phase input phase 0-->1
     * @return -1 or 1
     */
    forcedinline float getSquare( const float phase )
    {
        return phase < 0.5 ? - 1 : 1;
    }

    /**
     * Calculate the result of a triangle wave raised to the power of 2, but with polarity retained!
     * @param phase input phase 0-->1
     * @return -1 <--> 1
     */

    forcedinline float getExponential2( const float phase )
    {
        const auto tri = getTriangle(phase );
        const auto sign = std::signbit(tri) ? -1.0f : 1.0f;
        return sign*tri*tri;
    }

    /**
     * Calculate the result of a triangle wave raised to the power of 3
     * @param phase input phase 0-->1
     * @return -1 <--> 1
     */

    forcedinline float getExponential3( const float phase )
    {
        const auto tri = getTriangle(phase );
        return tri*tri*tri;
    }

    /**
     * Calculate the result of a sawtoothwave multiplied by itself and then scaled down to between -1 --> 1
     * @param phase input phase 0-->1
     * @param offset offsets the phase, the default 0.25 is so that this waveform hits its peaks and troughs at the same phase input as the sin calculation...
     * @return -1 <--> 1
     */
    forcedinline float getExponentialSweep( const float phase, const float offset = 0.25f )
    {
        jassert( phase + offset >= 0);
        const auto saw = getSaw( phase + offset );
        return 2.0f*saw*saw - 1.0f;
    }

    /**
     * Calculate the result of a sine wave, scaled to between 0-->1, multiplied by itself and then scaled down to between -1 --> 1
     * @param phase input phase 0-->1
     * @return -1 <--> 1
     */
    forcedinline float getExponentialSweep2( const float phase )
    {
        const auto sin = getSin( phase ) *0.5f + 0.5f;
        return 2.0f*sin*sin - 1.0f;
    }

    /**
     * A waveform that can be used to shift between a sine and triangle wave
     * @param phase input phse 0-->1
     * @param triangleness at three the output will be a sinewave (or there abouts), the higher this value the more triangle-like the output
     * @return -1 <--> 1
     */
    forcedinline float getRoundedTriangle( const float phase, const size_t triangleness )
    {
        jassert( triangleness >= 3 );
        const auto tri = getTriangle(phase);
        return Waveshapers::Clippers::soft( tri, triangleness, true );
    }

    /**
     * Calculates the absolute maximum result using the fast maths approximation of sin/cos above. SHould be 1, but isn't for float!
     * @param count the number of values to check
     * @return the maximum value calculated
     *
     * see @getCos @getSin
     */
    forcedinline float calculateSinCosError(const size_t count = 1000000)
    {
        float max = 1.0f;
        for( auto i = 0ul; i < count; ++i )
        {
            const auto frac = ((static_cast<float>(i) / static_cast<float>(count)) -0.5f)* 0.001f;
            const auto posCos = wrapPhase(1.0f + frac);
            const auto cos = abs(getCos(posCos));
            if( cos > max )
                max = cos;

            const auto posSin = wrapPhase(0.25f + frac);
            const auto sin =abs( getSin(posSin));
            if( sin > max )
                max = sin;
        }
        return max;
    }


    forcedinline float getHannWindow(const float phase)
    {
        return getCos(phase) * 0.5f + 0.5f;
    }
}




}

