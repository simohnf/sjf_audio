//
//  sjf_waveform.h
//
//  Created by Simon Fay on 20/08/2022.
//

#ifndef sjf_waveform_h
#define sjf_waveform_h


#include <vector>
#include <JuceHeader.h>


// need to check normalisation of buffer view because it seems to be a bit weird

class sjf_waveform : public juce::Component, public juce::SettableTooltipClient
{
    //==============================================================================
    //==============================================================================
    //==============================================================================
public:
    sjf_waveform()
    {
        setInterceptsMouseClicks(true, false);
        
        m_env.resize( 3 );
        m_env[ 0 ] = juce::Point< float >(0, 0);
        m_env[ 1 ] = juce::Point< float >(0.5, 0);
        m_env[ 2 ] = juce::Point< float >(1, 0);
        setSize (600, 400);
    }
    //==============================================================================
    ~sjf_waveform(){}
    //==============================================================================
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
        
        auto nPoints = m_env.size();
        if (nPoints <= 0){ return; }
        std::vector< std::array < float, 2 > > env;
        env.resize( nPoints + 2 );
        env.resize( nPoints + 2 );
        env[ 0 ] = {0, (float)getHeight()};
        env[ nPoints + 1 ] = {(float)getWidth(), (float)getHeight()};
        g.setColour( findColour( waveColourID ) );
        for ( int i = 0; i < nPoints; i++ )
        {
            auto x = m_env[ i ].getX()*getWidth();
            auto y = m_env[ i ].getY()*getHeight();
            env[ i + 1 ] = { x, y };
            juce::Rectangle<float> rect = {  x - m_pointRadius, y - m_pointRadius, (float)m_pointRadius*2, (float)m_pointRadius*2 };
            g.fillEllipse( rect );
            
        }
        for ( int i = 0; i < env.size()-1; i++ )
        {
            g.drawLine( env[ i ][ 0 ], env[ i ][ 1 ], env[ i+1 ][ 0 ], env[ i+1 ][ 1 ] );
        }
        
    }
//    //==============================================================================
//    void resized()
//    {
//        m_pointRadius = std::fmin( 5, std::fmin( getWidth(), getHeight() ) * 0.1f );
//    }
    //==============================================================================
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
    
    //==============================================================================
    std::vector< std::array< float, 2 > > getEnvelope()
    {
        std::vector< std::array< float, 2 > > env;
        auto nPoints = m_env.size();
        env.resize( nPoints + 2 );
        env[ 0 ] = { 0, 0 };
        env[ nPoints+1 ] = { 1, 0 };
        for ( int i = 0; i < nPoints; i++ )
        {
            env[ i+1 ][ 0 ] = m_env[ i ].getX();
            env[ i+1 ][ 1 ] = 1.0f - m_env[ i ].getY();
        }
        for ( int i = 0; i < env.size(); i++ )
        {
            DBG( "Env in Waveform " << env[ i ][ 0 ] << " " << env[ i ][ 1 ] );
        }
        return env;
    }
    //==============================================================================
    void reverseEnvelope()
    {
        std::vector< juce::Point< float > > env = m_env;
        
        auto size = m_env.size();
        for ( int i = 0; i < size; i++ )
        {
            m_env[ i ] = env[ size - 1 - i ];
            m_env[ i ].setX( 1.0f - m_env[ i ].getX() );
        }
        repaint();
        if ( onMouseEvent != nullptr ){ onMouseEvent(); }
    }
    //==============================================================================
    std::function<void()> onMouseEvent;
    //==============================================================================
    void shouldOutputOnMouseUp( bool shouldOutputOnMouseUp )
    {
        m_outputEnvOnMouseUp = shouldOutputOnMouseUp;
    }
    //==============================================================================
