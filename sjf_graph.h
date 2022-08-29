//
//  sjf_graph.h
//
//  Created by Simon Fay on 29/08/2022.
//

#ifndef sjf_graph_h
#define sjf_graph_h

#include <JuceHeader.h>
#include "sjf_numBox.h"
#include "sjf_audioUtilities.h"
#define PI 3.14159265
//==============================================================================
inline
float cos01(float phase)
{
    auto val = phase;
    val -= 0.75;
    val *= 2.0f * PI;
    val = sin( val );
    val += 1;
    val*= 0.5;
    return val;
};
//==============================================================================
inline
float halfCos01(float phase)
{
    auto val = phase *0.5f;
    val -= 0.75;
    val *= 2.0f * PI;
    val = sin( val );
    val += 1;
    val*= 0.5;
    return val;
};
//==============================================================================
inline
float invHalfCos01(float phase)
{
    auto val = phase *0.5f;
    val -= 0.75;
    val *= 2.0f * PI;
    val = sin( val );
    val += 1;
    val*= 0.5;
    return 1 - val;
}
//==============================================================================
inline
float invSin01(float phase)
{
    auto val = phase;
    val -= 0.5;
    val *= 2.0f * PI;
    val = sin( val );
    val += 1;
    val*= 0.5;
    return val;
}
//==============================================================================
inline
float sin01(float phase)
{
    auto val = sin( 2.0f * PI * phase );
    val += 1;
    val*= 0.5;
    return val;
}
//==============================================================================
inline
float hann01(float phase)
{
    auto val = phase;
    val -= 0.25;
    val *= 2.0f * PI;
    val = sin( val );
    val += 1;
    val*= 0.5;
    return val;
}
//==============================================================================
inline
float halfsin01(float phase)
{
     return sin( PI * phase);
}
//==============================================================================
inline
float lineUp01(float phase)
{
    return phase;
}
//==============================================================================
inline
float lineDown01(float phase)
{
    return 1 - phase;
}
//==============================================================================
inline
float expodec01(float phase)
{
    return pow( 1 - phase, 5.0f );
}
//==============================================================================
inline
float rexpodec01(float phase)
{
    return pow( phase, 5.0f );
}
//==============================================================================
inline
float upDown01(float phase)
{
    auto val = phase * 2.0f;
    if (val > 1.0f) { val = 1.0f - (val-1.0f); }
    return val;
}
//==============================================================================
inline
float downUp01(float phase)
{
    auto val = phase * 2.0f;
    if (val > 1.0f) { val = 1.0f - (val-1.0f); }
    return 1.0f - val;
}
//==============================================================================
class sjf_graph : public juce::Component
{
public:
    sjf_graph() : m_values(100, 1.0f)
    {
        setSize (600, 400);
    }
    ~sjf_graph(){ };

    void paint (juce::Graphics& g) override
    {
        g.setColour( findColour(backgroundColourId) );
        g.fillAll();
//        g.setFont (juce::Font (16.0f));
        g.setColour ( findColour(outlineColourId) );
        g.drawRect(0, 0, getWidth(), getHeight());
        
        
        g.setColour ( findColour(pointColourID) );
        auto lineStart = 0;
        auto nVals = m_values.size();
        auto sliderWidth = ((float)getWidth()) / (float)nVals;
        auto height = getHeight();
        for(int i = 0; i < nVals; i++)
        {
            auto val = m_values[i] * height;
            g.drawLine( round(lineStart), val, round(lineStart + sliderWidth), val );
            lineStart += sliderWidth;
        }
    }
    
    //==============================================================================
    void resized() override
    {

    }
    //==============================================================================
    void mouseDown (const juce::MouseEvent& e) override
    {
        calulateMousePosToVal(e);
    }
    //==============================================================================
    void mouseDrag(const juce::MouseEvent& e) override
    {
        calulateMousePosToVal(e);
    }
    //==============================================================================
    void calulateMousePosToVal(const juce::MouseEvent& e)
    {
        auto pos = e.position;
        float x = pos.getX();
        float y = pos.getY();
        float nVals = m_values.size();
        float width = getWidth();
        float height = getHeight();
        
        if ( x > width ) { x = width; }
        if ( x < 0 ) { x = 0; }
        if ( y > height ) { y = height; }
        if ( y < 0 ) { y = 0; }
        int valToChange = round( nVals * x / width );
        auto newVal = y / height;
        
        m_values[valToChange] = newVal;
        repaint();
    }
    //==============================================================================
    void setNumPoints(int nPoints)
    {
        if ( nPoints < 1 ) { nPoints = 1; }
        auto oldSize = m_values.size();
        m_values.resize(nPoints);
        if ( nPoints > oldSize )
        {

            for (int i = oldSize-1; i < nPoints; i++)
            {
                m_values[i] = 1.0f;
            }
        }
    }
    //==============================================================================
    float outputValue(int indexToOutput)
    {
        if ( indexToOutput < 0 ){ indexToOutput = 0; }
        if ( indexToOutput > m_values.size()-1 ) { indexToOutput = m_values.size()-1; }
        
        return 1.0f - m_values[indexToOutput];
    }
    //==============================================================================
    void setPoint(int indexOfPointToSet, float newValue)
    {
        if ( indexOfPointToSet < 0 ) { indexOfPointToSet = 0;}
        if ( indexOfPointToSet > m_values.size() ) { indexOfPointToSet = m_values.size(); }
        if ( newValue < 0 ) { newValue = 0; }
        if ( newValue > 1 ) { newValue = 1; }
        m_values[indexOfPointToSet] = 1.0f - newValue;
        repaint();
    }
    //==============================================================================
    int getNumPoints()
    {
        return m_values.size();
    }
    //==============================================================================
    enum ColourIds
    {
        backgroundColourId          = 0x1001200,
        outlineColourId             = 0x1000c00,
        pointColourID              = 0x1001310,
    };
    //==============================================================================
    
protected:
    std::vector<float> m_values;
};



