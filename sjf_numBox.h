//
//  sjf_numBox.h
//
//  Created by Simon Fay on 24/08/2022.
//

#ifndef sjf_numBox_h
#define sjf_numBox_h

#include <JuceHeader.h>

class sjf_numBox : public juce::Slider
/* Copied from https://suzuki-kengo.dev/posts/numberbox */
{
public:
    sjf_numBox()
    {
        setSliderStyle (juce::Slider::LinearBarVertical);
//        setColour (juce::Slider::trackColourId, juce::Colours::transparentWhite);
        setTextBoxIsEditable (true);
        setVelocityBasedMode (true);
        setVelocityModeParameters (0.5, 1, 0.01, true);
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
    
//    void mouseEnter(const juce::MouseEvent &e) override
//    {
//        DBG("mouse is over this");
//    }
};

#endif /* sjf_numBox_h */
