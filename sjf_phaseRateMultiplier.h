//
//  sjf_phaseRateMultiplier.h
//
//  Created by Simon Fay on 17/11/2022.
//


// rate changer for phase ramps (0 --> 1)
#ifndef sjf_phaseRateMultiplier_h
#define sjf_phaseRateMultiplier_h

class sjf_phaseRateMultiplier
{
public:
    sjf_phaseRateMultiplier(){};
    ~sjf_phaseRateMultiplier(){};
    
    float rateChange( float x )
    {
        if ( x < m_lastIn * 0.5 ) // multiply by 0.5 to compensate for slight inaccuracies with sync increment
        {
            DBG(x << " " << m_lastIn);
            m_accum += 1.0f;
            DBG( "one more ");
        }
        m_lastIn = x;
        x *= m_rate;
        x += ( m_accum * m_rate );
        x -= (int)x;
        return x;
    }
    
    void setRate( float rate )
    {
        m_rate = 1.0f / rate;
    }
    
    void setInvertedRate( float invertedRate )
    {
        m_rate = invertedRate;
    }
private:
    float m_lastIn = 1.0f, m_rate = 1.0f, m_accum = 0.0f;
};
#endif /* sjf_phaseRateMultiplier_h */

