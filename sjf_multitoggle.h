//
//  sjf_multitoggle.h
//  multiSlider
//
//  Created by Simon Fay on 20/08/2022.
//

#ifndef sjf_multitoggle_h
#define sjf_multitoggle_h

class sjf_multitoggle : public juce::Component
{
    //==============================================================================
    //==============================================================================
    //==============================================================================
    class sjf_multitoggle_LookAndFeel : public juce::LookAndFeel_V4
    {
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
        //==============================================================================
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
                g.fillRect(w*0.1f, h*0.1f, w*0.8f, h*0.8f);
            }
        };
    };
    //==============================================================================
    //==============================================================================
    //==============================================================================
public:
    //==============================================================================
    sjf_multitoggle()
    {
        setColour(tickDisabledColourId, juce::Colours::transparentBlack);
        setLookAndFeel(&thisLookAndFeel);
        setInterceptsMouseClicks(true, false);
        createButtonArray(2, 2);
        setSize (600, 400);
    }
    //==============================================================================
    ~sjf_multitoggle()
    {
        setLookAndFeel(nullptr);
        deleteAllChildren();
    }
    //==============================================================================
    void paint (juce::Graphics& g) override
    {
        g.setColour( findColour(backgroundColourId) );
        g.fillAll();
        g.setFont (juce::Font (16.0f));
        g.setColour ( findColour(outlineColourId) );
        g.drawRect(0, 0, getWidth(), getHeight());
        
        for (int b = 0; b < buttons.size(); b ++)
        {
            buttons[b]->setColour(juce::ToggleButton::tickColourId, findColour( tickColourId ));
            buttons[b]->setColour(juce::ToggleButton::tickDisabledColourId, findColour( tickDisabledColourId ));
        }
    }
    //==============================================================================
    void resized() override
    {
        auto bWidth = (float)this->getWidth() / (float)nColumns;
        auto bHeight = (float)this->getHeight() / (float)nRows;
        for (int r = 0; r < nRows; r++)
        {
            for (int c = 0; c < nColumns; c++)
            {
                auto b = c + r*nColumns;
                buttons[ b ]->setBounds(bWidth*c, bHeight*r, bWidth, bHeight);
            }
        }
    }
    //==============================================================================
    void setNumRowsAndColumns( int numRows, int numColumns )
    {
        auto newNumButtons  = numRows * numColumns;
        auto nButtons = buttons.size();
        
        if (newNumButtons < 1){ newNumButtons = 1; }
        if (newNumButtons == nButtons){ return; }
        
        std::vector<bool> temp;
        for (int b = 0; b < nButtons; b++)
        {
            temp.push_back(buttons[b]->getToggleState());
        }
        createButtonArray(numRows, numColumns);
        
        for (int b = 0; b < buttons.size(); b++)
        {
            if(b < temp.size()) { buttons[b]->setToggleState(temp[b], juce::dontSendNotification); }
        }
        resized();
    }
    //==============================================================================
    void setNumRows( int numRows)
    {
        if (numRows < 1){ numRows = 1; }
        auto newNumButtons  = numRows * nColumns;
        auto nButtons = buttons.size();
        if (newNumButtons == nButtons){ return; }
        
        std::vector<bool> temp;
        for (int b = 0; b < nButtons; b++)
        {
            temp.push_back(buttons[b]->getToggleState());
        }
        createButtonArray(numRows, nColumns);
        
        for (int b = 0; b < buttons.size(); b++)
        {
            if(b < temp.size()) { buttons[b]->setToggleState(temp[b], juce::dontSendNotification); }
        }
        resized();
    }
    //==============================================================================
    void setNumColumns( int numColumns )
    {
        if (numColumns < 1){ numColumns = 1; }
        auto newNumButtons  = nRows * numColumns;
        auto nButtons = buttons.size();
        if (newNumButtons == nButtons){ return; }
        
        std::vector<std::vector<bool>> temp;
        temp.resize(nRows);
        
        // copying to 2d array just ensures pattern stays the same when numColumns is changed
        for (int r = 0; r < nRows; r++)
        {
            temp[r].resize(nColumns);
            for (int c = 0; c < nColumns; c ++)
            {
                auto b = c + r*nColumns;
                temp[r][c] = buttons[b]->getToggleState();
            }
        }

        createButtonArray(nRows, numColumns);
        
        for (int r = 0; r < nRows; r++)
        {
            for (int c = 0; c < numColumns; c ++)
            {
                if (c < nColumns)
                {
                    auto b = c + r*numColumns;
                    buttons[b]->setToggleState(temp[r][c], juce::dontSendNotification);
                }
            }
        }
        resized();
    }
    //==============================================================================
    std::vector<bool> getRow ( int rowToGet )
    {
        std::vector<bool> temp;
        for (int c = 0; c < nColumns; c++)
        {
            temp.push_back( buttons[ c + rowToGet*nRows ]->getToggleState() );
        }
        return temp;
    }
    //==============================================================================
    std::vector<bool> getColumn ( int columnToGet )
    {
        std::vector<bool> temp;
        for (int r = 0; r < nRows; r++)
        {
            temp.push_back( buttons[ r + columnToGet*nColumns ]->getToggleState() );
        }
        return temp;
    }
    //==============================================================================
    int getNumButtons()
    {
        return buttons.size();
    }
    //==============================================================================
    float fetch(int buttonRow, int buttonIndex)
    {
        auto ind = buttonRow * buttonIndex;
        return ( buttons[ind]->getToggleState() );
    }
    
    //==============================================================================
    std::vector<float> outputList()
    {
        auto nButtons = buttons.size();
        std::vector<float> temp;
        for(int b = 0; b < nButtons; b++){ temp.push_back( buttons[b]->getToggleState() ); }
        return temp;
    }
    //==============================================================================
    enum ColourIds
    {
        backgroundColourId          = 0x1001200,
        outlineColourId        = 0x1000c00,
        tickColourId = 0x1006502,
        tickDisabledColourId = 0x1006503
    };
