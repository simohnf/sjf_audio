/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 20/08/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_ParameterFactory.h>
#include <sjf//helpers/sjf_Interpolators.h>

namespace sjf::dsp::waveshaper::adaa
{
	/**
	 * @brief An antiderivative and nonlinearity lookup table wrapper for continuous-time ADAA processors.
	 *
	 * Pre-computes and caches the integrated first antiderivative $F_1(x) = \int f(x) \, dx$ of a target memoryless
	 * nonlinearity using Simpson's rule numerical integration over a static, padded lookup table.
	 *
	 * Provides unified Catmull-Rom cubic spline interpolation for evaluating $F_1(x)$ via `operator()` and
	 * its exact continuous derivative $f(x) = \frac{d}{dx} F_1(x)$ via `processSample()`, ensuring mathematical
	 * continuity across ADAA fallback thresholds.
	 *
	 * @tparam Nonlinearity The target memoryless non-linearity class providing `processSample(float)` and `getName()`.
	 * @tparam AbsMaxLevel Pre-clipping voltage limit defining the lookup table range $[-\text{AbsMaxLevel}, +\text{AbsMaxLevel}]$. Defaults to `10.0f`.
	 * @tparam UserTableSize The base resolution of the lookup table. Internal sizing automatically adjusts for odd symmetry and padding. Defaults to `4096`.
	 */
	template<typename Nonlinearity, float AbsMaxLevel = 10.0f, size_t UserTableSize = 4096>
	struct ADAAAntiderivativeWrapper
	{
		static constexpr auto TablePadding = 2ul;
		static constexpr auto InputTableSize = (UserTableSize % 2 == 0) ? UserTableSize + 1 : UserTableSize;
		static constexpr size_t TableSize = InputTableSize + 2*TablePadding;
		static constexpr auto indexToVoltageScale = (2.0f * AbsMaxLevel)/static_cast<float>(InputTableSize - 1);
		static constexpr auto clippingPoint = AbsMaxLevel + static_cast<float>(TablePadding)*indexToVoltageScale;

		using Table = std::array<float, TableSize>;

		static const juce::String& getName() { return Nonlinearity::getName(); }


		[[nodiscard]] float operator() (const float x) const
		{
			return getAntiDerivative(x, adaaTable);
		}

		[[nodiscard]] float processSample(const float x) const
		{
			return getNonlinearityFromTable(x);
		}

	private:
		struct TableMaker
		{
			static Table makeFirstOrderTable()
			{
				Table table{};
				Nonlinearity nonlinearity_;


				table[0] = 0.0f;
				auto runningTotal = 0.0l;

				if constexpr (true)
				{
					// Simpsons rule
					constexpr double dtOver6 = indexToVoltageScale / 6.0;

					for (size_t i = 1ul; i < TableSize; ++i)
					{
						const auto xL = juce::jmap(static_cast<float>(i - 1), 0.0f, static_cast<float>(TableSize - 1), -clippingPoint, clippingPoint);
						const auto xM = juce::jmap(static_cast<float>(i) - 0.5f, 0.0f, static_cast<float>(TableSize - 1), -clippingPoint, clippingPoint);
						const auto xR = juce::jmap(static_cast<float>(i),     0.0f, static_cast<float>(TableSize - 1), -clippingPoint, clippingPoint);

						const auto yL = static_cast<double>(nonlinearity_.processSample(xL));
						const auto yM = static_cast<double>(nonlinearity_.processSample(xM));
						const auto yR = static_cast<double>(nonlinearity_.processSample(xR));

						runningTotal += dtOver6 * (yL + 4.0 * yM + yR);
						table[i] = static_cast<float>(runningTotal);
					}
				}
				else
				{
					constexpr double dtOver90 = indexToVoltageScale / 90.0;

					for (auto i = 1ul; i < TableSize; ++i)
					{
						const auto x0 = juce::jmap(static_cast<float>(i - 1),     0.0f, static_cast<float>(TableSize - 1), -clippingPoint, clippingPoint);
						const auto x1 = juce::jmap(static_cast<float>(i) - 0.75f, 0.0f, static_cast<float>(TableSize - 1), -clippingPoint, clippingPoint);
						const auto x2 = juce::jmap(static_cast<float>(i) - 0.50f, 0.0f, static_cast<float>(TableSize - 1), -clippingPoint, clippingPoint);
						const auto x3 = juce::jmap(static_cast<float>(i) - 0.25f, 0.0f, static_cast<float>(TableSize - 1), -clippingPoint, clippingPoint);
						const auto x4 = juce::jmap(static_cast<float>(i),        0.0f, static_cast<float>(TableSize - 1), -clippingPoint, clippingPoint);

						const auto y0 = static_cast<double>(nonlinearity_.processSample(x0));
						const auto y1 = static_cast<double>(nonlinearity_.processSample(x1));
						const auto y2 = static_cast<double>(nonlinearity_.processSample(x2));
						const auto y3 = static_cast<double>(nonlinearity_.processSample(x3));
						const auto y4 = static_cast<double>(nonlinearity_.processSample(x4));

						runningTotal += dtOver90 * (7.0*y0 + 32.0*y1 + 12.0*y2 + 32.0*y3 + 7.0*y4);
						table[i] = static_cast<float>(runningTotal);
					}
				}
				return table;
			}

