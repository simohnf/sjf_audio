//
//  sjf_comb_h
//
//  Created by Simon Fay on 01/02/2023.
//

#ifndef sjf_comb_h
#define sjf_comb_h

#include <limits>
#include "sjf_delayLine.h"
#include "sjf_circularBuffer.h"
#include "sjf_lpf.h"
#include "sjf_audioUtilitiesC++.h"

// implementation of universal comb filter (Zolzer DAFX)
//

template <class T>
class sjf_comb
{
private:
    
    sjf_circularBuffer < T > m_delayLine;
    sjf_lpf< T > m_lpf;
    T m_blend; // blend gain (BL)
    T m_feedforward; // feedforward gain ( FF )
    T m_feedback; // feedBack gain ( FB )
    T m_delayInSamps = 1;
    size_t m_roundedDelay = 1;
    bool m_shouldFilter = false;
public:
    sjf_comb(){};
    ~sjf_comb(){};
    
    
    void initialise( const int MaxDelayInSamps )
    {
        m_delayLine.initialise( MaxDelayInSamps );
    }
    
    T filterInput( const T input )
    {
        T delayed = m_shouldFilter ? m_lpf.filterInput( m_delayLine.getSample( m_delayInSamps ) ) : m_delayLine.getSample( m_delayInSamps );
        T delayedFB = delayed * m_feedback;
        m_delayLine.setSample( input + delayedFB );
        return ( ( input + delayedFB ) * m_blend ) + ( delayed * m_feedforward );
    }
    
    void filterInPlace( T& input )
    {
        T delayed = m_shouldFilter ? m_lpf.filterInput( m_delayLine.getSample( m_delayInSamps ) ) : m_delayLine.getSample( m_delayInSamps );
        T delayedFB = delayed * m_feedback;
        m_delayLine.setSample( input + delayedFB );
        input = ( ( input + delayedFB ) * m_blend ) + ( delayed * m_feedforward );
    }
    
    T filterInputRoundedIndex( const T& input )
    {
        T delayed = m_shouldFilter ? m_lpf.filterInput( m_delayLine.getSample( m_roundedDelay ) ): m_delayLine.getSample( m_roundedDelay );
        T delayedFB = delayed * m_feedback;
        m_delayLine.setSample( input + delayedFB );
        return ( ( input + delayedFB ) * m_blend ) + ( delayed * m_feedforward );
    }
    
    void filterInPlaceRoundedIndex( T& input )
    {
        T delayed = m_shouldFilter ? m_lpf.filterInput( m_delayLine.getSample( m_roundedDelay ) ): m_delayLine.getSample( m_roundedDelay );
        T delayedFB = delayed * m_feedback;
        m_delayLine.setSample( input + delayedFB );
        input = ( ( input + delayedFB ) * m_blend ) + ( delayed * m_feedforward );
    }
    
    void setDelayTimeSamps( const T& delayInSamps )
    {
        m_delayInSamps = delayInSamps;
        m_roundedDelay = static_cast< size_t >( std::round ( m_delayInSamps ) );
    }
    
    T getDelayTimeSamps( )
    {
        return m_delayInSamps;
    }
    
    void setCoefficients( const T blend, const T feedforward, const T feedback )
    {
        m_blend = blend;
        m_feedforward = feedforward;
        m_feedback = feedback;
    }
    
    void setInterpolationType( const int &interpolationType )
    {
        m_delayLine.setInterpolationType( interpolationType );
    }

    T size()
    {
        return m_delayLine.getSize();
    }
    
    void clearDelayline( )
    {
        m_lpf.reset();
        m_delayLine.clear();
    }
    
    void setShouldFilter( bool trueIfFilterOn )
    {
        m_shouldFilter = trueIfFilterOn;
    }
    
    void setLPFCoefficient( T coef )
    {
        m_lpf.setCoefficient( coef );
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_comb )
};


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// One multiply allpass as per Moorer - "about this reverberation business"
template < typename T >
class sjf_allpass
{
private:
    sjf_circularBuffer< T > m_delayLine;
    sjf_lpf< T > m_lpf;
    T m_gain;
    T m_delayInSamps = 1;
    size_t m_roundedDelay = 1;
    bool m_shouldFilter = false;
    
public:
    sjf_allpass() {}
    ~sjf_allpass() {}
    
    void initialise( const int MaxDelayInSamps )
    {
        m_delayLine.initialise( MaxDelayInSamps );
    }
    
    T filterInput( const T input )
    {
        T delayed = m_shouldFilter ? m_lpf.filterInput( m_delayLine.getSample( m_delayInSamps ) ) : m_delayLine.getSample( m_delayInSamps ) ;
        T xhn = ( input - delayed ) * m_gain;
        m_delayLine.setSample( input + xhn );
        return delayed + xhn;
    }
    
    void filterInPlace( T& input )
    {
        T delayed = m_shouldFilter ? m_lpf.filterInput( m_delayLine.getSample( m_delayInSamps ) ) : m_delayLine.getSample( m_delayInSamps ) ;
        T xhn = ( input - delayed ) * m_gain;
        m_delayLine.setSample( input + xhn );
        input = delayed + xhn;
    }
    
