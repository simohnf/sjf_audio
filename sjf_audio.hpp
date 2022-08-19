//
//  sjf_audio.hpp
//  
//
//  Created by Simon Fay on 22/07/2022.
//

#ifndef sjf_audio_hpp
#define sjf_audio_hpp

#include <stdio.h>
#include <JuceHeader.h>
#include <vector>

#define PI 3.14159265
//#define outlineColour juce::LookAndFeel_V4::ColourScheme::UIColour::outline
//==============================================================================
/**
 */
class sjf_numBox : public juce::Slider
/* Copied from https://suzuki-kengo.dev/posts/numberbox */
{
public:
    sjf_numBox()
    {
        setSliderStyle (juce::Slider::LinearBarVertical);
        setColour (juce::Slider::trackColourId, juce::Colours::transparentWhite);
        setTextBoxIsEditable (true);
        setVelocityBasedMode (true);
        setVelocityModeParameters (0.5, 1, 0.09, false);
        setDoubleClickReturnValue (true, 50.0);
        setWantsKeyboardFocus (true);
        onValueChange = [&]()
        {
            if (getValue() < 10)
                setNumDecimalPlacesToDisplay(2);
            else if (10 <= getValue() && getValue() < 100)
                setNumDecimalPlacesToDisplay(1);
            else
                setNumDecimalPlacesToDisplay(0);
        };
    };
    ~sjf_numBox(){};
    
    void paint(juce::Graphics& g) override
    {
        if (hasKeyboardFocus (false))
        {
            auto bounds = getLocalBounds().toFloat();
            auto h = bounds.getHeight();
            auto w = bounds.getWidth();
            auto len = juce::jmin (h, w) * 0.15f;
            auto thick  = len / 1.8f;
            
            g.setColour (findColour (juce::Slider::textBoxOutlineColourId));
            
            // Left top
            g.drawLine (0.0f, 0.0f, 0.0f, len, thick);
            g.drawLine (0.0f, 0.0f, len, 0.0f, thick);
            
            // Left bottom
            g.drawLine (0.0f, h, 0.0f, h - len, thick);
            g.drawLine (0.0f, h, len, h, thick);
            
            // Right top
            g.drawLine (w, 0.0f, w, len, thick);
            g.drawLine (w, 0.0f, w - len, 0.0f, thick);
            
            // Right bottom
            g.drawLine (w, h, w, h - len, thick);
            g.drawLine (w, h, w - len, h, thick);
        }
    };
};
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
class sjf_Label : public juce::Label
{
    juce::Colour outlineColour;
public:
    sjf_Label(){
        outlineColour = juce::Colours::grey;//findColour( juce::LookAndFeel_V4::ColourScheme::getUIColour (juce::LookAndFeel_V4::ColourScheme::UIColour::outline) );
        this->setColour(juce::Label::outlineColourId, outlineColour);
    };
    ~sjf_Label(){};
private:
    void componentMovedOrResized (Component& component, bool /*wasMoved*/, bool /*wasResized*/) override
    {
        auto& lf = getLookAndFeel();
        auto f = lf.getLabelFont (*this);
        auto borderSize = lf.getLabelBorderSize (*this);
        
        if ( isAttachedOnLeft() )
        {
            auto width = juce::jmin (juce::roundToInt (f.getStringWidthFloat (getTextValue().toString()) + 0.5f)
                               + borderSize.getLeftAndRight(),
                               component.getX());
            
            setBounds (component.getX() - width, component.getY(), width, component.getHeight());
        }
        else
        {
            auto height = borderSize.getTopAndBottom() + 2 + juce::roundToInt (f.getHeight() + 0.5f);
            
            setBounds (component.getX(), component.getY() - height, component.getWidth(), height);
        }
    }
};


class sjf_LookAndFeel : public juce::LookAndFeel_V4
{
public:
    juce::Colour outlineColour;
    
    sjf_LookAndFeel(){
        outlineColour = juce::Colours::grey;//sjf_LookAndFeel::findColour( juce::ComboBox::outlineColourId );
//        setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colours::red);
//        setColour( sjf_Label::outlineColourId, outlineColour );
    }
    ~sjf_LookAndFeel(){};
    
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
        
