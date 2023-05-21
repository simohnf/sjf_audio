//
//  sjf_lookAndFeel.h
//
//  Created by Simon Fay on 24/08/2022.
//

#ifndef sjf_LookAndFeel_h
#define sjf_LookAndFeel_h

#include <JuceHeader.h>
#include "sjf_compileTimeRandom.h"
#include "sjf_audioUtilities.h"
class sjf_lookAndFeel : public juce::LookAndFeel_V4
{
public:
    juce::Colour outlineColour = juce::Colours::grey;
    juce::Colour toggleBoxOutlineColour = outlineColour;
    juce::Colour backGroundColour = juce::Colours::darkgrey;
    juce::Colour panelColour = juce::Colours::aliceblue;
    juce::Colour tickColour = juce::Colours::lightgrey;
    juce::Colour sliderFillColour = juce::Colours::red;
    juce::Colour fontColour = juce::Colours::white;
    
    bool drawComboBoxTick = true;
    
    //====================================================================================
    sjf_lookAndFeel()
    {
        //        auto slCol = juce::Colours::darkred.withAlpha(0.5f);
        //        setColour (juce::Slider::thumbColourId, juce::Colours::darkred.withAlpha(0.5f));
        
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::black.withAlpha(0.5f) );
        setColour(juce::Slider::rotarySliderFillColourId, sliderFillColour.withAlpha(0.5f) );
        setColour(juce::Slider::trackColourId, sliderFillColour.withAlpha(0.5f) );
        
