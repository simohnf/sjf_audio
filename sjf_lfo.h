//
//  sjf_lfo.h
//
//  Created by Simon Fay on 17/10/2022.
//

#ifndef sjf_lfo_h
#define sjf_lfo_h
#define PI 3.14159265

#include "/Users/simonfay/Programming_Stuff/sjf_audio/sjf_phaseRateMultiplier.h"
#include "/Users/simonfay/Programming_Stuff/sjf_audio/sjf_audioUtilities.h"
#include "/Users/simonfay/Programming_Stuff/sjf_audio/sjf_triangle.h"
#include "/Users/simonfay/Programming_Stuff/sjf_audio/sjf_noiseOSC.h"
#include "/Users/simonfay/Programming_Stuff/sjf_audio/sjf_lpfFirst.h"

class sjf_lfo
{
public:
    sjf_lfo(){};
    ~sjf_lfo(){};
    
    float output()
    {
        m_phase = m_count / m_SR;
        m_phase = m_rateMultiplier.rateChange( m_phase );
        
        switch( m_lfoType )
        {
            case sine:
                m_out = sin( 2*PI*m_phase );
                break;
            case triangle:
                m_out = m_triangle.output( m_phase );
                break;
            case noise1:
                if ( m_phase < m_lastPhase )
                {
                    m_out = ( rand01() * 2.0f ) -1.0f;
                }
                break;
            case noise2:
                m_out = m_noise2.output( m_phase );
                break;
            default:
                m_out = sin( 2*PI*m_phase );
                break;
        }
        
        m_lastPhase = m_phase;
        m_count++;
        while ( m_count >= m_SR )
        {
            m_count -= m_SR;
        }
        
        if ( m_lfoType == noise1 )
        { // smooth out random changes slightly
            return lpf.filterInput ( m_out );
        }
        return m_out;
    }
    
    void setSampleRate( float sr )
    {
        m_SR = sr;
        lpf.setCutoff( sin( 20 * 2 * 3.141593 / m_SR ) );
    }
    
    void setLFOtype( int type )
    {
        m_lfoType = type;
    }
    
    void setTriangleDuty( float d )
    {
        m_triangle.setDuty( d );
    }
    
    void setRateChange( float r )
    {
        m_rateMultiplier.setRate( r );
    }
    
    enum lfoType
    {
        sine = 1, triangle, noise1, noise2
    };
    
private:
    float m_SR = 44100.0f, m_phase = 0.0f, m_out = 0.0f, m_lastPhase = 1.0f;
    int m_count = 0, m_lfoType = 0;
    sjf_phaseRateMultiplier m_rateMultiplier;
    sjf_triangle m_triangle;
    sjf_noiseOSC m_noise2;
    sjf_lpfFirst lpf; // just for safety
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_lfo )
};
#endif /* sjf_lfo_h */