    void filterInPlace2Multiply( T& input )
    {
        T delayed = m_shouldFilter ? m_lpf.filterInput( m_delayLine.getSample( m_delayInSamps ) ) : m_delayLine.getSample( m_delayInSamps ) ;
        T xhn = input - ( delayed * m_gain );
        m_delayLine.setSample( xhn );
        input = delayed + ( xhn * m_gain );
    }
    
    T filterInputRoundedIndex( const T input )
    {
        T delayed = m_shouldFilter ? m_lpf.filterInput( m_delayLine.getSample( m_roundedDelay ) ): m_delayLine.getSample( m_roundedDelay );
        T xhn = ( input - delayed ) * m_gain;
        m_delayLine.setSample( input + xhn );
        return delayed + xhn;
    }
    
    void filterInPlaceRoundedIndex( T& input )
    {
        T delayed = m_shouldFilter ? m_lpf.filterInput( m_delayLine.getSample( m_roundedDelay ) ): m_delayLine.getSample( m_roundedDelay );
        T xhn = ( input - delayed ) * m_gain;
        m_delayLine.setSample( input + xhn );
        input = delayed + xhn;
    }
    
    void setDelayTimeSamps( const T delayInSamps )
    {
        m_delayInSamps = delayInSamps;
        m_roundedDelay = static_cast< size_t >( std::round( m_delayInSamps ) );
    }
    
    T getDelayTimeSamps( )
    {
        return m_delayInSamps;
    }
    
    void setCoefficient( const T &gain )
    {
        m_gain = gain;
    }
    
    void clearDelayline( )
    {
        m_lpf.reset();
        m_delayLine.clear();
    }
    
    void setInterpolationType( const int &interpolationType )
    {
        m_delayLine.setInterpolationType( interpolationType );
    }
    
    auto size()
    {
        return m_delayLine.getSize();
    }
    
    void setShouldFilter( bool trueIfFilterOn )
    {
        m_shouldFilter = trueIfFilterOn;
    }
    
    void setLPFCoefficient( T coef )
    {
        m_lpf.setCoefficient( coef );
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_allpass )
};



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

template <class T>
class sjf_fbComb
{
private:
    
    sjf_circularBuffer < T > m_delayLine;
    sjf_lpf< T > m_lpf;
    T m_feedback; // feedBack gain ( FB )
    T m_delayInSamps = 1;
    size_t m_roundedDelay = 1;
    bool m_shouldFilter = false;
    
public:
    sjf_fbComb(){};
    ~sjf_fbComb(){};
    
    
    void initialise( const int MaxDelayInSamps )
    {
        m_delayLine.initialise( MaxDelayInSamps );
    }
    
    T filterInput( T input )
    {
        T delayed = m_shouldFilter ? m_lpf.filterInput( m_delayLine.getSample( m_delayInSamps ) ) : m_delayLine.getSample( m_delayInSamps );
        input += ( delayed * m_feedback );
        m_delayLine.setSample( input );
        return input;
    }
    
    void filterInPlace( T& input )
    {
        T delayed = m_shouldFilter ? m_lpf.filterInput( m_delayLine.getSample( m_delayInSamps ) ) : m_delayLine.getSample( m_delayInSamps );
        input += ( delayed * m_feedback );
        m_delayLine.setSample( input );
    }
    
    T filterInputRoundedIndex( T input )
    {
        
        T delayed = m_shouldFilter ? m_lpf.filterInput( m_delayLine.getSample( m_roundedDelay ) ): m_delayLine.getSample( m_roundedDelay );
        input += ( delayed * m_feedback );
        m_delayLine.setSample( input );
        return input;
    }
    
    void filterInPlaceRoundedIndex( T& input )
    {
        T delayed = m_shouldFilter ? m_lpf.filterInput( m_delayLine.getSample( m_roundedDelay ) ): m_delayLine.getSample( m_roundedDelay );
        input += ( delayed * m_feedback );
        m_delayLine.setSample( input );
    }
    
    // comb as per Moorer - about this reverberation business
    T delay( T input )
    {
        T delayed = m_shouldFilter ? m_lpf.filterInput( m_delayLine.getSample( m_delayInSamps ) ) : m_delayLine.getSample( m_delayInSamps );
        m_delayLine.setSample( input + ( delayed * m_feedback ) );
        return delayed;
    }
    
    // comb as per Moorer - about this reverberation business
    T delayRoundedIndex( T input )
    {
        T delayed = m_shouldFilter ? m_lpf.filterInput( m_delayLine.getSample( m_roundedDelay ) ): m_delayLine.getSample( m_roundedDelay );
        m_delayLine.setSample( input + ( delayed * m_feedback ) );
        return delayed;
    }
    
    void setDelayTimeSamps( const T delayInSamps )
    {
        m_delayInSamps = delayInSamps;
        m_roundedDelay = static_cast< size_t >( std::round( m_delayInSamps ) );
    }
    
    T getDelayTimeSamps( )
    {
        return m_delayInSamps;
    }
    
    void setCoefficient( const T feedback )
    {
        m_feedback = feedback;
    }
    