class sjf_grapher : public juce::Component
{
public:
    sjf_grapher()
    {
        srand((unsigned)time(NULL));
        addAndMakeVisible(&m_graph);
        addAndMakeVisible(&graphChoiceBox);
        graphChoiceBox.addItem("sin", 1);
        graphChoiceBox.addItem("cos", 2);
        graphChoiceBox.addItem("hann", 3);
        graphChoiceBox.addItem("up", 4);
        graphChoiceBox.addItem("down", 5);
        graphChoiceBox.addItem("expod", 6);
        graphChoiceBox.addItem("rexpod", 7);
        graphChoiceBox.addItem("invsin",8);
        graphChoiceBox.addItem("halfcos",9);
        graphChoiceBox.addItem("invhalfcos",10);
        graphChoiceBox.addItem("updown",11);
        graphChoiceBox.addItem("downup",12);
//        graphChoiceBox.addItem("rand", 13);
        graphChoiceBox.onChange = [this]{ drawGraph( graphChoiceBox.getSelectedId() );};
        graphChoiceBox.setSelectedId(1);
        
        
        addAndMakeVisible(&rangeBox);
        rangeBox.setRange(0.0f, 1.0f);
        rangeBox.onValueChange = [this]{m_range = rangeBox.getValue(); drawGraph(m_lastGraphChoice);};
        
        addAndMakeVisible(&offsetBox);
        offsetBox.setRange(-1.0f, 1.0f);
        offsetBox.onValueChange = [this]{m_offset = offsetBox.getValue(); drawGraph(m_lastGraphChoice);};
        
        addAndMakeVisible(&jitterBox);
        jitterBox.setRange(0.0f, 1.0f);
        jitterBox.onValueChange = [this]{m_jitter = jitterBox.getValue(); drawGraph(m_lastGraphChoice);};
        
        addAndMakeVisible(&randomBox);
        randomBox.setButtonText("random");
        randomBox.onClick = [this]{ randomGraph(); };
        
        setSize (600, 400);
    };
    ~sjf_grapher(){ };
    
    void randomGraph()
    {
        rangeBox.setValue( rand01() );
        offsetBox.setValue( (pow(rand01(), 0.5) ) - 0.5f );
        jitterBox.setValue( pow(rand01(), 4) );
        graphChoiceBox.setSelectedId( 1+ rand01() * (float)graphChoiceBox.getNumItems() );
    }
    
    void drawGraph( int graphType )
    {
        auto nVals = m_graph.getNumPoints();
        for (int i = 0 ; i < nVals ; i++ )
        {
            auto val = 0.0f;
            switch (graphType)
            {
                case 1:
                    val = sin01( (float)i / (float)nVals );
                    break;
                case 2:
                    val = cos01( (float)i / (float)nVals );
                    break;
                case 3:
                    val = hann01( (float)i / (float)nVals );
                    break;
                case 4:
                    val = lineUp01( (float)i / (float)nVals );
                    break;
                case 5:
                    val = lineDown01( (float)i / (float)nVals );
                    break;
                case 6:
                    val = expodec01( (float)i / (float)nVals );
                    break;
                case 7:
                    val = rexpodec01( (float)i / (float)nVals );
                    break;
                case 8:
                    val = invSin01( (float)i / (float)nVals );
                    break;
                case 9:
                    val = halfCos01( (float)i / (float)nVals );
                    break;
                case 10:
                    val = invHalfCos01( (float)i / (float)nVals );
                    break;
                case 11:
                    val = upDown01( (float)i / (float)nVals );
                    break;
                case 12:
                    val = downUp01( (float)i / (float)nVals );
                    break;
//                case 13:
//                    val = rand01();
//                    break;
            }
            val *= m_range;
            val += m_offset;
            auto jit = rand01() * m_jitter;
            jit -= m_jitter*0.5;
            val += jit;
            
            if (val < 0){ val = 0; }
            if (val > 1){ val = 1; }
            m_graph.setPoint(i, val);
        }
        m_lastGraphChoice = graphType;
    }
    //==============================================================================
    void paint (juce::Graphics& g) override
    {
        g.setColour( findColour(backgroundColourId) );
        g.fillAll();
        g.setColour ( findColour(outlineColourId) );
        g.drawRect(0, 0, getWidth(), getHeight());
        m_graph.setColour(m_graph.pointColourID, juce::Colours::white);
    }
    //==============================================================================
    enum ColourIds
    {
        backgroundColourId          = 0x1001200,
        outlineColourId             = 0x1000c00,
        pointColourID              = 0x1001310,
    };
    
    //==============================================================================
    void resized() override
    {
        auto indent = getWidth() * 0.2;
        m_graph.setBounds(indent, 0, getWidth()-indent, getHeight());
        m_graph.setNumPoints(getWidth()-indent);
        
        
        graphChoiceBox.setBounds(0, 0, indent, 20);
        rangeBox.setBounds(0, 20, indent, 20);
        offsetBox.setBounds(0, 40, indent, 20);
        jitterBox.setBounds(0, 60, indent, 20);
        randomBox.setBounds(0, 80, indent, 20);
    }
    

private:
    int m_lastGraphChoice;
    sjf_graph m_graph;
public:
    juce::ComboBox graphChoiceBox;
    float m_offset = 0, m_range = 1, m_jitter = 0;
    sjf_numBox offsetBox, rangeBox, jitterBox;
    juce::TextButton randomBox;
};

#endif /* sjf_graph_h */