        auto thumbWidth = lineW * 1.0f;
        juce::Point<float> thumbPoint (bounds.getCentreX() + arcRadius * std::cos (toAngle - juce::MathConstants<float>::halfPi),
                                 bounds.getCentreY() + arcRadius * std::sin (toAngle - juce::MathConstants<float>::halfPi));
        
        g.setColour (slider.findColour (juce::Slider::thumbColourId));
        g.fillEllipse (juce::Rectangle<float> (thumbWidth, thumbWidth).withCentre (thumbPoint));
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
    
};



//==============================================================================
inline
float phaseEnv( float phase, float period, float envLen){
    auto nSegments = period / envLen;
    auto segmentPhase = phase * nSegments;
    auto rampUp = segmentPhase;
    if (rampUp > 1) {rampUp = 1;}
    else if (rampUp < 0) {rampUp = 0;}
    
    float rampDown = segmentPhase - (nSegments - 1);
    if (rampDown > 1) {rampDown = 1;}
    else if (rampDown < 0) {rampDown = 0;}
    rampDown *= -1;
    //    return rampUp+rampDown; // this would give linear fade
    return sin( PI* (rampUp+rampDown)/2 ); // this gives a smooth sinewave based fade
}
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================

inline
float cubicInterpolate(std::vector<float> buffer, float read_pos)
{
    auto bufferSize = buffer.size();
    double y0; // previous step value
    double y1; // this step value
    double y2; // next step value
    double y3; // next next step value
    double mu; // fractional part between step 1 & 2
    
    float findex = read_pos;
    if(findex < 0){ findex+= bufferSize;}
    else if(findex > bufferSize){ findex-= bufferSize;}
    
    int index = findex;
    mu = findex - index;
    
    if (index == 0)
    {
        y0 = buffer[ bufferSize - 1 ];
    }
    else
    {
        y0 = buffer[ index - 1 ];
    }
    y1 = buffer[ index % bufferSize ];
    y2 = buffer[ (index + 1) % bufferSize ];
    y3 = buffer[ (index + 2) % bufferSize ];
    double a0,a1,a2,a3,mu2;
    
    mu2 = mu*mu;
    a0 = y3 - y2 - y0 + y1;
    a1 = y0 - y1 - a0;
    a2 = y2 - y0;
    a3 = y1;
    
    return (a0*mu*mu2 + a1*mu2 + a2*mu + a3);
}
//==============================================================================
inline
float cubicInterpolate(juce::AudioBuffer<float>& buffer, int channel, float read_pos)
{
    auto bufferSize = buffer.getNumSamples();
    double y0; // previous step value
    double y1; // this step value
    double y2; // next step value
    double y3; // next next step value
    double mu; // fractional part between step 1 & 2
    
    float findex = read_pos;
    if(findex < 0){ findex+= bufferSize;}
    else if(findex > bufferSize){ findex-= bufferSize;}
    
    int index = findex;
    mu = findex - index;
    
    if (index == 0)
    {
        y0 = buffer.getSample(channel, bufferSize - 1);
    }
    else
    {
        y0 = buffer.getSample(channel, index - 1);
    }
    y1 = buffer.getSample(channel, index % bufferSize);
    y2 = buffer.getSample(channel, (index + 1) % bufferSize);
    y3 = buffer.getSample(channel, (index + 2) % bufferSize);
    double a0,a1,a2,a3,mu2;
    
    mu2 = mu*mu;
    a0 = y3 - y2 - y0 + y1;
    a1 = y0 - y1 - a0;
    a2 = y2 - y0;
    a3 = y1;
    
    return (a0*mu*mu2 + a1*mu2 + a2*mu + a3);
}
//==============================================================================
inline
float fourPointInterpolatePD(juce::AudioBuffer<float>& buffer, int channel, float read_pos)
{
    auto bufferSize = buffer.getNumSamples();
    double y0; // previous step value
    double y1; // this step value
    double y2; // next step value
    double y3; // next next step value
    double mu; // fractional part between step 1 & 2
    
    float findex = read_pos;
    if(findex < 0){ findex+= bufferSize;}
    else if(findex > bufferSize){ findex-= bufferSize;}
    
    int index = findex;
    mu = findex - index;
    
    if (index == 0)
    {
        y0 = buffer.getSample(channel, bufferSize - 1);
    }
    else
    {
        y0 = buffer.getSample(channel, index - 1);
    }
    y1 = buffer.getSample(channel, index % bufferSize);
    y2 = buffer.getSample(channel, (index + 1) % bufferSize);
    y3 = buffer.getSample(channel, (index + 2) % bufferSize);
    
    auto y2minusy1 = y2-y1;
    return y1 + mu * (y2minusy1 - 0.1666667f * (1.0f - mu) * ( (y3 - y0 - 3.0f * y2minusy1) * mu + (y3 + 2.0f*y0 - 3.0f*y1) ) );
}
//==============================================================================
inline
float linearInterpolate(juce::AudioBuffer<float>& buffer, int channel, float read_pos)
{
    auto bufferSize = buffer.getNumSamples();
    double y1; // this step value
    double y2; // next step value
    double mu; // fractional part between step 1 & 2
    
    float findex = read_pos;
    if(findex < 0){ findex+= bufferSize;}
    else if(findex > bufferSize){ findex-= bufferSize;}
    
    int index = findex;
    mu = findex - index;
    
    y1 = buffer.getSample(channel, index % bufferSize);
    y2 = buffer.getSample(channel, (index + 1) % bufferSize);
    
    return y1 + mu*(y2-y1) ;
}
//==============================================================================
inline
float fourPointFourthOrderOptimal(juce::AudioBuffer<float>& buffer, int channel, float read_pos)
{
    //    Copied from Olli Niemitalo - Polynomial Interpolators for High-Quality Resampling of Oversampled Audio
    auto bufferSize = buffer.getNumSamples();
    double y0; // previous step value
    double y1; // this step value
    double y2; // next step value
    double y3; // next next step value
    double mu; // fractional part between step 1 & 2
    
    float findex = read_pos;
    if(findex < 0){ findex+= bufferSize;}
    else if(findex > bufferSize){ findex-= bufferSize;}
    
    int index = findex;
    mu = findex - index;
    
    if (index == 0)
    {
        y0 = buffer.getSample(channel, bufferSize - 1);
    }
    else
    {
        y0 = buffer.getSample(channel, index - 1);
    }
    y1 = buffer.getSample(channel, index % bufferSize);
    y2 = buffer.getSample(channel, (index + 1) % bufferSize);
    y3 = buffer.getSample(channel, (index + 2) % bufferSize);
    
    
    // Optimal 2x (4-point, 4th-order) (z-form)
    float z = mu - 1/2.0;
    float even1 = y2+y1, odd1 = y2-y1;
    float even2 = y3+y0, odd2 = y3-y0;
    float c0 = even1*0.45645918406487612 + even2*0.04354173901996461;
    float c1 = odd1*0.47236675362442071 + odd2*0.17686613581136501;
    float c2 = even1*-0.253674794204558521 + even2*0.25371918651882464;
    float c3 = odd1*-0.37917091811631082 + odd2*0.11952965967158000;
    float c4 = even1*0.04252164479749607 + even2*-0.04289144034653719;
    return (((c4*z+c3)*z+c2)*z+c1)*z+c0;
    
}
//==============================================================================
inline
float cubicInterpolateGodot(juce::AudioBuffer<float>& buffer, int channel, float read_pos)
{
    //    Copied from Olli Niemitalo - Polynomial Interpolators for High-Quality Resampling of Oversampled Audio
    auto bufferSize = buffer.getNumSamples();
    double y0; // previous step value
    double y1; // this step value
    double y2; // next step value
    double y3; // next next step value
    double mu; // fractional part between step 1 & 2
    
    float findex = read_pos;
    if(findex < 0){ findex+= bufferSize;}
    else if(findex > bufferSize){ findex-= bufferSize;}
    
    int index = findex;
    mu = findex - index;
    
    if (index == 0)
    {
        y0 = buffer.getSample(channel, bufferSize - 1);
    }
    else
    {
        y0 = buffer.getSample(channel, index - 1);
    }
    y1 = buffer.getSample(channel, index % bufferSize);
    y2 = buffer.getSample(channel, (index + 1) % bufferSize);
    y3 = buffer.getSample(channel, (index + 2) % bufferSize);
    double a0,a1,a2,a3,mu2;
    mu2 = mu*mu;
    
    a0 = 3 * y1 - 3 * y2 + y3 - y0;
    a1 = 2 * y0 - 5 * y1 + 4 * y2 - y3;
    a2 = y2 - y0;
    a3 = 2 * y1;
    
    return (a0 * mu * mu2 + a1 * mu2 + a2 * mu + a3) / 2;
}
//==============================================================================
inline
float cubicInterpolateHermite(juce::AudioBuffer<float>& buffer, int channel, float read_pos)
{
    auto bufferSize = buffer.getNumSamples();
    double y0; // previous step value
    double y1; // this step value
    double y2; // next step value
    double y3; // next next step value
    double mu; // fractional part between step 1 & 2
    
    float findex = read_pos;
    if(findex < 0){ findex+= bufferSize;}
    else if(findex > bufferSize){ findex-= bufferSize;}
    
    int index = findex;
    mu = findex - index;
    
    if (index == 0)
    {
        y0 = buffer.getSample(channel, bufferSize - 1);
    }
    else
    {
        y0 = buffer.getSample(channel, index - 1);
    }
    y1 = buffer.getSample(channel, index % bufferSize);
    y2 = buffer.getSample(channel, (index + 1) % bufferSize);
    y3 = buffer.getSample(channel, (index + 2) % bufferSize);
    double a0,a1,a2,a3,mu2;
    mu2 = mu*mu;
    
    a0 = y1;
    a1 = 0.5f * (y2 - y0);
    a2 = y0 - (2.5f * y1) + (2 * y2) - (0.5f * y3);
    a3 = (0.5f * (y3 - y0)) + (1.5F * (y1 - y2));
    return (((((a3 * mu) + a2) * mu) + a1) * mu) + a0;
}
//==============================================================================

