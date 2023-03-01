//
//  sjf_lfoInterface.h
//
//  Created by Simon Fay on 01/03/2023.
//

#ifndef sjf_lfoInterface_h
#define sjf_lfoInterface_h

#include "../../sjf_audio/sjf_numBox.h"
#include "../../sjf_audio/sjf_LookAndFeel.h"
#include "../../sjf_audio/sjf_numBox.h"

class sjf_lfoInterface: public juce::Component
{
    sjf_lookAndFeel otherLookAndFeel;
    
public:
    juce::Slider m_lfoRateSlider, m_lfoDepthSlider, m_lfoOffsetSlider, m_lfoTriangleDutySlider;
    juce::ComboBox m_lfoType, m_lfoBeatName,  m_lfoBeatType;
    juce::ToggleButton m_lfoSyncToggle;
    sjf_numBox m_lfonBeats;
    
    
    
    
public:
    sjf_lfoInterface()
    {
        juce::StringArray beatNames { "/1", "/2", "/4", "/8", "/16" };
        juce::StringArray beatTypes { "tup", "dot", "trip", "5s", "7s" };
        //////////////////////////////////////////////////////////////////
        setLookAndFeel( &otherLookAndFeel );
        otherLookAndFeel.drawComboBoxTick = false;
        //////////////////////////////////////////////////////////////////
        addAndMakeVisible( m_lfoType );
        m_lfoType.addItem( "Sine", 1 );
        m_lfoType.addItem( "Tri", 2 );
        m_lfoType.addItem( "Noise1-SAH", 3 );
        m_lfoType.addItem( "Noise2", 4 );
        m_lfoType.onChange = [ this ] { checkIflfoIsTriangle(); };
        m_lfoType.setTooltip("This allows you to choose between different types of modulation \nsine wave, triangle wave, sample and hold random, or noise");
        m_lfoType.sendLookAndFeelChange();
        m_lfoType.setSelectedId( 1 );
        //////////////////////////////////////////////////////////////////
        addAndMakeVisible( &m_lfoSyncToggle );
        m_lfoSyncToggle.setButtonText( "Sync" );
        m_lfoSyncToggle.onClick = [ this ] { checklfoSyncState(); };
        m_lfoSyncToggle.setTooltip("This allows you to sync the modulator to the underlying tempo");
        m_lfoSyncToggle.sendLookAndFeelChange();
        //////////////////////////////////////////////////////////////////
        addAndMakeVisible( &m_lfoRateSlider );
        m_lfoRateSlider.setTextValueSuffix( "Hz" );
        m_lfoRateSlider.setSliderStyle( juce::Slider::LinearBar );
        m_lfoRateSlider.setTooltip("This sets the rate of modulation in Hz");
        m_lfoRateSlider.sendLookAndFeelChange();
        m_lfoRateSlider.setRange( 0.0f, 20.0f );
        //////////////////////////////////////////////////////////////////
        addAndMakeVisible( &m_lfonBeats );
        m_lfonBeats.setTooltip("This sets the number of beats for the tempo synced modulation");
        m_lfonBeats.sendLookAndFeelChange();
        //////////////////////////////////////////////////////////////////
        m_lfoBeatName.addItemList( beatNames, 1 );
        addAndMakeVisible( &m_lfoBeatName );
        m_lfoBeatName.setTooltip("This allows you to choose between different rhythmic division \n/1 is a bar, /2 is half a bar, /4 is a quarter note, etc");
        m_lfoBeatName.sendLookAndFeelChange();
        m_lfoBeatName.setSelectedId( 1 );
        //////////////////////////////////////////////////////////////////
        m_lfoBeatType.addItemList( beatTypes, 1 );
//        m_lfoBeatTypeAttachment.reset( new juce::AudioProcessorValueTreeState::ComboBoxAttachment( valueTreeState, audioProcessor.lfoNames[ 0 ]+audioProcessor.lfoParamNames[ 10 ], m_lfoBeatType ) );
        addAndMakeVisible( &m_lfoBeatType );
        m_lfoBeatType.setTooltip("This allows you to choose between different rhythmic division types \ntuplets (normal), dotted notes, triplets, quintuplets, and septuplets");
        m_lfoBeatType.sendLookAndFeelChange();
        m_lfoBeatType.setSelectedId( 1 );
        //////////////////////////////////////////////////////////////////
        addAndMakeVisible( m_lfoDepthSlider );
        m_lfoDepthSlider.setSliderStyle( juce::Slider::LinearBar );
        m_lfoDepthSlider.setTooltip("This sets the depth of the modulation");
        m_lfoDepthSlider.sendLookAndFeelChange();
        m_lfoDepthSlider.setRange( 0.0f, 1.0f );
        //////////////////////////////////////////////////////////////////
        addAndMakeVisible( m_lfoOffsetSlider );
        m_lfoOffsetSlider.setSliderStyle( juce::Slider::LinearBar );
        m_lfoOffsetSlider.setTooltip("This sets an offset for the modulation \nIf set to 0 the parameter will modulate above and below the set value, if set to -1 all of the modulation will be below the set value, if set to +1 all of the modulation will be above the set value, etc.");
        m_lfoOffsetSlider.sendLookAndFeelChange();
        m_lfoOffsetSlider.setRange( -1.0f, 1.0f );
        //////////////////////////////////////////////////////////////////
        addAndMakeVisible( &m_lfoTriangleDutySlider );
        m_lfoTriangleDutySlider.setSliderStyle( juce::Slider::LinearBar );
        m_lfoTriangleDutySlider.setTooltip("This sets the duty cycle for the triangle wave \nAt 0.5 it will be a normal triangle wave, at 0 it will be a descending sawthooth wave starting at 1, and at 1 it will be an ascending sawtooth wave");
        m_lfoTriangleDutySlider.sendLookAndFeelChange();
        m_lfoTriangleDutySlider.setRange( 0.0f, 1.0f );
        
        checkIflfoIsTriangle();
        checklfoSyncState();
    }
    ~sjf_lfoInterface()
    {
        setLookAndFeel( nullptr );
    }
    
    
    void resized() override
    {
        auto indent = getLocalBounds().getWidth() / 4;
        auto width = getLocalBounds().getWidth();
        auto width2 = indent*3;
        auto compHeight = getLocalBounds().getHeight() / 6;
        
        m_lfoType.setBounds( 0, 0, width, compHeight );
        m_lfoSyncToggle.setBounds( 0, m_lfoType.getBottom(), width, compHeight );
        m_lfoRateSlider.setBounds( indent, m_lfoSyncToggle.getBottom(), width2, compHeight );
        m_lfonBeats.setBounds( indent, m_lfoSyncToggle.getBottom(), width/4, compHeight );
        m_lfoBeatName.setBounds( m_lfonBeats.getRight(), m_lfonBeats.getY(), width/4, compHeight );
        m_lfoBeatType.setBounds( m_lfoBeatName.getRight(), m_lfoBeatName.getY(), width/4, compHeight );
        
        m_lfoDepthSlider.setBounds( m_lfoRateSlider.getX(), m_lfoRateSlider.getBottom(), width2, compHeight );
        m_lfoOffsetSlider.setBounds( m_lfoDepthSlider.getX(), m_lfoDepthSlider.getBottom(), width2, compHeight );
        m_lfoTriangleDutySlider.setBounds( m_lfoOffsetSlider.getX(), m_lfoOffsetSlider.getBottom(), width2, compHeight );
    }
    
    
    void paint (juce::Graphics& g) override
    {
        auto width = getLocalBounds().getWidth()/4 - 2;
        auto compHeight = getLocalBounds().getHeight() / 6;
        
        g.setColour( otherLookAndFeel.fontColour );
        g.drawFittedText( "rate", 1, m_lfoRateSlider.getY(), width, compHeight, juce::Justification::right, 1 );
        g.drawFittedText( "depth", 1, m_lfoDepthSlider.getY(), width, compHeight, juce::Justification::right, 1 );
        g.drawFittedText( "offset", 1, m_lfoOffsetSlider.getY(), width, compHeight, juce::Justification::right, 1 );
        g.drawFittedText( "duty", 1, m_lfoTriangleDutySlider.getY(), width, compHeight, juce::Justification::right, 1 );
        
        g.setColour( otherLookAndFeel.outlineColour );
        g.drawLine( 0, m_lfoSyncToggle.getBottom(), 0, m_lfoTriangleDutySlider.getBottom() );
        
        g.drawLine( 0, m_lfoRateSlider.getY(), m_lfoRateSlider.getX(), m_lfoRateSlider.getY() );
        g.drawLine( 0, m_lfoRateSlider.getBottom(), m_lfoRateSlider.getX(), m_lfoRateSlider.getBottom() );
        g.drawLine( 0, m_lfoDepthSlider.getBottom(), m_lfoDepthSlider.getX(), m_lfoDepthSlider.getBottom() );
        g.drawLine( 0, m_lfoOffsetSlider.getBottom(), m_lfoOffsetSlider.getX(), m_lfoOffsetSlider.getBottom() );
        g.drawLine( 0, getLocalBounds().getBottom(), m_lfoTriangleDutySlider.getX(), getLocalBounds().getBottom() );
    }
    
private:
    void checkIflfoIsTriangle()
    {
        if ( m_lfoType.getSelectedId() == 2 )
        {
//            m_lfoTriangleDutySlider.setVisible( true );
            m_lfoTriangleDutySlider.setColour(juce::Slider::trackColourId, otherLookAndFeel.sliderFillColour.withAlpha(0.5f) );
        }
        else
        {
//            m_lfoTriangleDutySlider.setVisible( false );
            m_lfoTriangleDutySlider.setColour(juce::Slider::trackColourId, otherLookAndFeel.fontColour.withAlpha(0.8f) );
        }
//        repaint();
    }
    
    
    void checklfoSyncState()
    {
        if ( m_lfoSyncToggle.getToggleState() )
        {
            m_lfonBeats.setVisible( true );
            m_lfoBeatName.setVisible( true );
            m_lfoBeatType.setVisible( true );
            m_lfoRateSlider.setVisible( false );
        }
        else
        {
            m_lfonBeats.setVisible( false );
            m_lfoBeatName.setVisible( false );
            m_lfoBeatType.setVisible( false );
            m_lfoRateSlider.setVisible( true );
        }
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_lfoInterface )
};
#endif /* sjf_lfoInterface_h */