    void setInterpolationType( const int &interpolationType )
    {
        m_delayLine.setInterpolationType( interpolationType );
    }
    
    T size( )
    {
        return m_delayLine.getSize();
    }
    
    void clearDelayline( )
    {
        m_lpf.reset();
        m_delayLine.clear();
    }
    
    void setShouldFilter( bool trueIfFilterOn )
    {
        m_shouldFilter = trueIfFilterOn;
    }
    
    void setLPFCoefficient( T coef )
    {
        m_lpf.setCoefficient( coef );
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_fbComb )
};
// nested allpass as per moorer - about this reverberation business


// nested allpass as per moorer - about this reverberation business
//template< typename T, int NUMSTAGES >
//class sjf_nestedAllPass
//{
//private:
//    static constexpr int NSTAGES = std::max( NUMSTAGES, 2 );
//
//
//    bool m_shouldFilter = false;
//    std::array< sjf_circularBuffer< T >, NSTAGES > m_apStages;
//    std::array< sjf_lpf< T >,  NSTAGES > m_lpf;
//    std::array< T,  NSTAGES > m_gains = { 0.6, 0.6 };
//    std::array< T,  NSTAGES > m_delaysInSamps = { 1, 1 };
//    std::array< size_t, NSTAGES > m_roundedDelays = { 1, 1 };
//    std::array< T, NSTAGES > m_delayedSamples;
//    std::array< T, NSTAGES > m_xhns;
//public:
//    sjf_nestedAllPass()
//    {
//        for ( auto i = 0; i < NSTAGES; i++ )
//        {
//            setCoefficient( 0.5, i );
//            setDelayTimeSamps( 10, i );
//            m_delayedSamples[ i ] = m_xhns[ i ] = 0;
//        }
//    }
//    ~sjf_nestedAllPass(){}
//
//    void initialise( const int MaxDelayInSamps )
//    {
//        for ( auto & ap : m_apStages )
//            ap.initialise( MaxDelayInSamps );
//    }
//
//
//    T process( T input, bool delayTimeIsFractional )
//    {
//        auto finalStage = NSTAGES - 1;
//
//        for ( auto i = 0; i < NSTAGES-1; i++ )
//        {
//            // get delayed signal at this stage
//            m_delayedSamples[ i ] = delayTimeIsFractional ? m_apStages[ i ].getSample( m_delaysInSamps[ i ] ) : m_apStages[ i ].getSample( m_roundedDelays[ i ] );
//            // to filter or not to filter
//            m_delayedSamples[ i ] = m_shouldFilter ? m_lpf[ i ].filterInput( m_delayedSamples[ i ] ) : m_delayedSamples[ i ];
//            // one multiply value
//            m_xhns[ i ] = ( input - m_delayedSamples[ i ] ) * m_gains[ i ];
//            input += m_xhns[ i ];
//        }
//
//        // final stage is a normal allpass
//        m_delayedSamples[ finalStage ] = delayTimeIsFractional ? m_apStages[ finalStage ].getSample( m_delaysInSamps[ finalStage ] ) : m_apStages[ finalStage ].getSample( m_roundedDelays[ finalStage ] );
//        m_delayedSamples[ finalStage ] = m_shouldFilter ? m_lpf[ finalStage ].filterInput( m_delayedSamples[ finalStage ] ) : m_delayedSamples[ finalStage ];
//        m_xhns[ finalStage ] = ( input - m_delayedSamples[ finalStage ] ) * m_gains[ finalStage ];
//        m_apStages[ finalStage ].setSample( input + m_xhns[ finalStage ] );
//        input = m_delayedSamples[ finalStage ] + m_xhns[ finalStage ];
//
//        // then go backwards
//        for ( auto i = NSTAGES-1; i >= 0; i-- )
//        {
//            // set value in circular buffer
//            m_apStages[ i ].setSample( input );
//            // calculate input to previous stage
//            input = m_delayedSamples[ i ] + m_xhns[ i ];
//        }
//        return input;
//    }
//
//
//
//
//
//
//    void setDelayTimeSamps( const T delayInSamps, size_t voice )
//    {
//        m_delaysInSamps[ voice ] = delayInSamps;
//        m_roundedDelays[ voice ] = static_cast< size_t >( std::round( delayInSamps ) );
//    }
//
//
//    void setCoefficient( const T gain, size_t voice )
//    {
//        m_gains[ voice ] = gain;
//    }
//
//    void clearDelaylines( )
//    {
//        for ( auto & lpf : m_lpf )
//            lpf.reset();
//        for ( auto & ap : m_apStages )
//            ap.clear();
//    }
//
//    void setInterpolationType( const int &interpolationType )
//    {
//        for ( auto & ap : m_apStages )
//            ap.setInterpolationType( interpolationType );
//    }
//
//    void setShouldFilter( bool trueIfFilterOn )
//    {
//        m_shouldFilter = trueIfFilterOn;
//    }
//
//    void setLPFCoefficient( T coef, size_t voice )
//    {
//        m_lpf[ voice ].setCoefficient( coef );
//    }
//};



#endif /* sjf_comb_h */




