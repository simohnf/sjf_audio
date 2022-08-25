//
//  sjf_multislider.h
//  multiSlider
//
//  Created by Simon Fay on 20/08/2022.
//

#ifndef sjf_multislider_h
#define sjf_multislider_h

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
        
        auto nSliders = sliders.size();
        for (int s = 0; s < nSliders; s++)
        {
            sliders[s]->setColour( juce::Slider::textBoxOutlineColourId, juce::Colours::black.withAlpha(0.0f) ) ;
            sliders[s]->setColour( juce::Slider::trackColourId, findColour( sliderColourID ) );
        }
    }
    //==============================================================================
    void resized() override
    {
        auto nSliders = sliders.size();
        for (int s = 0; s < nSliders; s++)
        {
            if (!isHorizontalFlag)
            {
                auto sWidth = (float)this->getWidth() / (float)nSliders;
                sliders[s]->setSliderStyle(juce::Slider::LinearBarVertical);
                sliders[s]->setBounds(sWidth*s, 0.0f, sWidth, getHeight());
            }
            else
            {
                auto sHeight = (float)getHeight()/(float)nSliders;
                sliders[s]->setSliderStyle(juce::Slider::LinearBar);
                sliders[s]->setBounds(0.0f, sHeight*s, getWidth(), sHeight);
            }
        }
    }
    //==============================================================================
    void setNumSliders( int numSliders )
    {
        auto nSliders = sliders.size();
        if (numSliders < 1){ numSliders = 1; }
        if (numSliders == nSliders){ return; }
        
        std::vector<float> temp;
        for(int s = 0; s < nSliders; s++){ temp.push_back(sliders[s]->getValue()); }
        
        createSliderArray(numSliders);
        
        for(int s = 0; s < numSliders; s++)
        {
            if(s < temp.size()) { sliders[s]->setValue(temp[s]); }
        }
        resized();
    }
    //==============================================================================
    int getNumSliders()
    {
        return sliders.size();
    }
    //==============================================================================
    float fetch(int sliderIndex)
    {
        return ( sliders[sliderIndex]->getValue() );
    }
    //==============================================================================
    void setRange(float min, float max)
    {
        for (int s = 0; s < sliders.size(); s ++) { sliders[s]->setRange(min, max); }
    }
    //==============================================================================
    std::vector<float> outputList()
    {
        auto nSliders = sliders.size();
        std::vector<float> temp;
        for(int s = 0; s < nSliders; s++){ temp.push_back(sliders[s]->getValue()); }
        return temp;
    }
    //==============================================================================
    void setHorizontal(bool horizontal)
    {
        if(isHorizontalFlag != horizontal)
        {
            isHorizontalFlag = horizontal;
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
            sliders[theTouchedSlider]->setValue(val);
        }
    }
    //==============================================================================
    int findTouchedSlider( const juce::MouseEvent& e )
    {
        float nSliders = sliders.size();
        auto x = e.position.getX();
        auto y = e.position.getY();
        int theTouchedSlider;
        if (!isHorizontalFlag)
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
        if (!isHorizontalFlag) { return ((float)getHeight()-y)/(float)getHeight(); }
        else { return ( x / (float)getWidth() ) ; }
    }
    //==============================================================================
    void mouseActionAndShiftLogic( float val )
    {
        for (int s = 0; s < sliders.size(); s++) { sliders[s]->setValue( val ); }
    }
    //==============================================================================
    void mouseActionAndAltLogic( float val, int theTouchedSlider )
    {
        
        for (int s = 0; s < sliders.size(); s++)
        {
            auto distance = abs(theTouchedSlider - s);
            if (distance == 0){ sliders[s]->setValue( val ); }
            else
            {
                auto range = val - sliders[s]->getMinimum();
                auto min = sliders[s]->getMinimum();
                /* auto newVal = range / pow(2, distance); */
                distance += 1;
                auto newVal = range * ( (sliders.size() - distance) / (float)sliders.size() );
                newVal += min;
                sliders[s]->setValue( newVal );
            }
        }
    }
    //==============================================================================
    void mouseActionAndAltAndShiftLogic( float val, int theTouchedSlider )
    {
        
        for (int s = 0; s < sliders.size(); s++)
        {
            auto distance = abs(theTouchedSlider - s);
            if (distance == 0){ sliders[s]->setValue( val ); }
            else
            {
                auto max = sliders[s]->getMaximum();
                auto range = max - val;
                
                //                auto newVal = range / pow(2, distance);
                distance += 1;
                auto newVal = range * ( (sliders.size() - distance) / (float)sliders.size() );
                newVal  = max - newVal;
                sliders[s]->setValue( newVal );
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
        sliders.clear();
        for (int s = 0; s < nSliders; s ++)
        {
            juce::Slider *slider = new juce::Slider;
            slider->setTextBoxStyle(juce::Slider::NoTextBox, 0,0,0);
            slider->setRange(0.0, 1.0, 0.0);
            slider->setInterceptsMouseClicks(false, false);
            addAndMakeVisible(slider);
            sliders.add(slider);
        }
    }
    //==============================================================================
private:
    juce::Array<juce::Slider*> sliders;
    bool isHorizontalFlag = false;
};
#endif /* sjf_multislider_h */