inline
float rand01()
{
    return float( rand() ) / float( RAND_MAX );
}

//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================

class sjf_phasor{
public:
    sjf_phasor() { calculateIncrement() ; };
    sjf_phasor(float sample_rate, float f) { initialise(sample_rate, f); };
    
    ~sjf_phasor() {};
    
    void initialise(float sample_rate, float f)
    {
        SR = sample_rate;
        setFrequency( f );
    };
    
    void setSampleRate( float sample_rate)
    {
        SR = sample_rate;
        calculateIncrement();
    }
    void setFrequency(float f)
    {
        frequency = f;
        
        if (frequency >= 0)
        {
            negFreq = false;
            increment = frequency / SR ;
        }
        else
        {
            negFreq = true;
            increment = -1*frequency / SR ;
        }
        
        
        
    };
    
    float getFrequency(){ return frequency ;};
    
    float output()
    {
        if (!negFreq)
        {
            float p = position;
            position += increment;
            if (position >= 1){ position -= 1; }
            return p;
        }
        else
        {
            float p = position;
            position += increment;
            while (position >= 1){ position -= 1; }
            return 1 - p;
        }
    };
    
    void setPhase(float p)
    {
        if (p < 0) {p = 0;}
        else if (p > 1){ p = 1 ;}
        position = p;
    };
    float getPhase(){
        return position;
    }
private:
    void calculateIncrement(){ increment = ( frequency / SR ); };
    