        setColour(juce::TextButton::buttonColourId, backGroundColour.withAlpha(0.2f));
        setColour(juce::ComboBox::backgroundColourId, backGroundColour.withAlpha(0.2f));
        setColour(juce::Slider::backgroundColourId, backGroundColour.withAlpha( 0.7f ) );
        setColour(juce::Slider::textBoxOutlineColourId, outlineColour );
//        setColour(juce::Slider::textBoxOutlineColourId, outlineColour );
        setColour(juce::ComboBox::outlineColourId, outlineColour );
//        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::darkblue.withAlpha(0.2f) );
        setColour(juce::PopupMenu::backgroundColourId, backGroundColour.withAlpha(0.7f) );
        setColour(juce::ToggleButton::tickColourId, tickColour.withAlpha(0.5f) );
    }
    //====================================================================================
    ~sjf_lookAndFeel() { /*DBG("DELETING LOOK ANd FEEL");*/ }
    //====================================================================================
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                           bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto fontSize = juce::jmin (15.0f, static_cast<float>( button.getHeight() ) * 0.75f);
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
    //====================================================================================
    void drawTickBox (juce::Graphics& g, juce::Component& component,
                      float x, float y, float w, float h,
                      const bool ticked,
                      const bool isEnabled,
                      const bool shouldDrawButtonAsHighlighted,
                      const bool shouldDrawButtonAsDown) override
    {
        juce::ignoreUnused (isEnabled, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
        
        if (ticked)
        {
            g.setColour (component.findColour (juce::ToggleButton::tickColourId, true));
//            g.setOpacity(0.3);
//            auto x = component.getWidth();
//            auto y = component.getHeight();// - getHeight();
//            auto w = component.getWidth();
//            auto h = component.getHeight();
            g.fillRect( x, y, w, h );
        }
        
        juce::Rectangle<float> tickBounds (x, y, w, h);
//        g.setColour (component.findColour (juce::ToggleButton::tickDisabledColourId));
        g.setColour( toggleBoxOutlineColour );
        g.drawRect(tickBounds.getX(), tickBounds.getY(), tickBounds.getWidth(), tickBounds.getHeight());

    };
    //====================================================================================
    void drawComboBox (juce::Graphics& g, int width, int height, bool,
                       int, int, int, int, juce::ComboBox& box) override
    {
        juce::Rectangle<int> boxBounds (0, 0, width, height);
        
        g.setColour (box.findColour (juce::ComboBox::outlineColourId));
        g.drawRect(boxBounds.getX(), boxBounds.getY(), boxBounds.getWidth(), boxBounds.getHeight());
        
        if ( drawComboBoxTick )
        {
            juce::Rectangle<int> arrowZone (width - width*.2, 0, width*.2, height);
            juce::Path path;
            path.startNewSubPath ( static_cast<float>( arrowZone.getX() ) + 3.0f, static_cast<float>( arrowZone.getCentreY() ) - 2.0f);
            path.lineTo ( static_cast<float>( arrowZone.getCentreX() ), static_cast<float>( arrowZone.getCentreY() ) + 3.0f);
            path.lineTo ( static_cast<float>( arrowZone.getRight() ) - 3.0f, static_cast<float>( arrowZone.getCentreY() ) - 2.0f);
            
            g.setColour (box.findColour (juce::ComboBox::arrowColourId).withAlpha ((box.isEnabled() ? 0.9f : 0.2f)));
            g.strokePath (path, juce::PathStrokeType (2.0f));
        }
        
    }
    //====================================================================================
    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        if ( drawComboBoxTick )
        {
            label.setBounds (1, 1,
                             box.getWidth() + 3 - box.getHeight(),
                             box.getHeight() - 2);
            
            label.setFont (getComboBoxFont (box));
        }
        else
        {
            label.setBounds (1, 1,
                             box.getWidth() - 2,
                             box.getHeight() - 2);
            
            label.setFont (getComboBoxFont (box));
        }
        
    }
    
    //====================================================================================
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
    //====================================================================================
    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPos,
                                           float minSliderPos,
                                           float maxSliderPos,
                           const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        if (slider.isBar())
        {
            g.setColour (slider.findColour (juce::Slider::trackColourId));
            g.fillRect (slider.isHorizontal() ? juce::Rectangle<float> (static_cast<float> (x), static_cast<float>(y) + 0.5f, sliderPos - static_cast<float>(x), static_cast<float>(height) - 1.0f)
                        : juce::Rectangle<float> (static_cast<float>(x) + 0.5f, sliderPos, static_cast<float>(width) - 1.0f, static_cast<float>(y) + (static_cast<float>(height) - sliderPos)));
        }
        else
        {
            auto isTwoVal   = (style == juce::Slider::SliderStyle::TwoValueVertical   || style == juce::Slider::SliderStyle::TwoValueHorizontal);
            auto isThreeVal = (style == juce::Slider::SliderStyle::ThreeValueVertical || style == juce::Slider::SliderStyle::ThreeValueHorizontal);
            
            auto trackWidth = slider.isHorizontal() ? static_cast<float>(height): static_cast<float>(width);
            
            
            juce::Point<float> startPoint (slider.isHorizontal() ? static_cast<float>(x) : static_cast<float>(x) + static_cast<float>(width) * 0.5f,
                                     slider.isHorizontal() ? static_cast<float>(y) + static_cast<float>(height) * 0.5f : static_cast<float>(height + y));
            
            juce::Point<float> endPoint (slider.isHorizontal() ? static_cast<float>(width + x) : startPoint.x,
                                   slider.isHorizontal() ? startPoint.y : static_cast<float>(y) );
            
            juce::Path backgroundTrack;
            backgroundTrack.startNewSubPath (startPoint);
            backgroundTrack.lineTo (endPoint);
            g.setColour (slider.findColour (juce::Slider::backgroundColourId));
            g.strokePath (backgroundTrack, { trackWidth, juce::PathStrokeType::mitered, juce::PathStrokeType::square });
            

            
            juce::Path valueTrack;
            juce::Point<float> minPoint, maxPoint, thumbPoint;
            
            if (isTwoVal || isThreeVal)
            {
                minPoint = { slider.isHorizontal() ? minSliderPos : static_cast<float>(width) * 0.5f,
                    slider.isHorizontal() ? static_cast<float>(height) * 0.5f : minSliderPos };
                
                if (isThreeVal)
                    thumbPoint = { slider.isHorizontal() ? sliderPos : static_cast<float>(width) * 0.5f,
                        slider.isHorizontal() ? static_cast<float>(height) * 0.5f : sliderPos };
                
                maxPoint = { slider.isHorizontal() ? maxSliderPos : static_cast<float>(width) * 0.5f,
                    slider.isHorizontal() ? static_cast<float>(height) * 0.5f : maxSliderPos };
            }
            else
            {
                auto kx = slider.isHorizontal() ? sliderPos : ( static_cast<float>(x) + static_cast<float>(width) * 0.5f);
                auto ky = slider.isHorizontal() ? ( static_cast<float>(y) + static_cast<float>(height) * 0.5f) : sliderPos;
                
                minPoint = startPoint;
                maxPoint = { kx, ky };
            }
            
            auto thumbWidth = getSliderThumbRadius (slider);
            
            valueTrack.startNewSubPath (minPoint);
            valueTrack.lineTo (isThreeVal ? thumbPoint : maxPoint);
            g.setColour (slider.findColour (juce::Slider::trackColourId));
//            g.setColour (juce::Colours::purple);
            g.strokePath (valueTrack, { trackWidth, juce::PathStrokeType::mitered, juce::PathStrokeType::square });
            
            if (! isTwoVal)
            {
                g.setColour (slider.findColour (juce::Slider::thumbColourId));
                g.fillRect (juce::Rectangle<float> (static_cast<float> (thumbWidth), static_cast<float> (thumbWidth)).withCentre (isThreeVal ? thumbPoint : maxPoint));
            }
//              NO NEED FOR THE POINTERSOn The Sliders... I'm not a massive fan of how they look
//            if (isTwoVal || isThreeVal)
//            {
//                auto sr = juce::jmin (trackWidth, (slider.isHorizontal() ? (float) height : (float) width) * 0.4f);
//                auto pointerColour = slider.findColour (juce::Slider::thumbColourId);
//
//                if (slider.isHorizontal())
//                {
//                    drawPointer (g, minSliderPos - sr,
//                                 jmax (0.0f, (float) y + (float) height * 0.5f - trackWidth * 2.0f),
//                                 trackWidth * 2.0f, pointerColour, 2);
//
//                    drawPointer (g, maxSliderPos - trackWidth,
//                                 jmin ((float) (y + height) - trackWidth * 2.0f, (float) y + (float) height * 0.5f),
//                                 trackWidth * 2.0f, pointerColour, 4);
//                }
//                else
//                {
//                    drawPointer (g, jmax (0.0f, (float) x + (float) width * 0.5f - trackWidth * 2.0f),
//                                 minSliderPos - trackWidth,
//                                 trackWidth * 2.0f, pointerColour, 1);
//
//                    drawPointer (g, jmin ((float) (x + width) - trackWidth * 2.0f, (float) x + (float) width * 0.5f), maxSliderPos - sr,
//                                 trackWidth * 2.0f, pointerColour, 3);
//                }
//            }
            g.setColour( outlineColour );
            g.drawRect( 0, 0, slider.getWidth(), slider.getHeight() );
        }
    }
    //====================================================================================
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
    
    //====================================================================================
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
//====================================================================================
inline void sjf_drawBackgroundImage( juce::Graphics& g, const juce::Image& backGroundImage, const float& componentW, const float& componentH)
{
    auto backColour = juce::Colours::grey;
    g.fillAll ( backColour );
    float scale = 1;
    
    float imageW = backGroundImage.getWidth();
    float imageH = backGroundImage.getHeight();
    
    if ( imageW > imageH ) { scale = componentW / imageW; }
    else { scale = componentH / imageH; }
    imageW *= scale;
    imageH *= scale;
    juce::Image temp = backGroundImage.rescaled(imageW, imageH);
    temp.multiplyAllAlphas( 0.7f );
    imageW = temp.getWidth();
    imageH = temp.getHeight();
    float imgA = imageH * imageW;
    float componentA = componentW * componentH;
    scale = 1;
    while( imgA < componentA )
    {
        if( imageW < componentW ) { scale *= componentW / imageW; }
        else if ( imageH < componentH ) { scale *= componentH / imageH; }
        imageW *= scale;
        imageH *= scale;
        imgA = imageH * imageW;
    }
    temp = temp.rescaled(imageW, imageH);
    // centre image
    imageW = temp.getWidth();
    imageH = temp.getHeight();
    float bcx = imageW / 2;
    float bcy = imageH / 2;
    float gcx = componentW / 2;
    float gcy = componentH / 2;
    g.drawImageAt ( temp, gcx - bcx,  gcy - bcy );
}

