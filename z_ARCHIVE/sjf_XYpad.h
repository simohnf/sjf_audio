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
        juce::Rectangle<float> rect = { 0, 0, static_cast<float>( getWidth() ), static_cast<float>( getHeight() ) };
        g.drawRect( rect );
        
        if( m_drawCornerCirclesFlag ){ drawCirclesForCorners( g ); }
        
        g.setColour( findColour( circleColourID ) );
        
        rect = { m_pos[0] - 5, m_pos[1] - 5, 10, 10 };
        g.fillEllipse( rect ); //( rect, 2 );
    }
    //==============================================================================
    void resized() override
    {
        repaint();
    }
    //==============================================================================
    const std::array< float, 2 > getPosition()
    {
        std::array< float, 2 > outPos = { m_pos[ 0 ], getHeight() - m_pos[ 1 ] };
        return outPos;
    }
    //==============================================================================
    const std::array< float, 2 > getNormalisedPosition()
    {
        std::array< float, 2 > nPos = { m_pos[0] / static_cast<float>( getWidth() ), 1.0f - ( m_pos[1] / static_cast<float>( getHeight() ) ) };
        return nPos;
    }
    //==============================================================================
    void setNormalisedXposition( const float x )
    {
        m_pos[ 0 ] = std::fmax(0, std::fmin( x, 1.0f ) ) * static_cast<float>( getWidth() );
        repaint();
    }
    //==============================================================================
    void setNormalisedYposition( const float y )
    {
        m_pos[ 1 ] = ( std::fmax(0, std::fmin( 1.0f - y, 1.0f ) ) ) * static_cast<float>( getHeight() );
        repaint();
    }
    //==============================================================================
    void setNormalisedPosition( const std::array< float, 2 > pos )
    {
        m_pos[ 0 ] = std::fmax(0, std::fmin( pos[0], 1.0f ) ) * static_cast<float>( getWidth() );
        m_pos[ 1 ] = std::fmax(0, std::fmin( 1.0f - pos[1], 1.0f ) ) * static_cast<float>( getHeight() );
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
    void shouldDrawCornerCircles( const bool trueIfShouoldDrawCornerCircles )
    {
        m_drawCornerCirclesFlag = trueIfShouoldDrawCornerCircles;
        DBG("should draw corner circles");
        repaint();
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
        auto w = getWidth();
        auto h = getHeight();
        m_pos[0] = std::fmax( std::fmin(e.position.getX(), w ), 0);
        m_pos[1] = std::fmax( std::fmin(e.position.getY(), h ), 0);
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
    //==============================================================================
    void drawCirclesForCorners( juce::Graphics& g )
    {
        DBG("!!!!! draw corner circles !!!!");
        static const int nCorners = 4;
        static constexpr std::array< float, nCorners*2 > corners = { 0, 0, 1, 0, 1, 1, 0, 1 }; 
        constexpr auto randomColours = sjf_matrixOfRandomFloats< float, nCorners, 4 >();
        juce::Rectangle< float > rect;
        float w = getWidth();
        float h = getHeight();
        float tWidth = w*2.0f;
        float tHeight = h*2.0f;
        float alpha = 0.1;
        for ( int i = 0; i < nCorners; i++)
        {
            int red = randomColours.getValue(i, 0) * 255;
            int green = randomColours.getValue(i, 1)  * 255;
            int blue = randomColours.getValue(i, 2) * 255;
//            float alpha = randomColours.getValue(i, 3) * 0.3;
            
            auto randColour = juce::Colour( red, green, blue );
            randColour = randColour.withAlpha(alpha);
            DBG("COLOUR " << red << " " << green << " " << blue << " " << alpha);
            g.setColour( randColour );
            
            rect = { corners[ i*2 ]*w - w, corners[ i*2 + 1 ]*h - h, tWidth, tHeight };
            DBG("RECTANGLE " << rect.getX() << " " << rect.getY() << " " << rect.getRight() << " " << rect.getBottom() << " " );
            g.fillEllipse( rect );
        }
    }
    //==============================================================================
    
    bool m_drawCornerCirclesFlag = false;
    std::array< float, 2 > m_pos = {0, 0};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (sjf_XYpad)
};
#endif /* sjf_XYpad_h */


