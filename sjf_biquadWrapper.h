//
//  sjf_biquadWrapper_h
//
//  Created by Simon Fay on 28/11/2022.
//

#ifndef sjf_biquadWrapper_h
#define sjf_biquadWrapper_h

#include "/Users/simonfay/Programming_Stuff/sjf_audio/sjf_biquadCalculator.h"
#include "/Users/simonfay/Programming_Stuff/sjf_audio/sjf_biquad.h"

// wrapper for containing both biquad and coefficient calculator in one object
//

template < class T >
class sjf_biquadWrapper {
public:
    sjf_biquadWrapper() { };
    ~sjf_biquadWrapper(){ };
    
    void initialise( T sampleRate )
    {
        m_calculator.initialise( sampleRate );
    }
    
    void setFrequency( T f )
    {
        m_calculator.setFrequency( f );
        m_biquad.setCoefficients( m_calculator.getCoefficients() );
    }
    
    void setQFactor( T q )
    {
        m_calculator.setQFactor( q );
        m_biquad.setCoefficients( m_calculator.getCoefficients() );
//        DBG("q " << q);
    }
    
    void setOrder( bool isFirstOrder )
    {
        m_calculator.setOrder ( isFirstOrder );
        m_biquad.setCoefficients( m_calculator.getCoefficients() );
    }
    
    void filterType( int type )
    {
        m_calculator.filterType( type );
        m_biquad.setCoefficients( m_calculator.getCoefficients() );
    }
    
    std::vector< T > getCoefficients()
    {
        return m_calculator.getCoefficients();
    }
    
private:
    sjf_biquad< T > m_biquad;
    sjf_biquadCalculator< T > m_calculator;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_biquadWrapper )
};


#endif /* sjf_biquadWrapper_h */





