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
public:

	static size_t getInitialPluginRandomSeed()
	{
		static const auto seed = static_cast<size_t>(juce::String(JucePlugin_Name).hashCode());
		return seed;
	}

	static juce::LookAndFeel_V4::ColourScheme getBaseColourScheme()
	{
		return juce::LookAndFeel_V4::getDarkColourScheme();
	}

	static juce::Colour createRandomisedColour(size_t& seed, juce::Colour colour)
	{
		seed = helpers::functions::utilities::XORShift(seed);
		const auto rotate = juce::MathConstants<float>::pi * static_cast<float>(seed) / static_cast<float>(std::numeric_limits<size_t>::max());

		seed = helpers::functions::utilities::XORShift(seed);
		const float alpha = static_cast<float>(seed % 256) / static_cast<float>(256);


		seed = helpers::functions::utilities::XORShift(seed);
		const float saturation = static_cast<float>(seed % 256) / static_cast<float>(256);


		seed = helpers::functions::utilities::XORShift(seed);
		const float brightness = static_cast<float>(seed % 256) / static_cast<float>(256);

		seed = helpers::functions::utilities::XORShift(seed);
		const float lightness = static_cast<float>(seed % 256) / static_cast<float>(256);


		return colour
						.withRotatedHue(rotate)
						.withMultipliedAlpha(juce::jmap(alpha, 0.5f, 1.5f))
						.withMultipliedSaturation(juce::jmap(saturation, 0.75f, 1.75f))
						.withMultipliedBrightness(juce::jmap(brightness, 0.75f, 1.75f))
						.withMultipliedLightness(juce::jmap(lightness, 0.75f, 1.25f));
	}

	static juce::Image createBackground()
	{
		auto bg = juce::Rectangle<float>{0, 0};
		for (auto display : juce::Desktop::getInstance().getDisplays().displays)
		{
			bg.setHeight(juce::jmax(display.logicalBounds.getHeight(), bg.getHeight()));
			bg.setWidth(juce::jmax(display.logicalBounds.getWidth(), bg.getWidth()));
		}

		juce::Image img{juce::Image::PixelFormat::ARGB, static_cast<int>(bg.getWidth()), static_cast<int>(bg.getHeight()), true};
		Graphics g (img);


		auto seed = getInitialPluginRandomSeed();
		constexpr auto numRandomShapes = static_cast<size_t>(100);
		const static auto randomColours = [&](){
			std::vector<juce::Colour> colours;
			colours.reserve(numRandomShapes);
			auto colour = getBaseColourScheme().getUIColour(juce::LookAndFeel_V4::ColourScheme::defaultFill);
			for (auto i = 0ul; i < numRandomShapes; ++i)
			{
				colour = createRandomisedColour(seed, colour);
				colours.push_back(colour);
			}

			return colours;
		}();

		const static auto randomShapes = [&](){
			std::vector<std::array<float, 7>> shapes;
			shapes.reserve(numRandomShapes);
			for (auto i = 0ul; i < numRandomShapes; ++i)
			{
				seed = helpers::functions::utilities::XORShift(seed);
				const auto x = static_cast<float>(seed) / static_cast<float>(std::numeric_limits<size_t>::max());
				seed = helpers::functions::utilities::XORShift(seed);
				const auto y = static_cast<float>(seed) / static_cast<float>(std::numeric_limits<size_t>::max());
				seed = helpers::functions::utilities::XORShift(seed);
				const auto w = static_cast<float>(seed) / static_cast<float>(std::numeric_limits<size_t>::max());
				seed = helpers::functions::utilities::XORShift(seed);
				const auto h = static_cast<float>(seed) / static_cast<float>(std::numeric_limits<size_t>::max());
				seed = helpers::functions::utilities::XORShift(seed);
				const auto r = static_cast<float>(seed) / static_cast<float>(std::numeric_limits<size_t>::max());
				seed = helpers::functions::utilities::XORShift(seed);
				const auto sx = static_cast<float>(seed) / static_cast<float>(std::numeric_limits<size_t>::max());
				seed = helpers::functions::utilities::XORShift(seed);
				const auto sy = static_cast<float>(seed) / static_cast<float>(std::numeric_limits<size_t>::max());
				shapes.push_back({x, y, w, h, r, sx, sy});
			}
			return shapes;
		}();

		const auto w_ = static_cast<float>(bg.getWidth());
		const auto h_ = static_cast<float>(bg.getHeight());
		for (auto i = 0ul; i < numRandomShapes; ++i)
		{
			g.setColour(randomColours[i]);

			const auto& shape = randomShapes[i];
			const auto rect = juce::Rectangle<float>{shape[0]*w_*1.5f, shape[1]*h_*1.5f, shape[2]*w_, shape[3]*h_};
			const auto rotation = shape[4] * juce::MathConstants<float>::pi*2.0f;
			juce::Path p;
			p.addRectangle(rect);

			p.applyTransform(juce::AffineTransform::rotation(rotation, rect.getX(), rect.getY()));
			p.applyTransform(juce::AffineTransform::shear(shape[5], shape[6]));
			g.fillPath(p);
		}
		return img;
	}

	LookAndFeel(): juce::LookAndFeel_V4(makeColourScheme()) {}
	~LookAndFeel() override {}