    float frequency = 440;
    float SR = 44100;
    float increment;
    float position = 0;
    bool negFreq = false;
};

//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================

inline
void addBuffers(juce::AudioBuffer<float>& bufferToAddTo, juce::AudioBuffer<float>& bufferToAddFrom, float gainOfBuffer1, float gainOfBuffer2){
    float s1, s2;
    for (int channel = 0; channel < bufferToAddTo.getNumChannels() ; ++channel)
    {
        for (int index = 0; index < bufferToAddTo.getNumSamples(); index++){
            s1 = bufferToAddTo.getSample(channel, index) * gainOfBuffer1;
            s2 = bufferToAddFrom.getSample(channel, index) * gainOfBuffer2;
            
            bufferToAddTo.setSample(channel, index, (s1 + s2) );
        }
    }
}

//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================

class sjf_oscillator {
    
public:
    sjf_oscillator(){
        waveTable.resize(waveTableSize);
        setSine();
    };
    ~sjf_oscillator(){};
    
    void setFrequency(float f)
    {
        initialise(SR, f);
    };
    
    void initialise(int sampleRate, float f)
    {
        SR = sampleRate;
        frequency = f;
        read_increment = frequency * waveTableSize / SR ;
    };
    
    void setSine(){
        for (int index = 0; index< waveTableSize; index++)
        {
            waveTable[index] = sin( index * 2 * PI / waveTableSize ) ;
        }
    };
    
