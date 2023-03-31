//
//  sjf_XYpad.h
//
//  Created by Simon Fay on 20/08/2022.
//

#ifndef sjf_XYpad_h
#define sjf_XYpad_h


#include <vector>
#include <JuceHeader.h>

class sjf_XYpad : public juce::Component, public juce::SettableTooltipClient
{
    //==============================================================================
    //==============================================================================
    //==============================================================================
public:
    sjf_XYpad()
    {
        setInterceptsMouseClicks(true, false);
        setSize (600, 400);
    }
    ~sjf_XYpad(){}
    //==============================================================================
    void paint (juce::Graphics& g) override
    {
        g.setColour( findColour( backgroundColourId ) );
        g.fillAll();
        
        g.setColour( findColour( outlineColourId ) );
        juce::Rectangle<float> rect = { 0, 0, (float)getWidth(), (float)getHeight() };
        g.drawRect( rect );
        
        g.setColour( findColour( circleColourID ) );
        
        rect = { m_pos[0] - 5, m_pos[1] - 5, 10, 10 };
        g.drawEllipse( rect, 2 );
    }
    //==============================================================================
    void resized() override
    {
        repaint();
    }
    //==============================================================================
    const std::array< float, 2 > getPosition()
    {
        return m_pos;
    }
    //==============================================================================
    const std::array< float, 2 > getNormalisedPosition()
    {
        std::array< float, 2 > nPos = { m_pos[0] / (float)getWidth(), m_pos[1] / (float)getHeight() };
        return nPos;
    }
    //==============================================================================
    void setNormalisedXposition( const float x )
    {
        m_pos[ 0 ] = std::fmax(0, std::fmin( x, 1.0f ) ) * (float)getWidth();
        repaint();
    }
    //==============================================================================
    void setNormalisedYposition( const float y )
    {
        m_pos[ 1 ] = std::fmax(0, std::fmin( y, 1.0f ) ) * (float)getWidth();
        repaint();
    }
    //==============================================================================
    const std::array< float, 4 > distanceFromCorners()
    {
        std::array< float, 2 > nPos = getNormalisedPosition();
        std::array< float, 4 > corners;
        corners[0] = std::sqrt( std::pow(nPos[0], 2) + std::pow(nPos[1], 2) );
        corners[1] = std::sqrt( std::pow(1.0f - nPos[0], 2) + std::pow(nPos[1], 2) );
        corners[2] = std::sqrt( std::pow(1.0f - nPos[0], 2) + std::pow(1.0f - nPos[1], 2) );
        corners[3] = std::sqrt( std::pow(nPos[0], 2) + std::pow(1.0f - nPos[1], 2) );
        for ( int i = 0; i < corners.size(); i++ )
        {
            corners[i] = std::fmax( 0, 1.0f - corners[i] );
        }
        return corners;
    }
    //==============================================================================
    enum ColourIds
    {
        backgroundColourId          = 0x1001200,
        outlineColourId             = 0x1000c00,
        circleColourID              = 0x1001310,
    };
    //==============================================================================
    std::function<void()> onMouseEvent;
private:
    void calculatePositions( const juce::MouseEvent& e )
    {
        m_pos[0] = std::fmax(std::fmin(e.position.getX(), getWidth()), 0);
        m_pos[1] = std::fmax(std::fmin(e.position.getY(), getWidth()), 0);
        repaint();
    }
    //==============================================================================
    void mouseDown (const juce::MouseEvent& e) override
    {
        calculatePositions( e );
        if ( onMouseEvent != nullptr ){ onMouseEvent(); }
    }
    //==============================================================================
    void mouseDrag(const juce::MouseEvent& e) override
    {
        calculatePositions( e );
        if ( onMouseEvent != nullptr ){ onMouseEvent(); }
    }
    
    std::array< float, 2 > m_pos = {0, 0};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (sjf_XYpad)
};
#endif /* sjf_XYpad_h */


