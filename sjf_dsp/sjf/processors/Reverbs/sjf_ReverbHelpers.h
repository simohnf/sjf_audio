/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 31/07/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/helpers/sjf_HelperFunctions.h>


namespace sjf::dsp::reverb_helpers
{
struct  ReverbDelayTimeCalculator
{
	enum class SpacingType
	{
		Linear,
		Logarithmic,
		PowerRatio // e.g., Golden Ratio spacing
	};

	template<size_t NumDelayTimes, float MinMs, float MaxMs, SpacingType Spacing>
	static constexpr std::array<float, NumDelayTimes> calculateMsDelayTimes()
	{
		static_assert(NumDelayTimes > 0, "NumDelayTimes must be greater than 0");
		static_assert(MaxMs > MinMs + 1.0f, "MaxMs must be greater than MinMs + 1");
		static_assert(MinMs > 0.01f, "MinMs must be greater than 0.01f");
		std::array<float, NumDelayTimes> times{};


		const auto range = (MaxMs - MinMs);


		if constexpr (NumDelayTimes == 1)
		{
			times[0] = MinMs;
			return times;
		}

		if constexpr (Spacing == SpacingType::Linear)
		{
			for ( auto i = 0ul; i < NumDelayTimes; ++i )
			{
				const auto frac = static_cast<float>(i) / static_cast<float>(NumDelayTimes - 1);
				times[i] = MinMs + frac * range;
			}
		}
		else if constexpr (Spacing == SpacingType::Logarithmic)
		{
			const float ratio = MaxMs / MinMs;
			const float logRatio = helpers::functions::utilities::constexpr_ln(ratio);

			for (auto i = 0ul; i < NumDelayTimes; ++i)
			{
				const float frac = static_cast<float>(i) / static_cast<float>(NumDelayTimes - 1);

				// times[i] = MinMs * e^(frac * ln(MaxMs/MinMs))
				times[i] = MinMs * helpers::functions::utilities::constexpr_exp(frac * logRatio);
			}
		}
		else if constexpr (Spacing == SpacingType::PowerRatio)
		{
			// 1. Accumulate phi dynamically inside the loop (faster & simpler than calling powOfPhi repeatedly)
			constexpr float phi = 1.61803398875f;

			// We compute phi^(N-1) for scaling
			float maxPhiPower = 1.0f;
			for (size_t p = 0; p < NumDelayTimes - 1; ++p)
				maxPhiPower *= phi;

			// Shift power down by 1 so i=0 produces 0.0 offset
			// growthScale maps (phi^i - 1) / (phi^(N-1) - 1)
			const float phiRange = maxPhiPower - 1.0f;

			float currentPhiPower = 1.0f;
			for (auto i = 0ul; i < NumDelayTimes; ++i)
			{
				const float normalized = (currentPhiPower - 1.0f) / phiRange;
				times[i] = MinMs + normalized * range;

				currentPhiPower *= phi; // Advance to next power: phi^0, phi^1, phi^2...
			}
		}
		return times;
	}

	template < size_t NumDelayTimes >
	static constexpr void calculateCoprimeSampleTimes( const std::array<float, NumDelayTimes>& msTimes, std::array<size_t, NumDelayTimes>& samples, const double sampleRate, const float sizeScale = 1.0f) noexcept
	{
		const double msToSamples = sampleRate * 0.001;

		for (size_t i = 0; i < NumDelayTimes; ++i)
		{
			const float scaledMs = msTimes[i] * sizeScale;
			const auto rawTarget = static_cast<size_t>(std::round(scaledMs * msToSamples));

			// Tap 0: Use raw target directly (enforcing minimum 2 samples)
			if (i == 0)
			{
				samples[0] = (rawTarget < 2) ? 2 : rawTarget;
				continue;
			}

			const auto previous = samples[i - 1];
			auto result = 0ul;

			// Alternate strategy: Even taps (i=2,4...) search Coprime -> Prime
			// Odd taps (i=1,3...) search Prime -> Coprime

			if ((i & 1) != 0)
			{
				result = findLocalPrime(previous, rawTarget);
				if (result == 0ul)
				{
					result = findLocalCoprime(previous, rawTarget, samples, i);
				}
			}
			else
			{
				result = findLocalCoprime(previous, rawTarget, samples, i);
				if (result == 0ul)
				{
					result = findLocalPrime(previous, rawTarget);
				}
			}

			// --- Exceptional Case: Search failed, execute Fallback ---
			if (result == 0ul)
			{
				// Raw target clamped to at least 1 sample above previous
				result = (rawTarget > previous) ? rawTarget : (previous + 1);
			}

			samples[i] = result;
		}
	}


	enum class FillMode { Column, Row };
	enum class ShuffleMode { Column, Row, Both };

	template <FillMode Fill, ShuffleMode Shuffle, size_t NumRows, size_t NumColumns>
	static constexpr void calculateCoprimeSampleTimes( const std::array<float, NumRows * NumColumns>& msTimes, std::array<std::array<size_t, NumColumns>, NumRows>& samples, const double sampleRate, const float sizeScale = 1.0f) noexcept
	{
		std::array<size_t, NumRows * NumColumns> flatSamplesArray{};
		calculateCoprimeSampleTimes(msTimes, flatSamplesArray, sampleRate, sizeScale); // this is the original method for flat arrays
		apply2DIndexMap<NumRows, NumColumns, Fill, Shuffle>(flatSamplesArray, samples);
	}

private:


	template <size_t NumRows, size_t NumColumns, FillMode Fill, ShuffleMode Shuffle>
	static constexpr auto create2DIndexMap()
	{
		std::array<std::array<size_t, NumColumns>, NumRows> indexMap{};

		// Helper lambda to calculate prime stride > N / 2
		constexpr auto getStride = [](size_t N) constexpr {
			if (N <= 2) return static_cast<size_t>(1);
			size_t candidate = (N / 2) + 1;
			while (candidate < N) {
				if (helpers::functions::utilities::isPrime(candidate)) return candidate;
				++candidate;
			}
			return static_cast<size_t>(1);
		};

		// Set stride to 0 if that dimension isn't shuffled
		constexpr size_t rowStride = (Shuffle == ShuffleMode::Row || Shuffle == ShuffleMode::Both)
									 ? getStride(NumRows) : 0;

		constexpr size_t colStride = (Shuffle == ShuffleMode::Column || Shuffle == ShuffleMode::Both)
									 ? getStride(NumColumns) : 0;

		for (size_t r = 0; r < NumRows; ++r)
		{
			for (size_t c = 0; c < NumColumns; ++c)
			{
				size_t targetRow = (r + c * rowStride) % NumRows;
				size_t targetCol = (c + r * colStride) % NumColumns;

				if constexpr (Fill == FillMode::Column) {
					indexMap[targetRow][targetCol] = c * NumRows + r;
				} else {
					indexMap[targetRow][targetCol] = r * NumColumns + c;
				}
			}
		}

		return indexMap;
	}



	template <size_t NumRows, size_t NumColumns, FillMode Fill, ShuffleMode Shuffle, typename T>
	static constexpr void apply2DIndexMap(
	const std::array<T, NumRows * NumColumns>& flatArray,
	std::array<std::array<size_t, NumColumns>, NumRows>& samples)
	{
		constexpr auto  indexMap = create2DIndexMap<NumRows, NumColumns, Fill, Shuffle>();
		for (size_t r = 0; r < NumRows; ++r)
			for (size_t c = 0; c < NumColumns; ++c)
				samples[r][c] = flatArray[indexMap[r][c]];
	}


	static constexpr size_t findLocalPrime(size_t previous, size_t target) noexcept
	{
		if (target <= previous)
		{
			target = previous + 1;
		}

		const size_t radius = (target - previous) / 2;

		// Asymmetric range: [target - radius, target + 2 * radius]
		const size_t lowerBound = (target > radius) ? std::max(target - radius, previous + 1) : (previous + 1);
		const size_t upperBound = target + (2 * radius);

		// Fast check: if target itself is prime
		if (helpers::functions::utilities::isPrime(target))
		{
			return target;
		}

		// Search outward from target to bias toward closest candidate
		for (size_t offset = 1; ; ++offset)
		{
			const bool canTryLower = (target >= offset) && ((target - offset) >= lowerBound);
			const bool canTryUpper = (target + offset) <= upperBound;

			if (!canTryLower && !canTryUpper)
			{
				break; // Exhausted search window
			}

			// Bias downward first to counteract length creep
			if (canTryLower)
			{
				const size_t lowerCandidate = target - offset;
				if (helpers::functions::utilities::isPrime(lowerCandidate))
				{
					return lowerCandidate; // Immediate return: closest downward prime
				}
			}

			if (canTryUpper)
			{
				const size_t upperCandidate = target + offset;
				if (helpers::functions::utilities::isPrime(upperCandidate))
				{
					return upperCandidate; // Immediate return: closest upward prime
				}
			}
		}

		return 0; // No prime found in range
	}

	template <size_t N>
	static constexpr size_t findLocalCoprime(
	size_t previous,
	size_t target,
	const std::array<size_t, N>& allocated,
	size_t count) noexcept
	{
		if (target <= previous)
		{
			target = previous + 1;
		}

		const size_t radius = (target - previous) / 2;

		const size_t lowerBound = (target > radius) ? std::max(target - radius, previous + 1) : (previous + 1);
		const size_t upperBound = target + (2 * radius);

		// Fast check: if target itself is coprime to all previous taps
		if (isCoprimeWithAll(target, allocated, count))
		{
			return target;
		}

		// Search outward from target to bias toward closest candidate
		for (size_t offset = 1; ; ++offset)
		{
			const bool canTryLower = (target >= offset) && ((target - offset) >= lowerBound);
			const bool canTryUpper = (target + offset) <= upperBound;

			if (!canTryLower && !canTryUpper)
			{
				break; // Exhausted search window
			}

			// Bias downward first to counteract length creep
			if (canTryLower)
			{
				const size_t lowerCandidate = target - offset;
				if (isCoprimeWithAll(lowerCandidate, allocated, count))
				{
					return lowerCandidate; // Immediate return: closest downward coprime
				}
			}

			if (canTryUpper)
			{
				const size_t upperCandidate = target + offset;
				if (isCoprimeWithAll(upperCandidate, allocated, count))
				{
					return upperCandidate; // Immediate return: closest upward coprime
				}
			}
		}

		return 0; // No valid coprime found in range
	}

	template <size_t N>
	[[nodiscard]] static constexpr bool isCoprimeWithAll(
	size_t candidate,
	const std::array<size_t, N>& allocated,
	size_t count) noexcept
	{
		// A sample delay of 0 or 1 is invalid/trivial for coprime checks
		if (candidate <= 1) return false;

		for (size_t i = 0; i < count; ++i)
		{
			if (helpers::functions::utilities::gcd(candidate, allocated[i]) != 1)
			{
				return false; // Found a shared prime factor with a previously allocated tap
			}
		}

		return true; // Candidate shares no common factors with any allocated tap
	}
};

}


//DUMMY_PLUGIN_SJF_REVERBHELPERS_H