//====================================================================================
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
//        DBG(" rotate " << rotate << " " << shearX << " " << shearY );
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
//====================================================================================
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
//        DBG(" rotate " << rotate << " " << shearX << " " << shearY );
        auto rotation = juce::AffineTransform::rotation( rotate, rectRand.getX(), rectRand.getY() );
        auto shear = juce::AffineTransform::shear(shearX, shearY);
        juce::Path path;
        path.addRoundedRectangle( rectRand, CORNER_SIZE );
        path.applyTransform( shear );
        g.fillPath(path, rotation );
    }
}
//====================================================================================
inline void sjf_setTooltipLabel( juce::Component* mainComponent, const juce::String& MAIN_TOOLTIP, juce::Label& tooltipLabel  )
{
    juce::Point mouseThisTopLeft = mainComponent->getMouseXYRelative();
    if( mainComponent->reallyContains( mouseThisTopLeft, true ) )
    {
        juce::Component* const underMouse = juce::Desktop::getInstance().getMainMouseSource().getComponentUnderMouse();
        juce::TooltipClient* const ttc = dynamic_cast <juce::TooltipClient*> (underMouse);
        juce::String toolTip = MAIN_TOOLTIP;
        if (ttc != 0 && !(/*underMouse->isMouseButtonDown() ||*/ underMouse->isCurrentlyBlockedByAnotherModalComponent()))
            toolTip = ttc->getTooltip();
        tooltipLabel.setText( toolTip, juce::dontSendNotification );
//        DBG( "TOOLTIP " << toolTip );
        
    }
//    DBG("NO TOOLTIP");
}
//====================================================================================
#endif /* sjf_lookAndFeel_h */
