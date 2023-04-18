//
//  sjf_waveform.h
//
//  Created by Simon Fay on 20/08/2022.
//

#ifndef sjf_waveform_h
#define sjf_waveform_h


#include <vector>
#include <JuceHeader.h>

class sjf_waveform : public juce::Component, public juce::SettableTooltipClient
{
    //==============================================================================
    //==============================================================================
    //==============================================================================
public:
    sjf_waveform()
    {
        setInterceptsMouseClicks(true, false);
        setSize (600, 400);
    }
    ~sjf_waveform(){}
    
    void drawWaveform( juce::AudioBuffer< float >& bufferToDraw )
    {
        m_ptrToBuffer = &bufferToDraw;
        repaint();
    }
    
    //==============================================================================
    void paint ( juce::Graphics& g ) override
    {
        g.setColour( findColour( backgroundColourId ) );
        g.fillAll();
        
        g.setColour( findColour( outlineColourId ) );
        juce::Rectangle<float> rect = { 0, 0, (float)getWidth(), (float)getHeight() };
        g.drawRect( rect );
        
        g.setColour( findColour( waveColourID ) );
        drawBuffer( g );
    }
    
    void setNormaliseFlag( bool displayIsNormalised )
    {
        m_normaliseFlag = displayIsNormalised;
    }
    //==============================================================================
    enum ColourIds
    {
        backgroundColourId          = 0x1001200,
        outlineColourId             = 0x1000c00,
        waveColourID              = 0x1001310,
    };
    
private:
    void drawBuffer( juce::Graphics& g )
    {
        if ( m_ptrToBuffer == nullptr ){ return; }
        auto nChans = m_ptrToBuffer->getNumChannels();
        if ( nChans == 0 ){ return; }
        auto nSamps = m_ptrToBuffer->getNumSamples();
        auto stride = nSamps / getWidth();
        auto chanHeight = getHeight() / nChans;
        auto hChanHeight = chanHeight * 0.5;
        float normalise = 1.0f;
        if ( m_normaliseFlag )
        {
            // there must be a neater way to do this
            normalise = m_ptrToBuffer->getMagnitude( 0, 0, nSamps );
            for ( int c = 1; c < nChans; c++ )
            {
                auto mag = m_ptrToBuffer->getMagnitude( c, 0, nSamps );
                normalise = ( normalise >  mag ) ? mag : normalise;
            }
            normalise = 1.0f / normalise;
        }
        
        for ( int c = 0; c < nChans; c++ )
        {
            auto centre = c * chanHeight + hChanHeight;
            for ( int s = 0; s < getWidth(); s++ )
            {
                auto lineSize = m_ptrToBuffer->getSample( c, s*stride ) * hChanHeight * normalise;
                if ( lineSize >= 0 )
                {
                    g.drawVerticalLine( s, centre-lineSize, centre );
                }
                else
                {
                    g.drawVerticalLine( s, centre, centre + abs(lineSize) );
                }
            }
        }
    }
    
    juce::AudioBuffer< float >* m_ptrToBuffer = nullptr;
    
    bool m_normaliseFlag = false;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (sjf_waveform)
};
#endif /* sjf_waveform_h */