private:
    //==============================================================================
    int calulateMousePosToToggleNumber(const juce::MouseEvent& e)
    {
        auto pos = e.position;
        auto x = pos.getX();
        auto y = pos.getY();
        auto bWidth = getWidth()/(float)nColumns;
        auto bHeight = getHeight()/(float)nRows;
        
        for (int r = 0; r < nRows; r++)
        {
            for (int c = 0; c < nColumns; c++)
            {
                if( y >= r*bHeight && y < (r+1)*bHeight && x >= c*bWidth && x < (c+1)*bWidth )
                {
                    auto b = c + r*nColumns;
                    return b;
                }
            }
        }
        return -1;
    }
    //==============================================================================
    void mouseDown (const juce::MouseEvent& e) override
    {
        if( e.mods.isAltDown() )
        {
            for (int b = 0; b < buttons.size(); b++)
            {
                buttons[b]->setToggleState( false, juce::dontSendNotification );
                lastMouseDownToggleState = false;
            }
        }
        else if( e.mods.isShiftDown() )
        {
            for (int b = 0; b < buttons.size(); b++)
            {
                buttons[b]->setToggleState( true, juce::dontSendNotification );
                lastMouseDownToggleState = false;
            }
        }
        else
        {
            auto b = calulateMousePosToToggleNumber(e);
            if (b < 0) { return; }
            buttons[b]->setToggleState(! buttons[b]->getToggleState(), juce::dontSendNotification );
            lastMouseDownToggleState = buttons[b]->getToggleState();
        }
    }
    //==============================================================================
    void mouseDrag(const juce::MouseEvent& e) override
    {
        auto b = calulateMousePosToToggleNumber(e);
        if (b < 0) { return; }
        buttons[b]->setToggleState( lastMouseDownToggleState, juce::dontSendNotification );
    }
    //==============================================================================
    void createButtonArray(int numRows, int numColumns)
    {
        deleteAllChildren();
        buttons.clear();
        nRows = numRows;
        nColumns = numColumns;
        auto nButtons = nRows * nColumns;
        for (int b = 0; b < nButtons; b ++)
        {
            juce::ToggleButton *button = new juce::ToggleButton;
            button->setInterceptsMouseClicks(false, false);
            addAndMakeVisible(button);
            buttons.add(button);
        }
    }
    //==============================================================================
private:
    sjf_multitoggle_LookAndFeel thisLookAndFeel;
    
    juce::Array<juce::ToggleButton*> buttons;
    int nRows, nColumns;
    bool lastMouseDownToggleState;
};
#endif /* sjf_multitoggle_h */
