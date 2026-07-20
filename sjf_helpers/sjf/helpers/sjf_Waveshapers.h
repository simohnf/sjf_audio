/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */

/*
  ==============================================================================

    Waveshapers.h
    Created: 12/03/2025
    Author:  Simon Fay

  ==============================================================================
*/

#pragma once


#include <JuceHeader.h>

namespace sjf::helpers {

/**
 * A variety of NON-BANDLIMITED memoryless waveshapers
 *     Where possible branchless algorithms have been chosen
 *
 *     See https://en.wikipedia.org/wiki/Sigmoid_function
 */


struct Waveshapers
{

    struct Clippers
    {
        /**
       * Simple hard clipping with optional clipping point
       */
        static float hard( const float value, const float clippingPoint = 1.0f )
        {
            return 0.5f * ( abs(value+clippingPoint) - abs(value-clippingPoint) );
        }

        /**
         * Based on the standard cubic soft clipping algorithm, but modified to allow different clipping shapes, and optional normalisation
         */
        static float soft( float value, const size_t clipType = 3, const bool shouldNormalise = false )
        {

            jassert( clipType >= 3 ); // must be >= 3 or weird things happen
            value = Clippers::hard( value);
            const auto sign = value < 0.0f ? -1.0f : 1.0f;
            const auto raised = std::pow(value, static_cast<float>(clipType));
            const auto clipped = value - (sign*abs(raised/static_cast<float>(clipType)));
            return clipped * (shouldNormalise ? static_cast<float>(clipType)/(static_cast<float>(clipType)-1.0f) : 1.0f);
        }

        /**
         * FastMastApproximations::Tanh based clipper (there's a hard clipper in the signal path so the inaccurate range of the calculation isn't hit)
         *
         * You can specify a scale factor which will determine the upper limit of the output
         *      i.e.  if scale == 1, the output will approach but never equal 1
         *            if scale == 5, the output will approach but never equal 5
         *            etc.
         */
        template<unsigned scale = 1>
        static float tanh( float value )
        {
            if constexpr (scale == 1 || scale == 0)
            {
                value = Clippers::hard( value, 5.0f );
                return juce::dsp::FastMathApproximations::tanh( value );
            }
            constexpr auto scaleUp = static_cast<float>(scale);
            constexpr auto scaleDown = 1.0f /scaleUp;

            value = Clippers::hard( scaleDown * value, 5.0f );
            return scaleUp * juce::dsp::FastMathApproximations::tanh( value );
        }

        /**
         * Schetzen formula Zölzer DAFX p.124
         */
        static float schetzen( float value )
        {
            constexpr auto oneThird = 1.0f/3.0f;
            constexpr  auto twoThirds = 2.0f * oneThird;
            const auto sign = value < 0.0f ? -1.0f : 1.0f;
            value = std::abs( value );
            if (value < oneThird)
                value  *= 2.0f;
            else if (value < twoThirds)
                value = 3.0f - pow( 2 - 3*value, 3.0f)/3.0f;
            else
                value = 1;
            return value * sign;
        }
    };

    // SIGMOID FUNCTIONS
    struct Sigmoids
    {
        static float twoOverPiArcTan( const float input )
        {
            static constexpr float twoOverPi = 2.0f / juce::MathConstants<float>::pi;
            static constexpr float piOverTwo = juce::MathConstants<float>::pi / 2.0f;
            return twoOverPi * std::atan( input * piOverTwo );
        }

        
        static float gudermannian( const float input )
        {
            return 2.0f * std::atan( std::tanh( input * 0.5f) );
        }

        /**
         * Same as @gudermannian but using FastMathApproximation for tanh
         */
        static float gudermannian2( const float input )
        {
            return 2.0f * std::atan( juce::dsp::FastMathApproximations::tanh( input * 0.5f) );
        }

        
        static float xOverOnePlusAbsX( const float input )
        {
            return input / ( 1 + abs( input ) );
        }

        
        static float xOverYthRootOf1PlusXpowY( const float input, const float power = 2 )
        {
            return input / std::pow( 1.0f + std::pow(input, power), 1.0f/power );
        }