    std::vector<float> outputBlock(int numSamples)
    {
        outBuff.resize( numSamples ) ;
        for ( int index = 0; index < numSamples; index++ )
        {
            outBuff[index] = cubicInterpolate(waveTable, read_pos);
            read_pos += read_increment;
            if (read_pos >= waveTableSize)
            {
                read_pos -= waveTableSize;
            }
        }
        return outBuff;
    };
    
    std::vector<float> outputBlock(int numSamples, float gain)
    {
        outBuff.resize( numSamples ) ;
        for ( int index = 0; index < numSamples; index++ )
        {
            outBuff[index] = cubicInterpolate(waveTable, read_pos) * gain;
            read_pos += read_increment;
            if (read_pos >= waveTableSize)
            {
                read_pos -= waveTableSize;
            }
        }
        return outBuff;
    };
    
    float outputSample(int numSamples)
    {
        float out = cubicInterpolate(waveTable, read_pos);
        read_pos += numSamples * read_increment;
        if (read_pos >= waveTableSize)
        {
            read_pos -= waveTableSize;
        }
        return out;
    };
    
private:
    float waveTableSize = 512;
    float SR = 44100;
    float read_pos = 0;
    float frequency = 440;
    float read_increment = ( frequency * SR ) / waveTableSize;
    std::vector<float> waveTable, outBuff;
};

//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================

class sjf_smoothValue {
    
public:
    
    ~sjf_smoothValue(){};
    float smooth (float input)
    {
        float out = ( (1 - alpha ) * preOut ) + ( alpha * ( input +  preInput)/2 ) ;
        preOut = out;
        preInput = input;
        return out;
    }
    
    void setAlpha (float a){
        if (a < 0){ alpha = 0; }
        else if (a < 1){ alpha = 1; }
        else { alpha = a; }
    }
    
private:
    float preOut, preInput;
    float alpha = 0.0001;
};

//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================

class sjf_overdrive {
    
public:
    ~sjf_overdrive(){};
    void drive(juce::AudioBuffer<float>& buffer, float gain)
    {
        for (int index = 0; index < buffer.getNumSamples(); index++)
        {
            for (int channel = 0; channel < buffer.getNumChannels(); channel++)
            {
                buffer.setSample(channel, index, tanh( buffer.getSample(channel, index) * gain ) );
            }
        }
    }
        
};

//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================

