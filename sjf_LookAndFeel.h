//
//  sjf_lookAndFeel.h
//
//  Created by Simon Fay on 24/08/2022.
//

#ifndef sjf_LookAndFeel_h
#define sjf_LookAndFeel_h

#include <JuceHeader.h>
#include "/Users/simonfay/Programming_Stuff/sjf_audio/sjf_compileTimeRandom.h"

class sjf_lookAndFeel : public juce::LookAndFeel_V4
{
public:
    juce::Colour outlineColour;
    
    sjf_lookAndFeel(){
        outlineColour = juce::Colours::grey;
        
        //        auto slCol = juce::Colours::darkred.withAlpha(0.5f);
        //        setColour (juce::Slider::thumbColourId, juce::Colours::darkred.withAlpha(0.5f));
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::darkblue.withAlpha(0.2f) );
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::darkred.withAlpha(0.7f) );
        setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey.withAlpha(0.2f));
        setColour(juce::ComboBox::backgroundColourId, juce::Colours::darkgrey.withAlpha(0.2f));
    }
    ~sjf_lookAndFeel(){};
    
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                           bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto fontSize = juce::jmin (15.0f, (float) button.getHeight() * 0.75f);
        auto offset = 0.0f;
        drawTickBox(g, button, offset, offset,
                    button.getBounds().getWidth() - offset, button.getBounds().getHeight() - offset,
                    button.getToggleState(),
                    button.isEnabled(),
                    shouldDrawButtonAsHighlighted,
                    shouldDrawButtonAsDown);
        
        
        g.setColour (button.findColour (juce::ToggleButton::textColourId));
        g.setFont (fontSize);
        
        if (! button.isEnabled())
            g.setOpacity (0.5f);
        g.drawFittedText(button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, 10);
    };
    
    void drawTickBox (juce::Graphics& g, juce::Component& component,
                      float x, float y, float w, float h,
                      const bool ticked,
                      const bool isEnabled,
                      const bool shouldDrawButtonAsHighlighted,
                      const bool shouldDrawButtonAsDown) override
    {
        juce::ignoreUnused (isEnabled, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
        
        juce::Rectangle<float> tickBounds (x, y, w, h);
        
        g.setColour (component.findColour (juce::ToggleButton::tickDisabledColourId));
        g.drawRect(tickBounds.getX(), tickBounds.getY(), tickBounds.getWidth(), tickBounds.getHeight());
        
        if (ticked)
        {
            g.setColour (component.findColour (juce::ToggleButton::tickColourId));
            g.setOpacity(0.3);
            auto tick = getCrossShape(0.75f);
            g.fillPath (tick, tick.getTransformToScaleToFit (tickBounds.reduced (4, 5).toFloat(), false));
        }
    };
    
    void drawComboBox (juce::Graphics& g, int width, int height, bool,
                       int, int, int, int, juce::ComboBox& box) override
    {
        juce::Rectangle<int> boxBounds (0, 0, width, height);
        
        g.setColour (box.findColour (juce::ComboBox::outlineColourId));
        g.drawRect(boxBounds.getX(), boxBounds.getY(), boxBounds.getWidth(), boxBounds.getHeight());
        
        juce::Rectangle<int> arrowZone (width - 30, 0, 20, height);
        juce::Path path;
        path.startNewSubPath ((float) arrowZone.getX() + 3.0f, (float) arrowZone.getCentreY() - 2.0f);
        path.lineTo ((float) arrowZone.getCentreX(), (float) arrowZone.getCentreY() + 3.0f);
        path.lineTo ((float) arrowZone.getRight() - 3.0f, (float) arrowZone.getCentreY() - 2.0f);
        
        g.setColour (box.findColour (juce::ComboBox::arrowColourId).withAlpha ((box.isEnabled() ? 0.9f : 0.2f)));
        g.strokePath (path, juce::PathStrokeType (2.0f));
    }
    
    
    
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                           const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider& slider) override
    {
        g.setColour( outlineColour );
        g.drawRect( 0, 0, width, height );
        
        auto outline = slider.findColour (juce::Slider::rotarySliderOutlineColourId);
        auto fill    = slider.findColour (juce::Slider::rotarySliderFillColourId);
        
        auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (10);
        
        auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        auto lineW = juce::jmin (8.0f, radius * 0.5f);
        auto arcRadius = radius - lineW * 0.5f;
        
        juce::Path backgroundArc;
        backgroundArc.addCentredArc (bounds.getCentreX(),
                                     bounds.getCentreY(),
                                     arcRadius,
                                     arcRadius,
                                     0.0f,
                                     rotaryStartAngle,
                                     rotaryEndAngle,
                                     true);
        
        g.setColour (outline);
        g.strokePath (backgroundArc, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        
        if (slider.isEnabled())
        {
            juce::Path valueArc;
            valueArc.addCentredArc (bounds.getCentreX(),
                                    bounds.getCentreY(),
                                    arcRadius,
                                    arcRadius,
                                    0.0f,
                                    rotaryStartAngle,
                                    toAngle,
                                    true);
            
            g.setColour (fill);
            g.strokePath (valueArc, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }
    
    juce::Slider::SliderLayout getSliderLayout (juce::Slider& slider) override
    {
        // 1. compute the actually visible textBox size from the slider textBox size and some additional constraints
        
        int minXSpace = 0;
        int minYSpace = 0;
        
        auto textBoxPos = slider.getTextBoxPosition();
        
        if (textBoxPos == juce::Slider::TextBoxLeft || textBoxPos == juce::Slider::TextBoxRight)
            minXSpace = 30;
        else
            minYSpace = 15;
        
        auto localBounds = slider.getLocalBounds();
        
        auto textBoxWidth  = juce::jmax (0, slider.getWidth() );
        auto textBoxHeight = juce::jmax (0, juce::jmin (slider.getTextBoxHeight(), localBounds.getHeight() - minYSpace));
        
        juce::Slider::SliderLayout layout;
        
        // 2. set the textBox bounds
        
        if (textBoxPos != juce::Slider::NoTextBox)
        {
            if (slider.isBar())
            {
                layout.textBoxBounds = localBounds;
            }
            else
            {
                layout.textBoxBounds.setWidth (textBoxWidth);
                layout.textBoxBounds.setHeight (textBoxHeight);
                
                if (textBoxPos == juce::Slider::TextBoxLeft)           layout.textBoxBounds.setX (0);
                else if (textBoxPos == juce::Slider::TextBoxRight)     layout.textBoxBounds.setX (localBounds.getWidth() - textBoxWidth);
                else /* above or below -> centre horizontally */ layout.textBoxBounds.setX ((localBounds.getWidth() - textBoxWidth) / 2);
                
                if (textBoxPos == juce::Slider::TextBoxAbove)          layout.textBoxBounds.setY (0);
                else if (textBoxPos == juce::Slider::TextBoxBelow)     layout.textBoxBounds.setY (localBounds.getHeight() - textBoxHeight);
                else /* left or right -> centre vertically */    layout.textBoxBounds.setY ((localBounds.getHeight() - textBoxHeight) / 2);
            }
        }
        
        // 3. set the slider bounds
        
        layout.sliderBounds = localBounds;
        
        if (slider.isBar())
        {
            layout.sliderBounds.reduce (1, 1);   // bar border
        }
        else
        {
            if (textBoxPos == juce::Slider::TextBoxLeft)       layout.sliderBounds.removeFromLeft (textBoxWidth);
            else if (textBoxPos == juce::Slider::TextBoxRight) layout.sliderBounds.removeFromRight (textBoxWidth);
            else if (textBoxPos == juce::Slider::TextBoxAbove) layout.sliderBounds.removeFromTop (textBoxHeight);
            else if (textBoxPos == juce::Slider::TextBoxBelow) layout.sliderBounds.removeFromBottom (textBoxHeight);
            
            const int thumbIndent = getSliderThumbRadius (slider);
            
            if (slider.isHorizontal())    layout.sliderBounds.reduce (thumbIndent, 0);
            else if (slider.isVertical()) layout.sliderBounds.reduce (0, thumbIndent);
        }
        
        return layout;
    }
    
    
    void drawButtonBackground (juce::Graphics& g,
                               juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override
    {
        auto cornerSize = 0.0f;
        auto bounds = button.getLocalBounds().toFloat().reduced (0.5f, 0.5f);
        
        auto baseColour = backgroundColour.withMultipliedSaturation (button.hasKeyboardFocus (true) ? 1.3f : 0.9f)
        .withMultipliedAlpha (button.isEnabled() ? 1.0f : 0.5f);
        
        if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)
            baseColour = baseColour.contrasting (shouldDrawButtonAsDown ? 0.2f : 0.05f);
        
        g.setColour (baseColour);
        
        auto flatOnLeft   = button.isConnectedOnLeft();
        auto flatOnRight  = button.isConnectedOnRight();
        auto flatOnTop    = button.isConnectedOnTop();
        auto flatOnBottom = button.isConnectedOnBottom();
        
        if (flatOnLeft || flatOnRight || flatOnTop || flatOnBottom)
        {
            juce::Path path;
            path.addRoundedRectangle (bounds.getX(), bounds.getY(),
                                      bounds.getWidth(), bounds.getHeight(),
                                      cornerSize, cornerSize,
                                      ! (flatOnLeft  || flatOnTop),
                                      ! (flatOnRight || flatOnTop),
                                      ! (flatOnLeft  || flatOnBottom),
                                      ! (flatOnRight || flatOnBottom));
            
            g.fillPath (path);
            
            g.setColour (button.findColour (juce::ComboBox::outlineColourId));
            g.strokePath (path, juce::PathStrokeType (1.0f));
        }
        else
        {
            g.fillRoundedRectangle (bounds, cornerSize);
            
            g.setColour (button.findColour (juce::ComboBox::outlineColourId));
            g.drawRoundedRectangle (bounds, cornerSize, 1.0f);
        }
    }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (sjf_lookAndFeel)
};


template< int NUM_RAND_BOXES >
inline void sjf_makeBackground (juce::Graphics& g, juce::Rectangle< int >& c )
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    auto backColour = juce::Colours::darkgrey;
    g.fillAll ( backColour );
    
//    static constexpr int NUM_RAND_BOXES = 70;
    constexpr auto randomColours = sjf_matrixOfRandomFloats< float, NUM_RAND_BOXES, 4 >();
    constexpr auto randomCoordinates = sjf_matrixOfRandomFloats< float, NUM_RAND_BOXES, 5 >();
    constexpr auto CORNER_SIZE = 5;
    
    for ( int i = 0; i < NUM_RAND_BOXES; i++)
    {
        int red = randomColours.getValue(i, 0) * 255;
        int green = randomColours.getValue(i, 1)  * 255;
        int blue = randomColours.getValue(i, 2) * 255;
        float alpha = randomColours.getValue(i, 3) * 0.3;
        auto randColour = juce::Colour( red, green, blue );
        randColour = randColour.withAlpha(alpha);
        g.setColour( randColour );
        
        auto x = randomCoordinates.getValue( i, 0 ) * c.getWidth()*1.5;// - getWidth();
        auto y = randomCoordinates.getValue( i, 1 ) * c.getHeight()*1.5;// - getHeight();
        auto w = randomCoordinates.getValue( i, 2 ) * c.getWidth();
        auto h = randomCoordinates.getValue( i, 3 ) * c.getHeight();
        auto rectRand = juce::Rectangle<float>( x, y, w, h );
        auto rotate = randomCoordinates.getValue( i, 4 ) * M_PI*2;
        auto shearX = randomCoordinates.getValue( i, 5 );
        auto shearY = randomCoordinates.getValue( i, 6 );
        DBG(" rotate " << rotate << " " << shearX << " " << shearY );
        auto rotation = juce::AffineTransform::rotation( rotate, rectRand.getX(), rectRand.getY() );
        auto shear = juce::AffineTransform::shear(shearX, shearY);
        juce::Path path;
        path.addRoundedRectangle( rectRand, CORNER_SIZE );
      
        int count = 0;
        bool breakFlag = false;
        while( !breakFlag )
        {
            path.applyTransform( shear );
            path.applyTransform( rotation );
            
            count++;
            auto p = path.getBounds().getSmallestIntegerContainer();
            if ( p.intersects( c ) || count > 100 )
            { breakFlag = true; }
        }
        g.fillPath( path );
    }
}

template< int NUM_RAND_BOXES >
inline void sjf_makeBackgroundNoFill (juce::Graphics& g, juce::Rectangle< int >& c )
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
//    auto backColour = juce::Colours::darkgrey;
//    g.fillAll ( backColour );
    
    //    static constexpr int NUM_RAND_BOXES = 70;
    constexpr auto randomColours = sjf_matrixOfRandomFloats< float, NUM_RAND_BOXES, 4 >();
    constexpr auto randomCoordinates = sjf_matrixOfRandomFloats< float, NUM_RAND_BOXES, 5 >();
    constexpr auto CORNER_SIZE = 5;
    
    for ( int i = 0; i < NUM_RAND_BOXES; i++)
    {
        int red = randomColours.getValue(i, 0) * 255;
        int green = randomColours.getValue(i, 1)  * 255;
        int blue = randomColours.getValue(i, 2) * 255;
        float alpha = randomColours.getValue(i, 3) * 0.3;
        auto randColour = juce::Colour( red, green, blue );
        randColour = randColour.withAlpha(alpha);
        g.setColour( randColour );
        
        auto x = randomCoordinates.getValue( i, 0 ) * c.getWidth()*1.5;// - getWidth();
        auto y = randomCoordinates.getValue( i, 1 ) * c.getHeight()*1.5;// - getHeight();
        auto w = randomCoordinates.getValue( i, 2 ) * c.getWidth();
        auto h = randomCoordinates.getValue( i, 3 ) * c.getHeight();
        auto rectRand = juce::Rectangle<float>( x, y, w, h );
        auto rotate = randomCoordinates.getValue( i, 4 ) * M_PI*2;
        auto shearX = randomCoordinates.getValue( i, 5 );
        auto shearY = randomCoordinates.getValue( i, 6 );
        DBG(" rotate " << rotate << " " << shearX << " " << shearY );
        auto rotation = juce::AffineTransform::rotation( rotate, rectRand.getX(), rectRand.getY() );
        auto shear = juce::AffineTransform::shear(shearX, shearY);
        juce::Path path;
        path.addRoundedRectangle( rectRand, CORNER_SIZE );
        path.applyTransform( shear );
        g.fillPath(path, rotation );
    }
}
#endif /* sjf_lookAndFeel_h */
