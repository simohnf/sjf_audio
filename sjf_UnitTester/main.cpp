/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 22/07/2026.
//
#include "sjf_UnitTestRunner.h"

static std::optional<int> parseSeedFromArgs(const juce::ArgumentList& args)
{
	// 1. Check for --seed or -seed flags with space or equals (e.g., --seed 1234, -seed=1234)
	for (const auto* flag : { "--seed", "-seed" })
	{
		if (args.containsOption(flag))
		{
			const auto valStr = args.getValueForOption(flag);
			if (valStr.isNotEmpty() && valStr.containsOnly("0123456789"))
				return valStr.getIntValue();
		}
	}
	return nullopt;
}

int main(int argc, char* argv[])
{
	juce::ScopedJuceInitialiser_GUI juceGui;
	sjf::tests::UnitTestRunner runner;

	const auto randomSeed = parseSeedFromArgs(juce::ArgumentList(argc, argv));

	if (randomSeed.has_value())
		return runner.runAndReport("sjf_audio Unit Tests", *randomSeed);

	return runner.runAndReport("sjf_audio Unit Tests");
}