class sjf_delayLine {
    
public:
    sjf_delayLine()
    {
        delayBuffer.setSize(2, SR * del_buff_len );
        delayBuffer.clear();
        
        delL.reset( SR, 0.02f ) ;
        delR.reset( SR, 0.02f ) ;
        
        delL.setCurrentAndTargetValue( 500.0f ) ;
        delR.setCurrentAndTargetValue( 500.0f ) ;
    };
    //==============================================================================
    virtual ~sjf_delayLine() {};
    //==============================================================================
    virtual void intialise( int sampleRate , int totalNumInputChannels, int totalNumOutputChannels, int samplesPerBlock)
    {
        if (sampleRate > 0 ) { SR = sampleRate; }
        delayBuffer.setSize( totalNumOutputChannels, SR * del_buff_len );
        delayBuffer.clear();
        
        delL.reset( SR, 0.02f ) ;
        delR.reset( SR, 0.02f ) ;
    };
    //==============================================================================
    void writeToDelayBuffer(juce::AudioBuffer<float>& sourceBuffer, float gain)
    {
        auto bufferSize = sourceBuffer.getNumSamples();
        auto delayBufferSize = delayBuffer.getNumSamples();
        for (int index = 0; index < bufferSize; index++)
        {
            auto wp = (write_pos + index) % delayBufferSize;
            for (int channel = 0; channel < sourceBuffer.getNumChannels(); channel++)
            {
                delayBuffer.setSample(channel % delayBuffer.getNumChannels(), wp, (sourceBuffer.getSample(channel, index) * gain) );
            }
        }
    };
    //==============================================================================
    void copyFromDelayBuffer(juce::AudioBuffer<float>& destinationBuffer, float gain)
    {
        auto bufferSize = destinationBuffer.getNumSamples();
        auto delayBufferSize = delayBuffer.getNumSamples();
        auto numChannels = destinationBuffer.getNumChannels();
        auto delTimeL = delL.getCurrentValue() * SR / 1000.0f;
        auto delTimeR = delR.getCurrentValue() * SR / 1000.0f;
        for (int index = 0; index < bufferSize; index++)
        {
            for (int channel = 0; channel < numChannels; channel++)
            {
                float channelReadPos;
                if( channel == 0 ) { channelReadPos = write_pos - delTimeL + index; }
                else if(channel == 1) { channelReadPos =  write_pos - delTimeR + index; }
                else { return; }
                while ( channelReadPos < 0 ) { channelReadPos += delayBufferSize; }
                while (channelReadPos >= delayBufferSize) { channelReadPos -= delayBufferSize; }
                auto val = cubicInterpolate(delayBuffer, channel, channelReadPos) * gain;
                destinationBuffer.addSample(channel, index, val );
            }
            delTimeL = delL.getNextValue() * SR / 1000.0f;
            delTimeR = delR.getNextValue() * SR / 1000.0f;
        }
    };
    //==============================================================================
    void addToDelayBuffer (juce::AudioBuffer<float>& sourceBuffer, float gain)
    {
        auto bufferSize = sourceBuffer.getNumSamples();
        auto delayBufferSize = delayBuffer.getNumSamples();
        
        for (int channel = 0; channel < sourceBuffer.getNumChannels(); ++channel)
        {
            for (int index = 0; index < bufferSize; index ++)
            {
                float value = sourceBuffer.getSample(channel, index) * gain;
                delayBuffer.addSample(channel, ( (write_pos + index) % delayBufferSize ) , value );
            }
        }
    };
    //==============================================================================
    void addToDelayBuffer (juce::AudioBuffer<float>& sourceBuffer, float gain1, float gain2)
    {
        auto bufferSize = sourceBuffer.getNumSamples();
        auto delayBufferSize = delayBuffer.getNumSamples();
        
        for (int channel = 0; channel < sourceBuffer.getNumChannels(); ++channel)
        {
            float gain;
            if (channel == 0){ gain = gain1; }
            else{ gain = gain2; }
            for (int index = 0; index < bufferSize; index ++)
            {
                float value = sourceBuffer.getSample(channel, index) * gain;
                delayBuffer.addSample(channel, ( (write_pos + index) % delayBufferSize ) , value );
            }
        }
    };
    //==============================================================================
    void updateBufferPositions(int bufferSize)
    {
        auto delayBufferSize = delayBuffer.getNumSamples();
        //    Update write position ensuring it stays within size of delay buffer
        write_pos += bufferSize;
        write_pos %= delayBufferSize;
    };
    //==============================================================================
    // set and retrieve parameters
    void setDelTimeL( float delLMS) { delL.setTargetValue( delLMS ); }
    void setDelTimeR( float delRMS) { delR.setTargetValue( delRMS ); }
    float getDelTimeL( ) { return delL.getTargetValue( ); }
    float getDelTimeR( ) { return delR.getTargetValue( ); }
    void clearBuffer() { delayBuffer.clear(); }
    //==============================================================================
    
    
protected:
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> delL, delR;
    juce::AudioBuffer<float> delayBuffer; // This is the circular buffer for the delay
    
    int SR = 44100;
    int write_pos = 0; // this is the index to write to in the "delayBuffer"
    const int del_buff_len = 3; // Maximum delay time equals 2 seconds plus 1 second for safety with stereo spread increase
};

//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================

class sjf_pitchShifter : public sjf_delayLine
{
public:
    sjf_pitchShifter(){};
    ~sjf_pitchShifter(){};
    
    void intialise( int sampleRate , int totalNumInputChannels, int totalNumOutputChannels, int samplesPerBlock) override
    {
        if (sampleRate > 0 ) { SR = sampleRate; }
        delayBuffer.setSize( totalNumOutputChannels, SR * del_buff_len );
        delayBuffer.clear();
        delL.reset( SR, 0.1f ) ;
        delR.reset( SR, 0.1f ) ;
        
        pitchPhasorL.initialise(SR, 0);
        pitchPhasorR.initialise(SR, 0);
        
        phaseResetL.reset(SR, 0.02f);
        phaseResetR.reset(SR, 0.02f);
    }
    
