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
    
    float filterInput( float val )
    {
        val *= ( 1.0f + (m_bassBoost * m_K) );
        
        m_preY = 0.0f;
        for ( int o = 0; o < m_nOrders; o++ )
        {
            m_preY += m_lpf[ o ].getY() * m_gamma[ o ];
        }
        m_preY *= m_K;
        val -= m_preY;
        val = tanh( val ) * m_alpha;
        for ( int o = 0; o < m_nOrders; o++ )
        {
            val = m_lpf[ o ].filterInput( val );
        }
        return val;
    }
    
    void setSampleRate( float sr )
    {
        m_SR = sr;
    }
    
    void setCoefficent( float c )
    {
        // c must be 0 --> 1 !!!
        m_a = c;
        for ( int o = 0; o < m_nOrders; o++ )
        {
            m_lpf[ o ].setCutoff( m_a );
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
    
    void setBassBoost ( float boost )
    {
        // must be between 0 and 1 !!!!
        m_bassBoost = fmin( fmax( boost, 0.0f ), 1.0f );
    }
    
    void setResonanceQuick( float r )
    {
        // must be 0 --> 4 for stability
        m_K = r;
        calculateAlpha();
    }
    
    void setBassBoostQuick ( float boost )
    {
        // must be between 0 and 1 !!!!
        m_bassBoost = boost;
    }
    
private:
    void calculateAlpha()
    {
        m_alpha = 1.0f / ( 1.0f + (m_a * m_a * m_a * m_a * m_K) );
    }
    void calculateGammas()
    {
        m_gamma[ 0 ] = m_a * m_a * m_a;
        m_gamma[ 1 ] = m_a * m_a;
        m_gamma[ 2 ] = m_a;
        m_gamma[ 3 ] = 1.0f;
    }
    
    const static int m_nOrders = 4;
    std::array < float, m_nOrders >  m_gamma;
    std::array < sjf_lpfFirst, m_nOrders > m_lpf;
    float m_bassBoost = 0.0f, m_a = 0.2f, m_K = 0.0f, m_alpha, m_preY = 0.0f, m_SR = 44100.0f;
                           
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_moogLadder )
};
#endif /* sjf_moogLadder_h */


