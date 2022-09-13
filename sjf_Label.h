//
//  sjf_Label.h
//
//  Created by Simon Fay on 24/08/2022.
//

#ifndef sjf_Label_h
#define sjf_Label_h

#include <JuceHeader.h>

class sjf_label : public juce::Label
{
    juce::Colour outlineColour;
public:
    sjf_label(){
        outlineColour = juce::Colours::grey;
        this->setColour(juce::Label::outlineColourId, outlineColour);
    };
    ~sjf_label(){};
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
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (sjf_label)
};

#endif /* sjf_Label_h */
