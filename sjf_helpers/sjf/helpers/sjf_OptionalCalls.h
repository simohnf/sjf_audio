//
// Created by Simon Fay on 15/07/2026.
//

#include <JuceHeader.h>
namespace sjf::optional_calls
{
/*
  void setPositionInfo(const Optional<juce::AudioPlayHead::PositionInfo>& positionInfo)
*/
template <typename T>
auto setPositionInfo(T& processor, const Optional<juce::AudioPlayHead::PositionInfo>& info, int)
-> decltype(processor.setPositionInfo(info), void())
{
    processor.setPositionInfo(info);
}
template <typename T>
void setPositionInfo(T&, const Optional<juce::AudioPlayHead::PositionInfo>&, long)
{
    // Do nothing
}

template <typename T>
auto setPositionInfo(T& processor, const Optional<juce::AudioPlayHead::PositionInfo>& info)
{
    optional_calls::setPositionInfo(processor, info, 0);
}

}