//
//  sjf_moogLadder.h
//
//  Created by Simon Fay on 13/11/2022.
//
//  modifieed implementation of Will Pirkle's Moog Ladder Emulation



#ifndef sjf_moogLadder_h
#define sjf_moogLadder_h

#include "/Users/simonfay/Programming_Stuff/sjf_audio/sjf_lpfFirst.h"
class sjf_moogLadder
{
public:
    sjf_moogLadder()
    {
        calculateAlpha();
        calculateGammas();
    };
    ~sjf_moogLadder(){};
    
    float filterInput( float x )
    {
        m_preY = 0.0f;
        for ( int o = 0; o < m_nOrders; o++ )
        {
            m_preY += m_lpf[ o ].getY() * m_gamma[ o ];
        }
        m_preY *= m_K * -1.0f;
        x += m_preY;
        x = tanh( x ) * m_alpha;
        for ( int o = 0; o < m_nOrders; o++ )
        {
            x = m_lpf[ o ].filterInput( x )
        }
    }
    
    void setSampleRate( float sr )
    {
        m_SR = sr;
    }
    void setCoefficent( float c )
    {
        m_a0 = c;
        for ( int o = 0; o < m_nOrders; o++ )
        {
            m_lpf[ o ].setCutoff( m_a0 );
        }
        calculateAlpha();
        calculateGammas();
    }
    
    void setResonance( float r )
    {
        m_K = fmin( fmax( r, 0.0f ), 4.0f );
        calculateAlpha();
    }
    
    void setCutoff( float f )
    {
        f = fmin( fmax( f, 20.0f ), 5000.0f );
        setCoefficent( sin( f * 2 * 3.141593 / m_SR ) );
    }
private:
    void calculateAlpha()
    {
        m_alpha0 = 1.0f / ( 1.0f + (m_a0*m_a0*m_a0*m_a0 * m_K) );
    }
    void calculateGammas()
    {
        gamma[ 0 ] = m_a0*m_a0*m_a0;
        gamma[ 1 ] = m_a0*m_a0;
        gamma[ 2 ] = m_a0;
        gamma[ 3 ] = 1.0f;
    }
    const static int m_nOrders = 4;
    std::array < float, m_nOrders >  m_gamma;
    std::array < sjf_lpfFirst, m_nOrders > m_lpf;
    float m_a0 = 0.2f, m_K = 0.0f, m_alpha0, m_preY, m_SR = 44100.0f;
                           
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_moogLadder )
};
#endif /* sjf_moogLadder_h */