private:
    //==============================================================================
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
    //==============================================================================
    void mouseDown (const juce::MouseEvent& e) override
    {
        m_breakPointErased = false;
        m_touchBreakPoint = calculateTouchedBreakPoint(e);

        
        if ( m_touchBreakPoint < 0 )
        {
            if ( e.mods.isShiftDown() )
            {
                m_touchBreakPoint = insertNewBreakPoint( e );
                repaint();
                if ( onMouseEvent != nullptr && !m_outputEnvOnMouseUp ){ onMouseEvent(); }
            }
            return;
        }
        
        else if ( e.mods.isAltDown() && m_env.size() >= 0 )
        {
            m_env.erase( m_env.begin() + m_touchBreakPoint );
            m_breakPointErased = true;
            repaint();
        }
        if ( onMouseEvent != nullptr && !m_outputEnvOnMouseUp ){ onMouseEvent(); }
    }
    //==============================================================================
    void mouseDrag(const juce::MouseEvent& e) override
    {
        if ( m_touchBreakPoint < 0 || m_breakPointErased ){ return; }
        auto x = e.position.getX()/getWidth();
        x = std::fmax( std::fmin( x, 1 ), 0 );
        if( m_touchBreakPoint > 0 )
        {
            auto x1 = m_env[ m_touchBreakPoint - 1 ].getX();
            x = x > x1 ? x : x1;
        }
        if ( m_touchBreakPoint < m_env.size() - 1 )
        {
            auto x1 = m_env[ m_touchBreakPoint + 1 ].getX();
            x = x < x1 ? x : x1;
        }
        auto y = e.position.getY()/getHeight();
        y = std::fmax( std::fmin( y, 1 ), 0 );
        m_env[ m_touchBreakPoint ] = { x, y };
        repaint();
        
        if ( onMouseEvent != nullptr && !m_outputEnvOnMouseUp ){ onMouseEvent(); }
    }
    //==============================================================================
    void mouseUp(const juce::MouseEvent& e) override
    {
        if ( onMouseEvent != nullptr && m_outputEnvOnMouseUp ){ onMouseEvent(); }
    }
    //==============================================================================
    int insertNewBreakPoint( const juce::MouseEvent& e )
    {
        auto x = e.position.getX()/getWidth();
        x = std::fmax( std::fmin( x, 1 ), 0 );
        auto y = e.position.getY()/getHeight();
        y = std::fmax( std::fmin( y, 1 ), 0 );
        int indx = -1;
        int oldSize = m_env.size();
        if ( x <= m_env[ 0 ].getX() )
        {
            indx = 0;
        }
        else if ( x >= m_env[ oldSize - 1 ].getX() )
        {
            indx = oldSize + 1;
        }
        else
        {
            for ( int i = 1; i < oldSize; i++ )
            {
                auto xBefore = m_env[ i-1 ].getX();
                auto xAfter = m_env[ i ].getX();
                if ( x >= xBefore && x <= xAfter )
                {
                    indx = i;
                }
            }
        }
        if ( indx == 0 )
        {
            std::vector< juce::Point< float > >::iterator it;
            
            it = m_env.begin();
            it = m_env.insert ( it , { x, y } );
        }
        else if ( indx == oldSize + 1 )
        {
            m_env.push_back ( { x, y } );
        }
        else
        {
            std::vector< juce::Point< float > >::iterator it;
            
            it = m_env.begin();
            it = m_env.insert ( it + indx , { x, y } );
        }
        indx = indx >= 0 && indx < m_env.size() ? indx : -1;
        return indx;
    }
    //==============================================================================
    int calculateTouchedBreakPoint( const juce::MouseEvent& e )
    {
        for ( int i = 0; i < m_env.size(); i++ )
        {
            auto x = m_env[ i ].getX()*getWidth();
            auto y = m_env[ i ].getY()*getHeight();
            auto xDist = x - e.position.getX();
            auto yDist = y - e.position.getY();
            auto dist = std::sqrt( std::pow( xDist, 2 ) + std::pow( yDist, 2 ) );
            if ( dist < m_pointRadius* 1.5 )
            {
                return i;
            }
        }
        
        return -1;
    }
    //==============================================================================
    juce::AudioBuffer< float >* m_ptrToBuffer = nullptr;
    std::vector< juce::Point< float > > m_env;  // m_env[ 0 ] --> attack; m_env[ 1 ] --> release;
    
    bool m_normaliseFlag = false, m_breakPointErased = false, m_outputEnvOnMouseUp = false;
    
    int m_touchBreakPoint = -1;
    
    int m_pointRadius = 5;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (sjf_waveform)
};
#endif /* sjf_waveform_h */



