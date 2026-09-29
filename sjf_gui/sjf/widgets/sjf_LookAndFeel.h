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

	const static auto modulatedID = juce::Identifier ("modulated");

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

		auto col = [&]()
		{
			const auto hue = (component.getProperties().getWithDefault(look_and_feel::ids::modulatedID, false)) ? 0.3f : 0.0f;
			auto c1 = component.findColour (ToggleButton::tickDisabledColourId);
			auto c1h = component.findColour (ToggleButton::tickDisabledColourId).withRotatedHue(hue);
			if (hue < 0.3f || c1h != c1)
			{
				return c1h;
			}
			else if (const auto c2 = component.findColour(juce::Slider::trackColourId); c2 != c1)
			{
				return c2.withRotatedHue(hue);
			}
			return c1;
		}();

		g.setColour(col);

		g.drawRoundedRectangle (tickBounds, 4.0f, 1.0f);

		if (ticked)
		{
			g.fillRoundedRectangle (tickBounds.reduced(2), 2.0f);
		}
	}

	void drawLinearSlider (Graphics& g, int x, int y, int width, int height,
									   float sliderPos,
									   float minSliderPos,
									   float maxSliderPos,
									   const Slider::SliderStyle style, Slider& slider) override
	{
		if (auto parent = slider.getParentComponent())
		{
			const auto hue = (slider.getProperties().getWithDefault(look_and_feel::ids::modulatedID, false)) ? 0.3f : 0.0f;
			slider.setColour(Slider::trackColourId, parent->findColour (Slider::trackColourId).withRotatedHue(hue));
			slider.setColour(Slider::backgroundColourId, parent->findColour (Slider::backgroundColourId).withRotatedHue(hue));
		}
		LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
	}

	void drawComboBox (Graphics& g, int width, int height, bool isButtonDown,
					   int buttonX, int buttonY, int buttonW, int buttonH, ComboBox& box) override
	{
		if (auto parent = box.getParentComponent())
		{
			const auto hue = (box.getProperties().getWithDefault(look_and_feel::ids::modulatedID, false)) ? 0.3f : 0.0f;
			box.setColour(juce::ComboBox::backgroundColourId, parent->findColour(juce::ComboBox::backgroundColourId).withRotatedHue(hue));
		}
		juce::LookAndFeel_V4::drawComboBox(g, width, height, isButtonDown, buttonX, buttonY, buttonW, buttonH, box);
	}
};

}


