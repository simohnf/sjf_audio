//
//  sjf_biquadWrapper_h
//
//  Created by Simon Fay on 28/11/2022.
//

#ifndef sjf_biquadWrapper_h
#define sjf_biquadWrapper_h

#include "sjf_biquadCalculator.h"
#include "sjf_biquad.h"

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
        m_biquad.setCoefficients( m_calculator.getCoefficients() );
    }
    
    void setFrequency( T f )
    {
        if ( m_calculator.getFrequency() != f )
        {
            m_calculator.setFrequency( f );
            m_biquad.setCoefficients( m_calculator.getCoefficients() );
        }
    }
    
    T getFrequency()
    {
        return m_calculator.getFrequency();
    }
    
    void setQFactor( T q )
    {
        if ( m_calculator.getQFactor() != q )
        {
            m_calculator.setQFactor( q );
            m_biquad.setCoefficients( m_calculator.getCoefficients() );
        }
//        DBG("q " << q);
    }
    
    T getQ()
    {
        return m_calculator.getQFactor();
    }
    
    void setOrder( bool isFirstOrder )
    {
        if ( m_calculator.isFirstOrder() != isFirstOrder )
        {
            m_calculator.setOrder ( isFirstOrder );
            m_biquad.setCoefficients( m_calculator.getCoefficients() );
        }
    }
    
    bool isFirstOrder()
    {
        return m_calculator.m_isFirstOrder;
    }
    
    void setFilterType( int type )
    {
        if ( m_calculator.getFilterType() != type )
        {
            m_biquad.clear();
            m_calculator.setFilterType( type );
            m_biquad.setCoefficients( m_calculator.getCoefficients() );
        }
    }
    
    std::vector< T > getCoefficients()
    {
        return m_calculator.getCoefficients();
    }
    
    T filterInput( T input )
    {
        return m_biquad.filterInput( input );
    }
    
    void clear()
    {
        m_biquad.clear();
    }
    
private:
    sjf_biquad< T > m_biquad;
    sjf_biquadCalculator< T > m_calculator;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_biquadWrapper )
};


#endif /* sjf_biquadWrapper_h */





