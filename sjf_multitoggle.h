//
//  sjf_multitoggle.h
//
//  Created by Simon Fay on 20/08/2022.
//

#ifndef sjf_multitoggle_h
#define sjf_multitoggle_h


#include <vector>
#include <JuceHeader.h>

class sjf_multitoggle : public juce::Component, public juce::SettableTooltipClient
{
    //==============================================================================
    //==============================================================================
    //==============================================================================
    class sjf_multitoggle_LookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        sjf_multitoggle_LookAndFeel(){ LookAndFeel_V4(); };
    private:
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
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (sjf_multitoggle_LookAndFeel)
    };
    //==============================================================================
    //==============================================================================
    //==============================================================================
public:
    //==============================================================================
    sjf_multitoggle()
    {
        setLookAndFeel(&m_landf);
        setColour(tickDisabledColourId, juce::Colours::transparentBlack);
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
        
        for (int b = 0; b < m_buttons.size(); b ++)
        {
            m_buttons[b]->setColour(juce::ToggleButton::tickColourId, m_buttons[b]->findColour( tickColourId ));
            m_buttons[b]->setColour(juce::ToggleButton::tickDisabledColourId, m_buttons[b]->findColour( tickDisabledColourId ));
        }
    }
    //==============================================================================
    void resized() override
    {
        auto bWidth = (float)this->getWidth() / (float)m_nColumns;
        auto bHeight = (float)this->getHeight() / (float)m_nRows;
        for (int r = 0; r < m_nRows; r++)
        {
            for (int c = 0; c < m_nColumns; c++)
            {
                auto b = c + r*m_nColumns;
                m_buttons[ b ]->setBounds(bWidth*c, bHeight*r, bWidth, bHeight);
            }
        }
    }
    //==============================================================================
    void setNumRowsAndColumns( int numRows, int numColumns )
    {
        auto newNumButtons  = numRows * numColumns;
        auto nButtons = m_buttons.size();
        
        if (newNumButtons < 1){ newNumButtons = 1; }
        if (newNumButtons == nButtons){ return; }
        
        std::vector<bool> temp;
        for (int b = 0; b < nButtons; b++)
        {
            temp.push_back(m_buttons[b]->getToggleState());
        }
        createButtonArray(numRows, numColumns);
        
        for (int b = 0; b < m_buttons.size(); b++)
        {
            if(b < temp.size()) { m_buttons[b]->setToggleState(temp[b], juce::dontSendNotification); }
        }
        resized();
    }
    //==============================================================================
    void setNumRows( int numRows)
    {
        if (numRows < 1){ numRows = 1; }
        auto newNumButtons  = numRows * m_nColumns;
        auto nButtons = m_buttons.size();
        if (newNumButtons == nButtons){ return; }
        
        std::vector<bool> temp;
        for (int b = 0; b < nButtons; b++)
        {
            temp.push_back(m_buttons[b]->getToggleState());
        }
        createButtonArray(numRows, m_nColumns);
        
        for (int b = 0; b < m_buttons.size(); b++)
        {
            if(b < temp.size()) { m_buttons[b]->setToggleState(temp[b], juce::dontSendNotification); }
        }
        resized();
    }
    //==============================================================================
    void setNumColumns( int numColumns )
    {
        if (numColumns < 1){ numColumns = 1; }
        auto newNumButtons  = m_nRows * numColumns;
        auto nButtons = m_buttons.size();
        if (newNumButtons == nButtons){ return; }
        
        std::vector<std::vector<bool>> temp;
        temp.resize(m_nRows);
        
        // copying to 2d array just ensures pattern stays the same when numColumns is changed
        for (int r = 0; r < m_nRows; r++)
        {
            temp[r].resize(m_nColumns);
            for (int c = 0; c < m_nColumns; c ++)
            {
                auto b = c + r*m_nColumns;
                temp[r][c] = m_buttons[b]->getToggleState();
            }
        }

        createButtonArray(m_nRows, numColumns);
        
        for (int r = 0; r < m_nRows; r++)
        {
            for (int c = 0; c < numColumns; c ++)
            {
                if (c < m_nColumns)
                {
                    auto b = c + r*numColumns;
                    m_buttons[b]->setToggleState(temp[r][c], juce::dontSendNotification);
                }
            }
        }
        resized();
    }
    //==============================================================================
    int getNumRows()
    {
        return m_nRows;
    }
    //==============================================================================
    int getNumColumns()
    {
        return m_nColumns;
    }
    //==============================================================================

    std::vector<bool> getRow ( int rowToGet )
    {
        std::vector<bool> temp;
        for (int c = 0; c < m_nColumns; c++)
        {
            temp.push_back( m_buttons[ rowToGet + m_nRows*c ]->getToggleState() );
        }
        return temp;
    }
    //==============================================================================
    std::vector<bool> getColumn ( int columnToGet )
    {
        std::vector<bool> temp;
        for (int r = 0; r < m_nRows; r++)
        {
            temp.push_back( m_buttons[ columnToGet + m_nColumns*r ]->getToggleState() );
        }
        return temp;
    }
    //==============================================================================
    int getNumButtons()
    {
        return m_buttons.size();
    }
    //==============================================================================
    float fetch(int buttonRow, int buttonIndex)
    {
        auto ind = buttonRow *m_buttons.size() + buttonIndex;
        return ( m_buttons[ind]->getToggleState() );
    }
    
    //==============================================================================
    std::vector<float> outputList()
    {
        auto nButtons = m_buttons.size();
        std::vector<float> temp;
        for(int b = 0; b < nButtons; b++){ temp.push_back( m_buttons[b]->getToggleState() ); }
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
    
    //==============================================================================
    void setColumnColour( int columnNumber, juce::Colour newColour )
    {
        for ( int r = 0; r < m_nRows; r++ )
        {
            auto tognum = columnNumber + m_nColumns*r;
            m_buttons[ tognum ]->setColour( tickDisabledColourId, newColour );
            m_buttons[ tognum ]->setColour( tickColourId, newColour );
        }
    }
    //==============================================================================
    void setToggleState( int row, int column, bool state )
    {
        auto toggleNumber = column + ( m_nColumns * row ); 
        m_buttons[ toggleNumber ]->setToggleState( state, juce::dontSendNotification );
    }
    
    std::function<void()> onMouseEvent;
private:
    //==============================================================================
    int calulateMousePosToToggleNumber(const juce::MouseEvent& e)
    {
        auto pos = e.position;
        auto x = pos.getX();
        auto y = pos.getY();
        auto bWidth = getWidth()/(float)m_nColumns;
        auto bHeight = getHeight()/(float)m_nRows;
        
        for (int r = 0; r < m_nRows; r++)
        {
            for (int c = 0; c < m_nColumns; c++)
            {
                if( y >= r*bHeight && y < (r+1)*bHeight && x >= c*bWidth && x < (c+1)*bWidth )
                {
                    auto b = c + r*m_nColumns;
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
            for (int b = 0; b < m_buttons.size(); b++)
            {
                m_buttons[b]->setToggleState( false, juce::dontSendNotification );
                m_lastMouseDownToggleState = false;
            }
        }
        else if( e.mods.isShiftDown() )
        {
            for (int b = 0; b < m_buttons.size(); b++)
            {
                m_buttons[b]->setToggleState( true, juce::dontSendNotification );
                m_lastMouseDownToggleState = false;
            }
        }
        else
        {
            auto b = calulateMousePosToToggleNumber(e);
            if (b < 0) { return; }
            m_buttons[b]->setToggleState(! m_buttons[b]->getToggleState(), juce::dontSendNotification );
            m_lastMouseDownToggleState = m_buttons[b]->getToggleState();
        }
        if ( onMouseEvent != nullptr ){ onMouseEvent(); }
    }
    //==============================================================================
    void mouseDrag(const juce::MouseEvent& e) override
    {
        auto b = calulateMousePosToToggleNumber(e);
        if (b < 0) { return; }
        m_buttons[b]->setToggleState( m_lastMouseDownToggleState, juce::dontSendNotification );
        if ( onMouseEvent != nullptr ){ onMouseEvent(); }
    }
    //==============================================================================
    void createButtonArray(int numRows, int numColumns)
    {
        deleteAllChildren();
        m_buttons.clear();
        m_nRows = numRows;
        m_nColumns = numColumns;
        auto nButtons = m_nRows * m_nColumns;
        for (int b = 0; b < nButtons; b ++)
        {
            juce::ToggleButton *button = new juce::ToggleButton;
            button->setInterceptsMouseClicks(false, false);
            addAndMakeVisible(button);
            m_buttons.add(button);
        }
    }

    //==============================================================================
private:
    sjf_multitoggle_LookAndFeel m_landf;
    
    juce::Array<juce::ToggleButton*> m_buttons;
    int m_nRows, m_nColumns;
    bool m_lastMouseDownToggleState;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (sjf_multitoggle)
};
#endif /* sjf_multitoggle_h */
