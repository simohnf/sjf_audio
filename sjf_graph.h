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
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
class sjf_graph : public juce::Component
{
public:
    sjf_graph() : m_values(100, 1.0f)
    {
        setSize (600, 400);
    }
    ~sjf_graph(){ setLookAndFeel(nullptr); };

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
        auto sliderWidth =  (float)getWidth()  / (float)nVals;
        auto height = getHeight();
        for(int i = 0; i < nVals; i++)
        {
            auto val = m_values[i] * height;
            g.drawLine( lineStart, val, lineStart + sliderWidth, val );
//            g.drawHorizontalLine(val, lineStart, lineStart + sliderWidth);
            lineStart += sliderWidth;
        }
        
        g.setColour(findColour(outlineColourId));
        g.drawVerticalLine( m_positionPhase * getWidth(), 1, getHeight() - 1 );
        g.drawFittedText (m_name, getLocalBounds(), juce::Justification::centred, 1);
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
        auto oldSize = (int)m_values.size();
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
        if ( indexToOutput > (int)m_values.size()-1 ) { indexToOutput = (int)m_values.size()-1; }
        
        return 1.0f - m_values[indexToOutput];
    }
    //==============================================================================
    void setPoint(int indexOfPointToSet, float newValue)
    {
        if ( indexOfPointToSet < 0 ) { indexOfPointToSet = 0;}
        if ( indexOfPointToSet > (int)m_values.size() ) { indexOfPointToSet = (int)m_values.size(); }
        if ( newValue < 0 ) { newValue = 0; }
        if ( newValue > 1 ) { newValue = 1; }
        m_values[indexOfPointToSet] = 1.0f - newValue;
        repaint();
    }
    //==============================================================================
    int getNumPoints()
    {
        return (int)m_values.size();
    }
    //==============================================================================
    enum ColourIds
    {
        backgroundColourId          = 0x1001200,
        outlineColourId             = 0x1000c00,
        pointColourID              = 0x1001310,
    };
    //==============================================================================
    std::vector<float> getGraphAsVector()
    {
        return m_values;
    }
    //==============================================================================
    
    void setGraphText( std::string newText)
    {
        m_name = newText;
        repaint();
    }
    
    float m_positionPhase = 0.0f;
protected:
    std::vector<float> m_values;
    std::string m_name = "sjf_grapher";
};
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================



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
//        graphChoiceBox.setSelectedId(1);
//        graphChoiceBox.setTooltip("This allows you to choose from different preset graphs");
        
        
        addAndMakeVisible(&rangeBox);
        rangeBox.setRange(0.0f, 1.0f);
        rangeBox.onValueChange = [this]{m_range = rangeBox.getValue(); drawGraph(m_lastGraphChoice);};
//        rangeBox.setValue( m_range );
//        rangeBox.setTooltip("This sets the maximum range of the preset graphs --> 1 is the full range, 0 is no range ==> a straight line");
        
        addAndMakeVisible(&offsetBox);
        offsetBox.setRange(-1.0f, 1.0f);
        offsetBox.onValueChange = [this]{m_offset = offsetBox.getValue(); drawGraph(m_lastGraphChoice);};
//        offsetBox.setValue( m_offset );
//        offsetBox.setTooltip("This sets the offset of the preset graphs --> 0 is no offset, negative numbers moves the graph down, positive numbers will shift it upwards");
        
        addAndMakeVisible(&jitterBox);
        jitterBox.setRange(0.0f, 1.0f);
        jitterBox.onValueChange = [this]{m_jitter = jitterBox.getValue(); drawGraph(m_lastGraphChoice);};
//        jitterBox.setValue( m_jitter );
//        jitterBox.setTooltip("This adds jitter (randomness) to the graph");
        
        addAndMakeVisible(&randomBox);
        randomBox.setButtonText("random");
        randomBox.onClick = [this]{ randomGraph(); };
        
        setSize (600, 400);
        
    };
    ~sjf_grapher(){ };
    
    std::vector<float>  randomGraph()
    {
        
        m_range = rand01();
        rangeBox.setValue( m_range, juce::dontSendNotification );
        m_offset = (pow(rand01(), 0.5) ) - 0.5f;
        offsetBox.setValue( m_offset, juce::dontSendNotification );
        m_jitter = pow(rand01(), 4);
        jitterBox.setValue( m_jitter, juce::dontSendNotification );
        m_lastGraphChoice = 1+ rand01() * (float)graphChoiceBox.getNumItems();
        graphChoiceBox.setSelectedId( m_lastGraphChoice , juce::dontSendNotification);
        drawGraph(m_lastGraphChoice);
        return getGraphAsVector();
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
            }
            val *= m_range;
            val += m_offset;
            auto jit = rand01() * m_jitter;
            jit -= m_jitter * 0.5;
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
        m_indent = getWidth() * 0.15f;
        m_boxHeight = getHeight() * 0.2f;
        m_graph.setBounds(m_indent, 0, getWidth()-m_indent, getHeight());
        m_graph.setNumPoints(getWidth()-m_indent);
        
        graphChoiceBox.setBounds(0, 0, m_indent, m_boxHeight);
        rangeBox.setBounds(0, m_boxHeight, m_indent, m_boxHeight);
        offsetBox.setBounds(0, m_boxHeight * 2, m_indent, m_boxHeight);
        jitterBox.setBounds(0, m_boxHeight * 3, m_indent, m_boxHeight);
        randomBox.setBounds(0, m_boxHeight * 4, m_indent, m_boxHeight);
    }
    //==============================================================================
    std::vector<float> getGraphAsVector()
    {
        std::vector<float> temp = m_graph.getGraphAsVector();
        for (int i = 0; i < temp.size(); i++)
        {
            temp[i] = 1.0f - temp[i];
        }
        return temp;
    }
    //==============================================================================
    void setGraph( std::vector<float> newGraph )
    {
        auto maxSize = m_graph.getNumPoints();
        auto nVals = newGraph.size();
        for (int i = 0; i < nVals; i++)
        {
            if (i < maxSize)
            {
                m_graph.setPoint(i, newGraph[i]);
            }
        }
    }
    //==============================================================================
    void setGraphText( std::string newText)
    {
        m_graph.setGraphText( newText );
    }
    //==============================================================================
    juce::Rectangle<int> getGraphBounds()
    {
        return m_graph.getBounds();
    }
    //==============================================================================
    void setGraphPosition( float positionPhase )
    {
        m_graph.m_positionPhase = positionPhase;
        m_graph.repaint();
    }
private:
    int m_lastGraphChoice;
    sjf_graph m_graph;
    float m_indent, m_boxHeight;
//    juce::TooltipWindow tooltipWindow {this, 700};
    
public:
    juce::ComboBox graphChoiceBox;
    float m_offset = 0, m_range = 1, m_jitter = 0;
    sjf_numBox offsetBox, rangeBox, jitterBox;
    juce::TextButton randomBox;
    
};

#endif /* sjf_graph_h */
