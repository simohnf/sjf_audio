//
//  sjf_twoValSlider.h
//
//  Created by Simon Fay on 20/08/2022.
//

#ifndef sjf_twoValSlider_h
#define sjf_twoValSlider_h

#include <vector>
#include <JuceHeader.h>
#include "sjf_audioUtilities.h"

class sjf_twoValSlider : public juce::Component, public juce::SettableTooltipClient
{
public:
    //==============================================================================
    sjf_twoValSlider()
    {
        setInterceptsMouseClicks(true, false);
        m_vals[ 0 ] = m_range[ 0 ] = 0;
        m_vals[ 1 ] = m_range[ 1 ] = 1;
        setSize (600, 400);
    }
    //==============================================================================
    ~sjf_twoValSlider()
    {
//        deleteAllChildren();
    }
    //==============================================================================
    void paint (juce::Graphics& g) override
    {
        g.setColour( findColour(backgroundColourId) );
        g.fillAll();
        g.setFont (juce::Font (16.0f));

        
        g.setColour( findColour( sliderColourID ) );
        float x, y, w, h;
        if (m_isHorizontalFlag)
        {
            x = m_vals[ 0 ]*getWidth();
            y = 0;
            w = m_vals[ 1 ]*getWidth() - x;
            h = getHeight();
        }
        else
        {
            x = 0;
            y = m_vals[ 1 ]*getHeight();
            w = getWidth();
            h = m_vals[ 0 ]*getHeight() - y;
            
        }
        g.fillRect( x, y, w, h );
        
        g.setColour ( findColour(outlineColourId) );
        g.drawRect(0, 0, getWidth(), getHeight());
    }
    //==============================================================================
    void resized() override
    {
        repaint();
    }
    //==============================================================================
    void setOutputOnMouseUpOnly( bool shouldOutputOnMouseUpOnly )
    {
        m_eventOnMouseUpOnlyFlag = shouldOutputOnMouseUpOnly;
    }
    //==============================================================================
    bool getOutputOnMouseUpOnly( )
    {
        return m_eventOnMouseUpOnlyFlag;
    }
    //==============================================================================
    void setHorizontal(bool horizontal)
    {
        if(m_isHorizontalFlag != horizontal)
        {
            m_isHorizontalFlag = horizontal;
            resized();
        }
    }
    //==============================================================================
    void setRange( double minimum, double maximum )
    {
        m_range[ 0 ] = std::fmin( minimum, maximum );
        m_range[ 1 ] = std::fmax( minimum, maximum );
    }
    //==============================================================================
    enum ColourIds
    {
        backgroundColourId          = 0x1001200,
        outlineColourId             = 0x1000c00,
        sliderColourID              = 0x1001310,
    };
    //==============================================================================
    void setMinAndMaxValues( double min, double max )
    {
        auto mn = std::fmin( min, max );
        sjf_scale< double >( mn, m_range[ 0 ], m_range[ 1 ], 0.0f, 1.0f );;
        auto mx = std::fmax( min, max );
        sjf_scale< double >( mx, m_range[ 0 ], m_range[ 1 ], 0.0f, 1.0f );;
        m_vals[ 0 ] = mn;
        m_vals[ 1 ] = mx;
        
        repaint();
    }
    //==============================================================================
    std::array< double, 2 > getMinAndMaxValues()
    {
        std::array< double, 2 > output = m_vals;
        sjf_scale< double >( output[ 0 ], 0.0f, 1.0f, m_range[ 0 ], m_range[ 1 ] );
        sjf_scale< double >( output[ 1 ], 0.0f, 1.0f, m_range[ 0 ], m_range[ 1 ] );
        return output;
    }
    //==============================================================================
    std::function<void()> onMouseEvent;
private:
    //==============================================================================
    void calulateMousePosToSliderVal( const juce::MouseEvent& e )
    {
        auto minOrMaxAndValue = findMinOrMaxAndValue( e );
//        auto val = calculatedSliderValue( e );
//        if (e.mods.isShiftDown() && e.mods.isAltDown())
//        {
//            mouseActionAndAltAndShiftLogic(  val,  theTouchedSlider );
//        }
        if( e.mods.isShiftDown() )
        {
            mouseActionAndShiftLogic( minOrMaxAndValue );
        }
//        else if( e.mods.isAltDown() )
//        {
//            mouseActionAndAltLogic( val, theTouchedSlider );
//        }
        else
        {
            m_vals[ (int)minOrMaxAndValue[ 0 ] ] = minOrMaxAndValue[ 1 ];
        }
    }
    //==============================================================================
    std::array< double, 2 > findMinOrMaxAndValue( const juce::MouseEvent& e )
    {
        auto x = e.position.getX();
        auto y = e.position.getY();
//        int theTouchedValue = minimumValue;
        double distFromMin, distFromMax;
        std::array< double, 2 > output;
        if (m_isHorizontalFlag)
        {
            distFromMin = abs( m_vals[ 0 ]*getWidth() - x );
            distFromMax = abs( m_vals[ 1 ]*getWidth() - x );
            output[ 1 ] = ( (float)x  / (float)getWidth() );
        }
        else
        {
            distFromMin = abs( m_vals[ 0 ]*getHeight() - y );
            distFromMax = abs( m_vals[ 1 ]*getHeight() - y );
            output[ 1 ] = ( (float)y  / (float)getHeight() );
        }
        
        
        output[ 0 ] = ( ( distFromMin < distFromMax ) ? minimumValue : maximumValue ) ;
        output[ 1 ] = std::fmax( std::fmin( 1, output [ 1 ]), 0 );
        DBG("two val slider " << output[ 0 ] << " " << output[ 1 ] << " x " << x << " y " << y);
        return output; // min/max normalised value
    }
    //==============================================================================
    void mouseActionAndShiftLogic( std::array< double, 2 > minOrMaxAndValue )
    {
        int valToChange = minOrMaxAndValue[ 0 ];
        auto dif = minOrMaxAndValue[ 1 ] - m_vals[ valToChange ];
        for ( int i = 0; i < m_vals.size(); i++ )
        {
            m_vals[ i ] = std::fmax( std::fmin( 1, m_vals[ i ] + dif ), 0 );
        }
    }
//    //==============================================================================
//    void mouseActionAndAltLogic( float val, int theTouchedSlider )
//    {
//
//        for (int s = 0; s < m_sliders.size(); s++)
//        {
//            auto distance = abs(theTouchedSlider - s);
//            if (distance == 0){ m_sliders[s]->setValue( val ); }
//            else
//            {
//                auto range = val - m_sliders[s]->getMinimum();
//                auto min = m_sliders[s]->getMinimum();
//                /* auto newVal = range / pow(2, distance); */
//                distance += 1;
//                auto newVal = range * ( (m_sliders.size() - distance) / (float)m_sliders.size() );
//                newVal += min;
//                m_sliders[s]->setValue( newVal );
//            }
//        }
//    }
//    //==============================================================================
//    void mouseActionAndAltAndShiftLogic( float val, int theTouchedSlider )
//    {
//
//        for (int s = 0; s < m_sliders.size(); s++)
//        {
//            auto distance = abs(theTouchedSlider - s);
//            if (distance == 0){ m_sliders[s]->setValue( val ); }
//            else
//            {
//                auto max = m_sliders[s]->getMaximum();
//                auto range = max - val;
//
//                //                auto newVal = range / pow(2, distance);
//                distance += 1;
//                auto newVal = range * ( (m_sliders.size() - distance) / (float)m_sliders.size() );
//                newVal  = max - newVal;
//                m_sliders[s]->setValue( newVal );
//            }
//        }
//    }
    //==============================================================================
    void mouseDown (const juce::MouseEvent& e) override
    {
        calulateMousePosToSliderVal(e);
        repaint();
        if ( onMouseEvent != nullptr && !m_eventOnMouseUpOnlyFlag ){ onMouseEvent(); }
    }
    //==============================================================================
    void mouseDrag(const juce::MouseEvent& e) override
    {
        calulateMousePosToSliderVal(e);
        repaint();
        if ( onMouseEvent != nullptr && !m_eventOnMouseUpOnlyFlag){ onMouseEvent(); }
    }
    //==============================================================================
    void mouseUp(const juce::MouseEvent& e) override
    {
        if ( onMouseEvent != nullptr ){ onMouseEvent(); }
    }
    
    //==============================================================================
    enum { minimumValue, maximumValue };
    //==============================================================================
private:

    std::array< double, 2 > m_vals, m_range;
    bool m_isHorizontalFlag = false, m_eventOnMouseUpOnlyFlag = true;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (sjf_twoValSlider)
};
#endif /* sjf_twoValSlider_h */

