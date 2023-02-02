//
//  sjf_comb_h
//
//  Created by Simon Fay on 01/02/2023.
//

#ifndef sjf_comb_h
#define sjf_comb_h

// implementation of universal comb filter (Zolzer DAFX)
//

#include "sjf_delayLine.h"

template <class T>
class sjf_comb
{
private:
    sjf_delayLine < T > m_delayLine;
    T m_blend; // blend gain (BL)
    T m_feedforward; // feedforward gain ( FF )
    T m_feedback; // feedBack gain ( FB )
    
public:
    sjf_comb(){};
    ~sjf_comb(){};
    
    
    void initialise( const int &MaxDelayInSamps )
    {
        m_delayLine.initialise( MaxDelayInSamps );
    }
    T filterInput( const T &input )
    {
        T delayed = m_delayLine.getSample2( );
        T delayedFB = delayed * m_feedback;
        m_delayLine.setSample2( input + delayedFB );
//        T output = (input * BL);
        return ( ( input + delayedFB ) * m_blend ) + ( delayed * m_feedforward );
    }
    
    T filterInputRounded( const T &input )
    {
        T delayed = m_delayLine.getSampleRoundedIndex2( );
        T delayedFB = delayed * m_feedback;
        m_delayLine.setSample2( input + delayedFB );
        //        T output = (input * BL);
        return ( ( input + delayedFB ) * m_blend ) + ( delayed * m_feedforward );
    }
    
    void setDelayTimeSamps( const T &delayInSamps )
    {
        m_delayLine.setDelayTimeSamps( delayInSamps );
    }
    
    T getDelayTimeSamps(  )
    {
        return m_delayLine.getDelayTimeSamps( );
    }
    
    void setCoefficients( const T &blend, const T &feedforward, const T &feedback )
    {
        m_blend = blend;
        m_feedforward = feedforward;
        m_feedback = feedback;
    }
    

    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_comb )
};


#endif /* sjf_comb_h */




