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

    /**
     * @brief Iterates over each element of a std::tuple and applies a callable function to it.
     *
     * ### Example Usage:
     * @code
     * auto myTuple = std::make_tuple(10, 3.14f, std::string("Hello"));
     *
     * // Prints: 10 | 3.14 | Hello |
     * forEach(myTuple, [](const auto& item) {
     *     std::cout << item << " | ";
     * });
     * @endcode
     *
     * @tparam Tuple The std::tuple type (or a type compatible with std::apply).
     * @tparam Callable The type of the callable function or lambda.
     * @param tuple The tuple whose elements will be iterated over.
     * @param func The callable to apply to each element. Must accept any type present in the tuple.
     */
    template <typename Tuple, typename Callable>
    forcedinline void forEach(Tuple&& tuple, Callable&& func)
    {
        std::apply([&]<typename... T0>(T0&&... items) {
            (func(std::forward<T0>(items)), ...);
        }, std::forward<Tuple>(tuple));
    }


    // template parameter pack helpers

    /**
     * @internal
     * @brief Private helper utilities for parameter pack inspection and manipulation.
     *
     * These templates are implementation details and must not be used directly in
     * user code. Use the public aliases instead.
     */
    namespace parameter_pack_helpers
    {
        /**
         * @internal
         * @brief Base case: Evaluates to std::false_type for types that do not match the template.
         */
        template <template <typename...> class Template, typename T>
        struct is_instantiation_of : std::false_type {};

        /**
         * @internal
         * @brief Specialisation: Evaluates to std::true_type when T is an instantiation of Template.
         */
        template <template <typename...> class Template, typename... Args>
        struct is_instantiation_of<Template, Template<Args...>> : std::true_type {};

        /**
         * @internal
         * @brief Helper variable template for is_instantiation_of.
         */
        template <template <typename...> class Template, typename T>
        inline constexpr bool is_instantiation_of_v = is_instantiation_of<Template, T>::value;

        /**
         * @internal
         * @brief Base case: Returns the Default fallback type when the search pack is empty.
         */
        template <template <typename...> class Template, typename Default, typename... Types>
        struct find_instantiation_of
        {
            using type = Default;
        };

        /**
         * @internal
         * @brief Recursive case: Checks the Head type; if it matches, returns it. Otherwise, searches the Tail.
         */
        template <template <typename...> class Template, typename Default, typename Head, typename... Tail>
        struct find_instantiation_of<Template, Default, Head, Tail...>
        {
            using type = std::conditional_t<
                is_instantiation_of_v<Template, std::decay_t<Head>>,
                Head,
                typename find_instantiation_of<Template, Default, Tail...>::type
            >;
        };
    }

    /**
     * @brief Checks if a specific target type exists within a parameter pack.
     *
     * ### Example Usage:
     * @code
     * struct Mono {};
     * struct Stereo {};
     * struct Surround {};
     *
     * // Evaluates to true
     * constexpr bool isStereoSupported = configurationAvailable<Stereo, Mono, Stereo>;
     *
     * // Evaluates to false
     * constexpr bool isSurroundSupported = configurationAvailable<Surround, Mono, Stereo>;
     * @endcode
     *
     * @tparam TargetConfig The exact type to search for in the pack.
     * @tparam Configurations The parameter pack of types to inspect.
     */
    template <typename TargetConfig, typename... Configurations>
    forcedinline constexpr bool configurationAvailable = (std::is_same_v<Configurations, TargetConfig> || ...);

    /**
     * @brief Compile-time trait that checks if a parameter pack contains an instantiation of a specific class template.
     *
     * ### Example Usage:
     * @code
     * template <typename T> struct Filter {};
     * template <typename T> struct Gain {};
     *
     * // Evaluates to true
     * constexpr bool test1 = has_any_instantiation<Filter, Gain<float>, Filter<int>>;
     *
     * // Evaluates to false
     * constexpr bool test2 = has_any_instantiation<Filter, Gain<float>, int>;
     * @endcode
     *
     * @tparam TargetTemplate The class template to search for in the pack.
     * @tparam Configurations The parameter pack of types to inspect.
     */
    template <template <typename...> class TargetTemplate, typename... Configurations>
    inline constexpr bool has_any_instantiation = (parameter_pack_helpers::is_instantiation_of_v<TargetTemplate, std::decay_t<Configurations>> || ...);




    /**
     * @brief Search helper to extract a matching instantiation of a class template from a parameter pack.
     *
     * ### Example Usage:
     * @code
     * template <typename... State> struct LfoState {};
     * template <typename T> struct Gain {};
     * struct Dummy {};
     *
     * using PackA = find_instantiation_of_t<LfoState, Dummy, Gain<float>, LfoState<int, double>>;
     * // PackA resolves to: LfoState<int, double>
     *
     * using PackB = find_instantiation_of_t<LfoState, Dummy, Gain<float>>;
     * // PackB resolves to: Dummy (not found fallback)
     * @endcode
     *
     * @tparam Template The target class template (which itself accepts a parameter pack) to look for.
     * @tparam Default The fallback type to return if no matching instantiation is found in the pack.
     * @tparam Types The parameter pack of types to search through.
     */
    template <template <typename...> class Template, typename Default, typename... Types>
    using find_instantiation_of_t = typename parameter_pack_helpers::find_instantiation_of<Template, Default, Types...>::type;


    /**
     * @brief A lightweight placeholder type used as a default fallback.
     *
     * This empty struct is designed to serve as the fallback type for template search traits
     * like `find_instantiation_of_t`. By checking if the resolved type is `DummyStruct`,
     * you can determine at compile time whether a target template instantiation was missing
     * from a parameter pack.
     *
     * ### Example Usage:
     * @code
     * template <typename T> struct MyConfig {};
     * struct UnsupportedType {};
     *
     * using Resolved = find_instantiation_of_t<MyConfig, DummyStruct, UnsupportedType>;
     *
     * // Compile-time check to see if MyConfig was found
     * constexpr bool found = !std::is_same_v<Resolved, DummyStruct>;
     * static_assert(!found, "MyConfig was not provided in the configurations pack!");
     * @endcode
     */
    struct DummyStruct{};




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

