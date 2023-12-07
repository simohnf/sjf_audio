
//
//  sjf_lpf.h
//
//  Created by Simon Fay on 27/01/2023.
//

#ifndef sjf_lpf_h
#define sjf_lpf_h

template < class T >
class sjf_lpf
{
public:
    sjf_lpf(){};
    ~sjf_lpf(){};
    ///////////////////////////////////////////////////
    T filterInput( const T x )
    {
        m_y0 += m_b * (x - m_y0);
//        m_y1 = m_y0; // store in case user changes to second order
        return m_y0;
    }
    ///////////////////////////////////////////////////
    T filterInputHP( const T x )
    {
        m_y0 = filterInput( x );
//        m_y1 = m_y0; // store in case user changes to second order
        return (x - m_y0);
    }
    ///////////////////////////////////////////////////
    T filterInputSecondOrder( const T x )
    {
        m_y0 += m_b * (x - m_y0);
        m_y1 += m_b * (m_y0 - m_y1);
        return m_y1;
    }
    ///////////////////////////////////////////////////
    T filterInputSecondOrderHP( const T x )
    {
        m_y1 = filterInputSecondOrder( x );
        return x - m_y1;
    }
    ///////////////////////////////////////////////////
    void filterInPlace( T& x )
    {
        m_y0 += m_b * (x - m_y0);
        x = m_y0; 
    }
    ///////////////////////////////////////////////////
    void filterInPlaceHP( T& x )
    {
        m_y0 += m_b * (x - m_y0);
        x -= m_y0;
    }
    ///////////////////////////////////////////////////
    void filterInPlaceSecondOrder( T x )
    {
        m_y0 += m_b * (x - m_y0);
        m_y1 += m_b * (m_y0 - m_y1);
        x = m_y1;
    }
    ///////////////////////////////////////////////////
    void filterInPlaceSecondOrderHP( T x )
    {
        m_y0 += m_b * (x - m_y0);
        m_y1 += m_b * (m_y0 - m_y1);
        x -= m_y1;
    }
    ///////////////////////////////////////////////////
    ///// this is poorly named so I need to change every other class that uses it...
    void setCutoff( const T newCutoff )
    {
        if (newCutoff > 0.999999) { m_b = 0.999999; }
        else if( newCutoff < 0.0 ) { m_b = 0.0; }
        else { m_b = newCutoff; }
    }
    ///////////////////////////////////////////////////
    void setCoefficient( const T newCoef)
    {
        m_b = newCoef < 0 ? 0 : ( newCoef > 0.999999 ? 0.999999 : newCoef );
//        if ( newCoef > 0.999999) { m_b = 0.999999; }
//        else if( newCoef < 0.0 ) { m_b = 0.0; }
//        else { m_b = newCoef; }
    }
    ///////////////////////////////////////////////////
    T getCutoff()
    {
        return m_b;
    }
    ///////////////////////////////////////////////////
    T getY0()
    {
        return m_y0;
    }
    ///////////////////////////////////////////////////
    T getY1()
    {
        return m_y1;
    }
    //////////////////////////////////////////////////////
    void reset()
    {
        m_y0 = 0;
        m_y1 = 0;
    }
    ///////////////////////////////////////////////////
private:
    T m_y0 = 0.0, m_y1 = 0.0, m_b = 0.5;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_lpf )
};
#endif /* sjf_lpf_h */

