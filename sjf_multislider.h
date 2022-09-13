//
//  sjf_multislider.h
//
//  Created by Simon Fay on 20/08/2022.
//

#ifndef sjf_multislider_h
#define sjf_multislider_h

#include <vector>
#include <JuceHeader.h>

class sjf_multislider : public juce::Component
{
public:
    //==============================================================================
    sjf_multislider()
    {
        setInterceptsMouseClicks(true, false);
        createSliderArray(4); // just default to 4 sliders because why not
        setSize (600, 400);
    }
    //==============================================================================
    ~sjf_multislider()
    {
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
        
        auto nSliders = m_sliders.size();
        for (int s = 0; s < nSliders; s++)
        {
            m_sliders[s]->setColour( juce::Slider::textBoxOutlineColourId, juce::Colours::black.withAlpha(0.0f) ) ;
            m_sliders[s]->setColour( juce::Slider::trackColourId, findColour( sliderColourID ) );
        }
    }
    //==============================================================================
    void resized() override
    {
        auto nSliders = m_sliders.size();
        for (int s = 0; s < nSliders; s++)
        {
            if (!m_isHorizontalFlag)
            {
                auto sWidth = (float)this->getWidth() / (float)nSliders;
                m_sliders[s]->setSliderStyle(juce::Slider::LinearBarVertical);
                m_sliders[s]->setBounds(sWidth*s, 0.0f, sWidth, getHeight());
            }
            else
            {
                auto sHeight = (float)getHeight()/(float)nSliders;
                m_sliders[s]->setSliderStyle(juce::Slider::LinearBar);
                m_sliders[s]->setBounds(0.0f, sHeight*s, getWidth(), sHeight);
            }
        }
    }
    //==============================================================================
    void setNumSliders( int numSliders )
    {
        auto nSliders = m_sliders.size();
        if (numSliders < 1){ numSliders = 1; }
        if (numSliders == nSliders){ return; }
        
        std::vector<float> temp;
        for(int s = 0; s < nSliders; s++){ temp.push_back(m_sliders[s]->getValue()); }
        
        createSliderArray(numSliders);
        
        for(int s = 0; s < numSliders; s++)
        {
            if(s < temp.size()) { m_sliders[s]->setValue(temp[s]); }
        }
        resized();
    }
    //==============================================================================
    int getNumSliders()
    {
        return m_sliders.size();
    }
    //==============================================================================
    float fetch(int sliderIndex)
    {
        return ( m_sliders[sliderIndex]->getValue() );
    }
    //==============================================================================
    void setRange(float min, float max)
    {
        for (int s = 0; s < m_sliders.size(); s ++) { m_sliders[s]->setRange(min, max); }
    }
    //==============================================================================
    std::vector<float> outputList()
    {
        auto nSliders = m_sliders.size();
        std::vector<float> temp;
        for(int s = 0; s < nSliders; s++){ temp.push_back(m_sliders[s]->getValue()); }
        return temp;
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
    enum ColourIds
    {
        backgroundColourId          = 0x1001200,
        outlineColourId             = 0x1000c00,
        sliderColourID              = 0x1001310,
    };
    //==============================================================================
private:
    void calulateMousePosToSliderVal( const juce::MouseEvent& e )
    {
        auto theTouchedSlider = findTouchedSlider( e );
        auto val = calculatedSliderValue( e );
        if (e.mods.isShiftDown() && e.mods.isAltDown())
        {
            mouseActionAndAltAndShiftLogic(  val,  theTouchedSlider );
        }
        else if( e.mods.isShiftDown() )
        {
            mouseActionAndShiftLogic( val );
        }
        else if( e.mods.isAltDown() )
        {
            mouseActionAndAltLogic( val, theTouchedSlider );
        }
        else
        {
            m_sliders[theTouchedSlider]->setValue(val);
        }
    }
    //==============================================================================
    int findTouchedSlider( const juce::MouseEvent& e )
    {
        float nSliders = m_sliders.size();
        auto x = e.position.getX();
        auto y = e.position.getY();
        int theTouchedSlider;
        if (!m_isHorizontalFlag)
        {
            if (x < 0) { theTouchedSlider = 0; }
            else if ( x >= getWidth() ) { theTouchedSlider = nSliders - 1; }
            else
            {
                auto sWidth = getWidth()/nSliders;
                for (int s = 0; s < nSliders; s++)
                {
                    if( x >= s*sWidth && x < (s+1)*sWidth) { theTouchedSlider = s; }
                }
            }
        }
        else
        {
            if (y < 0){ theTouchedSlider = 0; }
            else if (y >= getHeight()){ theTouchedSlider = nSliders - 1; }
            else
            {
                auto sHeight = getHeight()/nSliders;
                for (int s = 0; s < nSliders; s++)
                {
                    if( y >= s*sHeight && y < (s+1)*sHeight){ theTouchedSlider = s; }
                }
            }
        }
        return theTouchedSlider;
    }
    //==============================================================================
    float calculatedSliderValue( const juce::MouseEvent& e )
    {
        auto x = e.position.getX();
        auto y = e.position.getY();
        if (!m_isHorizontalFlag) { return ((float)getHeight()-y)/(float)getHeight(); }
        else { return ( x / (float)getWidth() ) ; }
    }
    //==============================================================================
    void mouseActionAndShiftLogic( float val )
    {
        for (int s = 0; s < m_sliders.size(); s++) { m_sliders[s]->setValue( val ); }
    }
    //==============================================================================
    void mouseActionAndAltLogic( float val, int theTouchedSlider )
    {
        
        for (int s = 0; s < m_sliders.size(); s++)
        {
            auto distance = abs(theTouchedSlider - s);
            if (distance == 0){ m_sliders[s]->setValue( val ); }
            else
            {
                auto range = val - m_sliders[s]->getMinimum();
                auto min = m_sliders[s]->getMinimum();
                /* auto newVal = range / pow(2, distance); */
                distance += 1;
                auto newVal = range * ( (m_sliders.size() - distance) / (float)m_sliders.size() );
                newVal += min;
                m_sliders[s]->setValue( newVal );
            }
        }
    }
    //==============================================================================
    void mouseActionAndAltAndShiftLogic( float val, int theTouchedSlider )
    {
        
        for (int s = 0; s < m_sliders.size(); s++)
        {
            auto distance = abs(theTouchedSlider - s);
            if (distance == 0){ m_sliders[s]->setValue( val ); }
            else
            {
                auto max = m_sliders[s]->getMaximum();
                auto range = max - val;
                
                //                auto newVal = range / pow(2, distance);
                distance += 1;
                auto newVal = range * ( (m_sliders.size() - distance) / (float)m_sliders.size() );
                newVal  = max - newVal;
                m_sliders[s]->setValue( newVal );
            }
        }
    }
    //==============================================================================
    void mouseDown (const juce::MouseEvent& e) override
    {
        calulateMousePosToSliderVal(e);
    }
    //==============================================================================
    void mouseDrag(const juce::MouseEvent& e) override
    {
        calulateMousePosToSliderVal(e);
    }
    //==============================================================================
    void createSliderArray(int nSliders)
    {
        deleteAllChildren();
        m_sliders.clear();
        for (int s = 0; s < nSliders; s ++)
        {
            juce::Slider *slider = new juce::Slider;
            slider->setTextBoxStyle(juce::Slider::NoTextBox, 0,0,0);
            slider->setRange(0.0, 1.0, 0.0);
            slider->setInterceptsMouseClicks(false, false);
            addAndMakeVisible(slider);
            m_sliders.add(slider);
        }
    }
    //==============================================================================
private:
    juce::Array<juce::Slider*> m_sliders;
    bool m_isHorizontalFlag = false;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (sjf_multislider)
};
#endif /* sjf_multislider_h */