    void copyFromDelayBufferWithPitchShift(juce::AudioBuffer<float>& destinationBuffer, float gain, float transpositionSemiTonesL, float transpositionSemiTonesR)
    {
        auto bufferSize = destinationBuffer.getNumSamples();
        auto delayBufferSize = delayBuffer.getNumSamples();
        auto numChannels = destinationBuffer.getNumChannels();
        auto delTimeL = delL.getCurrentValue() * SR / 1000.0f;
        auto delTimeR = delR.getCurrentValue() * SR / 1000.0f;
        
        auto pitchShiftWindowSize = 20.0f; // ms
        auto pitchShiftWindowSamples = SR* pitchShiftWindowSize/1000.0f;
        auto transpositionRatioL = pow(2.0f, transpositionSemiTonesL/12.0f);
        auto transpositionRatioR = pow(2.0f, transpositionSemiTonesR/12.0f);
        auto fL = -1*(transpositionRatioL - 1) / (pitchShiftWindowSize*0.001);
        auto fR = -1*(transpositionRatioR - 1) / (pitchShiftWindowSize*0.001);
        pitchPhasorL.setFrequency( fL );
        if (fL == 0.0f && lastTranspositionL != transpositionSemiTonesL){
            phaseResetL.setCurrentAndTargetValue(pitchPhasorL.getPhase());
            phaseResetL.setTargetValue(0.0f);
            pitchPhasorL.setPhase(phaseResetL.getCurrentValue());
        }
        
        pitchPhasorR.setFrequency( fR );
        if (fR == 0.0f && lastTranspositionR != transpositionSemiTonesR){
            phaseResetR.setCurrentAndTargetValue(pitchPhasorR.getPhase());
            phaseResetR.setTargetValue(0.0f);
            pitchPhasorR.setPhase(phaseResetR.getCurrentValue());
        }
        for (int index = 0; index < bufferSize; index++)
        {
            if(fL == 0.0f){ pitchPhasorL.setPhase( phaseResetL.getNextValue() ); }
            if(fR == 0.0f){ pitchPhasorR.setPhase( phaseResetR.getNextValue() ); }
            for (int channel = 0; channel < numChannels; channel++)
            {
                if(channel < 0 || channel > 1){ return; }
                float delTime, phase;
                if (channel == 0){
                    delTime = delTimeL;
                    phase = pitchPhasorL.output();
                }
                else{     delTime = delTimeR;
                    phase = pitchPhasorR.output();
                }
                auto val = pitchShiftOutputCalculations(channel, index, delTime, phase, pitchShiftWindowSamples, delayBufferSize, gain);
                destinationBuffer.setSample(channel, index, val );
                
                phase += 0.5;
                while ( phase >= 1.0f ){ phase -= 1.0f; }
                while ( phase < 0.0f ){ phase += 1.0f; }
                val = pitchShiftOutputCalculations(channel, index, delTime, phase, pitchShiftWindowSamples, delayBufferSize, gain);
                destinationBuffer.addSample(channel, index, val );
            }
            delTimeL = delL.getNextValue() * SR / 1000.0f;
            delTimeR = delR.getNextValue() * SR / 1000.0f;
        }
        lastTranspositionL = transpositionSemiTonesL;
        lastTranspositionR = transpositionSemiTonesR;
    };
    
    
private:
    sjf_phasor pitchPhasorL, pitchPhasorR;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> phaseResetL, phaseResetR;
    float lastTranspositionL, lastTranspositionR;
    
    float pitchShiftOutputCalculations(int channel, int index, float delTime, float phase, float pitchShiftWindowSamples, float delayBufferSize, float gain ){
        auto channelReadPos = write_pos - (delTime + ( pitchShiftWindowSamples * phase )) + index;
        auto amp = sin( PI * phase );
        while ( channelReadPos < 0 ) { channelReadPos += delayBufferSize; }
        while (channelReadPos >= delayBufferSize) { channelReadPos -= delayBufferSize; }
        auto val = cubicInterpolate(delayBuffer, channel, channelReadPos) * gain;
        val *= amp;
        return val;
    }
};

//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================

#endif /* sjf_audio_hpp */