private:
	static juce::LookAndFeel_V4::ColourScheme makeColourScheme()
	{
		auto scheme = getBaseColourScheme();
		auto seed = getInitialPluginRandomSeed();

		for ( auto i = 0; i < ColourScheme::numColours; ++i)
		{
			auto id = static_cast<ColourScheme::UIColour>(i);

			if (id == ColourScheme::defaultFill)
			{
				scheme.setUIColour(ColourScheme::defaultFill, scheme.getUIColour(ColourScheme::widgetBackground).contrasting(0.25f));
				continue;
			}
			if (id == ColourScheme::highlightedFill)
			{
				scheme.setUIColour(ColourScheme::highlightedFill, juce::Colour::contrasting(scheme.getUIColour(ColourScheme::defaultFill), scheme.getUIColour(ColourScheme::widgetBackground)));
				continue;
			}
			auto col = scheme.getUIColour(id);
			auto alpha = col.getAlpha();
			scheme.setUIColour(id, createRandomisedColour(seed, col).withAlpha(alpha));
		}
		return scheme;
	}

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

	/// borrowed from juce::LookandFeelHelpers.h just so I could tweak the corner size of the window...
	struct LookAndFeelHelpers
	{
		static juce::TextLayout layoutTooltipText (TypefaceMetricsKind metrics, const String& text, Colour colour) noexcept
		{
			const float tooltipFontSize = 13.0f;
			const int maxToolTipWidth = 400;

			juce::AttributedString s;
			s.setWordWrap (juce::AttributedString::WordWrap::byChar);
			s.setJustification (juce::Justification::centred);
			s.append (text, juce::FontOptions (tooltipFontSize, juce::Font::bold).withMetricsKind (metrics), colour);

			juce::TextLayout tl;
			tl.createLayoutWithBalancedLineLengths (s, (float) maxToolTipWidth);
			return tl;
		}
	};

	void drawTooltip (Graphics& g, const String& text, int width, int height) override
	{
		Rectangle<int> bounds (width, height);
		auto cornerSize = 1.0f;

		g.setColour (findColour (TooltipWindow::backgroundColourId));
		g.fillRoundedRectangle (bounds.toFloat(), cornerSize);

		g.setColour (findColour (TooltipWindow::outlineColourId));
		g.drawRoundedRectangle (bounds.toFloat().reduced (0.5f, 0.5f), cornerSize, 1.0f);

		LookAndFeelHelpers::layoutTooltipText (getDefaultMetricsKind(), text, findColour (TooltipWindow::textColourId))
			.draw (g, { static_cast<float> (width), static_cast<float> (height) });
	}
};

}


