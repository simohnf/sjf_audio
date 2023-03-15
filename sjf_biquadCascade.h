//
//  sjf_biquadCascade_h
//
//  Created by Simon Fay on 28/11/2022.
//

#ifndef sjf_biquadCascade_h
#define sjf_biquadCascade_h

#include "/Users/simonfay/Programming_Stuff/sjf_audio/sjf_biquadWrapper.h"

// implementation of biquadCascade
// maximum of 5 stages ==> 10th order
#define MAX_NUM_STAGES 5
#define MAX_ORDER MAX_NUM_STAGES*2
template <class T>
class sjf_biquadCascade {
public:
    sjf_biquadCascade( )
    {
        setNumOrders( 3 ); // start with a third order filter... just because...
    };
    ~sjf_biquadCascade() { };
    
    void initialise( T sampleRate )
    {
        for ( int i = 0; i < cascade.size(); i++ )
        {
            cascade[ i ].initialise( sampleRate );
        }
    }
    
    
    T filterInput( T input )
    {
        for ( int i = 0; i < m_nStages; i++ )
        {
            input = cascade[ i ].filterInput( input );
        }
        return input;
    }
    
    void setNumOrders( int nOrders )
    {
        DBG(" NUM ORDERS " << nOrders);
        jassert( nOrders > 0 && nOrders <= MAX_ORDER );
        m_nOrders = nOrders;
        m_nStages = ceil( m_nOrders * 0.5 ); // number of stages in us is equal to ceil(nOrders / 2)
        DBG("STages == " << m_nStages);
        DBG("nOrders % 2 == " << m_nOrders %2);
        for ( int i = 0; i < m_nStages; i ++ ) // set all stages but last of an odd number of orders to be second order... NOT WORKING
        {
            if ( i == (m_nStages - 1) && m_nOrders %2 != 0 )
            {
                cascade[ i ].setOrder( true );
            }
            else
            {
                cascade[ i ].setOrder( false );
            }
        }
        setQFactors();
        setFrequency( m_f0 );
    }
    
    void setFrequency( T f )
    {
        m_f0 = f;
        for ( int s = 0; s < m_nStages; s++ )
        {
            DBG( "stage " << s );
            cascade[ s ].setFrequency( m_f0 );
        }
    }
    
    void setFilterType( int type )
    {
        for ( int i = 0; i < cascade.size(); i++ )
        {
            cascade[ i ].setFilterType( type );
        }
        setFrequency( m_f0 );
    }

    
    enum filterDesign
    {
        butterworth
    };
   
private:
    
    void setQFactors()
    {
        auto stages = cascade.size();
        for ( int s = 0; s < stages; s++ )
        {
            DBG( "stage " << s );
            cascade[ s ].setQFactor( butterworthCoefficients[ s ][ m_nOrders - 1 ] );
        }
    }
    
    // coefficients for butterworth filters of up to 10 orders
    // [ stage ] [ order ]
    // just for avoiding any possible divide by 0 I have set default values (i.e. not in use values) to '1'
    const T butterworthCoefficients[ MAX_NUM_STAGES ][ MAX_ORDER ] =
    {
        { 0.7071, 0.7071, 1, 0.5412, 0.618, 0.5176, 0.555, 0.5098, 0.5321, 0.5062 }, // stage 1
        { 1, 1, 1, 1.3065, 1.6182, 0.7071, 0.8019, 0.6013, 0.6527, 0.5612 }, // stage 2
        { 1, 1, 1, 1, 1, 1.9319, 2.2471, 0.9, 1, 0.7071 }, // stage 3
        { 1, 1, 1, 1, 1, 1, 1, 2.5628, 2.8785, 1.1013 }, // stage 4
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 3.197 } // stage 5
    };
    
    std::array< sjf_biquadWrapper < T >, MAX_NUM_STAGES > cascade;
    int m_nOrders = 3, m_nStages = 2;
    int type = filterDesign::butterworth;
    T m_f0 = 1000;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_biquadCascade )
};


#endif /* sjf_biquadCascade_h */




