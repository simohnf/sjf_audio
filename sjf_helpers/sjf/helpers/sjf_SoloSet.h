/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 15/09/2026.
//

#pragma once
#include <JuceHeader.h>

namespace sjf::helpers{
class SoloSet : juce::AudioProcessorParameter::Listener
{
public:
	SoloSet(const bool parallel_) : parallel(parallel_)
	{}

	~SoloSet() override
	{
		for (auto parameter : parameters)
			parameter->removeListener(this);
	}

	void addToSoloSet(RangedAudioParameter* soloParam)
	{
		jassert(std::ranges::find(parameters, soloParam) == parameters.end());
		parameters.push_back(soloParam);
		soloSet.setBit(static_cast<int>(parameters.size()) - 1, false);
		soloParam->addListener(this);
	}



	int numberOfActiveSolos() const
	{
		return numberOfSetBits.load();
	}

	bool isParallelSolo()
	{
		return parallel;
	}
private:
	void parameterValueChanged (int /*parameterIndex*/, float /*newValue*/) override
	{}

	void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override
	{
		if (changedParamIndex == -1 && gestureIsStarting)
			changedParamIndex = parameterIndex;

		if (!gestureIsStarting && parameterIndex == changedParamIndex)
		{
			for (auto i = 0ul; i < parameters.size(); ++i)
			{
				if (parameterIndex == parameters[i]->getParameterIndex())
				{
					setSolo(parameters[i], parameters[i]->getValue() > 0.5f, ModifierKeys::getCurrentModifiers().isShiftDown());
					changedParamIndex = -1;
					break;
				}
			}
		}
	}

	void setSolo(RangedAudioParameter* soloParam, const bool on, const bool shiftDown)
	{
		jassert(std::ranges::find(parameters, soloParam) != parameters.end());
		jassert(MessageManager::existsAndIsLockedByCurrentThread());
		const auto indx = std::distance(parameters.begin(), std::ranges::find(parameters, soloParam));
		if (!shiftDown)
		{
			soloSet.clear();
		}

		soloSet.setBit(static_cast<int>(indx), on);
		for (auto i = 0ul; i < parameters.size(); ++i)
		{
			if (!juce::approximatelyEqual(soloSet[static_cast<int>(i)] ? 1.0f : 0.0f, parameters[i]->getValue()))
			{
				parameters[i]->beginChangeGesture();
				parameters[i]->setValueNotifyingHost(soloSet[static_cast<int>(i)] ? 1.0f : 0.0f);
				parameters[i]->endChangeGesture();
			}
		}
		numberOfSetBits.store(soloSet.countNumberOfSetBits());
	}

	std::vector<RangedAudioParameter*> parameters;
	juce::BigInteger soloSet;
	std::atomic<int> numberOfSetBits{0};
	int changedParamIndex{-1};
	const bool parallel{false};
};

}