        /**
         * This sigmoid can pass the -1 --> 1 limit, so probably best to a hard clipper afterwards
         */
        
        static float atanSXOverAtanS( const float input, const float S = 2 )
        {
            return std::atan( S * input ) / std::atan( S );
        }
    };

    struct Wavefolders
    {
    public:
        
        static float sin( const float input )
        {
            return juce::dsp::FastMathApproximations::sin( tri(input) );
        }

        
        static float tri( const float input )
        {
            static constexpr float oneOverTwoPi = 1 / (2*juce::MathConstants<float>::pi);
            auto x = oneOverTwoPi * input;
            return abs( x - floor(x + 0.75f)  + 0.25f ) * 4.0f- 1.0f;

        }

        /**
         * Sin fold applied to negative portion, tri fold applied to positive portion
         */
        
        static float dual( const float input ) // from pigments
        {
            return input < 0 ? abs( Wavefolders::sin( input ))*-1.0f : abs( Wavefolders::tri( input ) );
        }

    };

    struct Misc
    {
        /**
         * Model of bucket brigade nonlinearity, modified from: https://www.dafx.de/paper-archive/2010/DAFx10/RaffelSmith_DAFx10_P42.pdf
         * NOTE: Even though there is no dc offset when the input is 0, you still probably want to follow this with a dc blocker
         */
        
        static float bucketBrigade( float x, const float alpha = 1.0f/8.0f, const float beta = 1.0f/18.0f)
        {
            x = Clippers::hard(x);
            const auto x2 = x*x;
            const auto x3 = x2*x;
            return x - alpha*x2 - beta*x3 + alpha*abs(x); // I've modified this from the DAFX paper so that the dc offset is 0 at 0 input --> it was causing a nasty click on load
        }


        /**
         * Distortion Zölzer DAFX p.128
         */
        
        static float distortion( float value )
        {
            value = Clippers::hard( value, 4.0f );
            return (value < 0.0f ? -1 : 1) * (1.0f - juce::dsp::FastMathApproximations::exp(-abs(value)) );

        }
    };


    struct Helpers
    {
        /**
         * Calculates the Rms of a function over a period of samples ranging from 0-->1
         * @return rms result
         */
        template<typename Function, int NumPoints = 1000>
        static float calculateRMS( const Function&& f )
        {
            auto sum = 0.0f;
            for( auto i = 0; i < NumPoints; ++i )
            {
                const auto res = f( static_cast<float>(i)/static_cast<float>(NumPoints) );
                sum += res*res;
            }
            const auto a = sum/static_cast<float>(NumPoints);
            const auto rms = std::sqrt(a);
            return rms;
        }

        /**
         * Calculates the absolute Average of a function over a period of samples ranging from 0-->1
         * @return rms result
         */
        template<typename Function, int NumPoints = 1000>
        static float calculateAVG( const Function&& f )
        {
            auto sum = 0.0f;
            for( auto i = 0; i < NumPoints; ++i )
            {
                const auto res = f( static_cast<float>(i)/static_cast<float>(NumPoints) );
                sum += std::abs(res);
            }
            const auto a = sum/static_cast<float>(NumPoints);
            return a;
        }

    };

    /// NOTE: You probably want to use a dcBlocker after these!!!
    template<size_t order>
    struct Chebyshev
    {
        
        static constexpr float calculate(const float value)
        {
            static_assert(order <= 5, "That order hasn't been implemented yet. Feel free to add it");

            if constexpr (order == 0)
                return 1;
            if constexpr (order == 1)
                return value;
            if constexpr (order == 2)
                return 2.0f * value * value - 1.0f;
            if constexpr (order == 3)
                return 4.0f * value * value * value - 3.0f * value;
            if constexpr (order == 4)
            {
                const auto valueSqr = value*value;
                return 8.0f * valueSqr * valueSqr - 8.0f * valueSqr + 1.0f;
            }
            if constexpr (order == 5)
            {
                const auto valueSqr = value*value;
                const auto valueCube = valueSqr * value;
                return 16.0f*valueCube*valueSqr - 20.0f*valueCube + 5.0f*value;
            }

            return 0;
        }
    };
};
}