			static Table makeFirstOrderDerivativeTable()
			{
				Table table{};
				Nonlinearity nonlinearity_;

				for (size_t i = 0ul; i < TableSize; ++i)
				{
					const auto x = juce::jmap(static_cast<float>(i),     0.0f, static_cast<float>(TableSize - 1), -clippingPoint, clippingPoint);

					table[i] = nonlinearity_.processSample(x) * indexToVoltageScale;
				}
				return table;
			}
		};

		inline const static Table adaaTable = TableMaker::makeFirstOrderTable();
		inline const static Table slopeTable = TableMaker::makeFirstOrderDerivativeTable();


		forcedinline float getAntiDerivative(float x, const Table& table) const
		{
			constexpr auto centerIndex = static_cast<float>(TableSize - 1) * 0.5f;
			const auto findx = centerIndex + (x / indexToVoltageScale);

			const auto ind1 = static_cast<size_t>(findx);
			const auto mu = findx - static_cast<float>(ind1);

			const auto y0 = table[ind1 - 1];
			const auto y1 = table[ind1];
			const auto y2 = table[ind1 + 1];
			const auto y3 = table[ind1 + 2];

			return sjf::interpolation::cubicInterpolateCatmullRom(mu, y0, y1, y2, y3);
		}

		[[nodiscard]] forcedinline float getNonlinearityFromTable(float x) const
		{
			constexpr auto centerIndex = static_cast<float>(TableSize - 1) * 0.5f;
			const auto findx = centerIndex + (x / indexToVoltageScale);

			const auto ind1 = static_cast<size_t>(findx);
			const auto mu = findx - static_cast<float>(ind1);

			const auto y0 = adaaTable[ind1 - 1];
			const auto y1 = adaaTable[ind1];
			const auto y2 = adaaTable[ind1 + 1];
			const auto y3 = adaaTable[ind1 + 2];

			// Coefficients for dP/dmu
			const float c0 = y2 - y0;
			const float c1 = 2.0f * (2.0f * y0 - 5.0f * y1 + 4.0f * y2 - y3);
			const float c2 = 3.0f * (-y0 + 3.0f * y1 - 3.0f * y2 + y3);

			const float dP_dmu = 0.5f * (c0 + mu * (c1 + mu * c2));

			// Convert dF1/dmu -> dF1/dx (nonlinearity output)
			return dP_dmu / indexToVoltageScale;
		}

		Nonlinearity nonlinearity;

	};

	/**
	 * @brief A first-order Antiderivative Anti-Aliasing (ADAA1) stateless waveshaper wrapper.
	 *
	 * Implements continuous-time first-order ADAA to suppress aliasing generated by memoryless
	 * non-linearities. It uses the first antiderivative $F_1(x)$ of the target non-linearity $f(x)$
	 * to compute the continuous average over the sample interval:
	 *
	 * $$\bar{y}[n] = \frac{F_1(x[n]) - F_1(x[n-1])}{x[n] - x[n-1]}$$
	 *
	 * When the input change $\Delta x = x[n] - x[n-1]$ falls below a defined numerical tolerance,
	 * it falls back to direct evaluation $f(x[n])$ to prevent division by zero and catastrophic cancellation.
	 *
	 * @tparam Nonlinearity The target memoryless non-linearity class providing `processSample(float)` and `getName()`.
	 * @tparam F1 Functor/class implementing the first antiderivative $F_1(x) = \int f(x) \, dx$.
	 * @tparam AbsMaxLevel Pre-clipping threshold applied to constrain the input dynamic range before evaluation. Defaults to `10.0f`.
	 */
	template<typename Nonlinearity, typename F1, float AbsMaxLevel = 10.0f>
	struct ADAA
	{
		static constexpr auto tolerance = 1.0e-5f;
		static const juce::String& getName()
		{
			return Nonlinearity::getName();
		}

		float processSample (float x)
		{
			x = sjf::helpers::Waveshapers::Clippers::hard(x, AbsMaxLevel);

			const auto antiDerivative = f1(x);


			const float delta = x - lastInput;

			const auto ret = std::abs(delta) < tolerance ?
								nonlinearity.processSample((x+lastInput)*0.5f) :
								(antiDerivative - lastAntiDerivative) / delta;

			lastInput = x;
			lastAntiDerivative = antiDerivative;

			return ret;
		}

		void reset()
		{
			lastInput = lastAntiDerivative = 0.0f;
		}

	private:
		Nonlinearity nonlinearity{};
		F1 f1{};
		float lastInput{}, lastAntiDerivative{};
	};


}


