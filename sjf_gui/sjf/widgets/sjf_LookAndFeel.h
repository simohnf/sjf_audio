/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 24/08/2026.
//

#pragma once
#include <JuceHeader.h>

namespace sjf::gui{
namespace look_and_feel::ids
{
	namespace button
	{
		static const juce::Identifier drawText{"drawText"};
	}
}
class LookAndFeel : public juce::LookAndFeel_V4
{
	int getSliderThumbRadius (Slider& slider) override
	{
		return jmin (6, slider.isHorizontal() ? static_cast<int> (static_cast<float>(slider.getHeight()) * 0.25f)
										   : static_cast<int> (static_cast<float>(slider.getWidth())  * 0.25f));
	}

	void drawToggleButton (Graphics& g, ToggleButton& button,
									   bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
	{
		const auto fontSize = jmin (15.0f, static_cast<float>(button.getHeight()) * 0.75f);
		const auto tickWidth = fontSize * 1.1f;

		drawTickBox (g, button, 4.0f, (static_cast<float>(button.getHeight()) - tickWidth) * 0.5f,
					 tickWidth, tickWidth,
					 button.getToggleState(),
					 button.isEnabled(),
					 shouldDrawButtonAsHighlighted,
					 shouldDrawButtonAsDown);

		g.setColour (button.findColour (ToggleButton::textColourId));
		g.setFont (fontSize);

		if (! button.isEnabled())
			g.setOpacity (0.5f);

		if (button.getProperties().getWithDefault(look_and_feel::ids::button::drawText, false))
		{
			g.drawFittedText (button.getButtonText(),
							 button.getLocalBounds().withTrimmedLeft (roundToInt (tickWidth) + 10)
													.withTrimmedRight (2),
							 Justification::centredLeft, 10);
		}
	}

	void drawTickBox (Graphics& g, Component& component,
									  float x, float y, float w, float h,
									  const bool ticked,
									  [[maybe_unused]] const bool isEnabled,
									  [[maybe_unused]] const bool shouldDrawButtonAsHighlighted,
									  [[maybe_unused]] const bool shouldDrawButtonAsDown) override
	{
		const Rectangle<float> tickBounds (x, y, w, h);

		g.setColour (component.findColour (ToggleButton::tickDisabledColourId));
		g.drawRoundedRectangle (tickBounds, 4.0f, 1.0f);

		if (ticked)
		{
			g.setColour (component.findColour (ToggleButton::tickColourId));
			g.fillRoundedRectangle (tickBounds.reduced(2), 2.0f);
		}
	}
};

}


