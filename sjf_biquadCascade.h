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
    
    void setFilterDesign( const int design )
    {
        m_design = design;
        setQFactors();
        setFrequency( m_f0 );
    }
    
    void setNumOrders( int nOrders )
    {
//        DBG(" NUM ORDERS " << nOrders);
        jassert( nOrders > 0 && nOrders <= MAX_ORDER );
        if ( m_nOrders != nOrders )
        {
            for ( int i = 0; i < m_nStages; i ++ ) // set all stages but last of an odd number of orders to be second order... NOT WORKING
            {
                cascade[ i ].clear();
            }
        }
        m_nOrders = nOrders;
        m_nStages = ceil( m_nOrders * 0.5 ); // number of stages in us is equal to ceil(nOrders / 2)

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
        switch ( m_design )
        {
            case( butterworth ):
                for ( int s = 0; s < m_nStages; s++ )
                {
//                    DBG( "stage " << s );
                    cascade[ s ].setFrequency( m_f0 * butterworthFSFCoefficients[ s ][ m_nOrders - 1 ] );
                }
                break;
            case( bessel ):
                for ( int s = 0; s < m_nStages; s++ )
                {
//                    DBG( "stage " << s );
                    cascade[ s ].setFrequency( m_f0 * besselFSFCoefficients[ s ][ m_nOrders - 1 ] );
                }
                break;
            case( chebyshev ):
                for ( int s = 0; s < m_nStages; s++ )
                {
                    //                    DBG( "stage " << s );
                    cascade[ s ].setFrequency( m_f0 * chebyshev1dbFSFCoefficients[ s ][ m_nOrders - 1 ] );
                }
                break;
            default:
                for ( int s = 0; s < m_nStages; s++ )
                {
//                    DBG( "stage " << s );
                    cascade[ s ].setFrequency( m_f0 * butterworthFSFCoefficients[ s ][ m_nOrders - 1 ] );
                }
                break;
        }
    }
    
    void setFilterType( int type )
    {
        for ( int i = 0; i < cascade.size(); i++ )
        {
//            if ( cascade[ i ].getFilterType)
            cascade[ i ].setFilterType( type );
        }
        setFrequency( m_f0 );
    }

    
    enum filterDesign
    {
        butterworth = 1, bessel, chebyshev
    };
   
private:
    
    void setQFactors()
    {
        auto stages = cascade.size();
        switch ( m_design )
        {
            case( butterworth ):
                for ( int s = 0; s < stages; s++ )
                {
//                    DBG( "stage " << s );
                    cascade[ s ].setQFactor( butterworthQCoefficients[ s ][ m_nOrders - 1 ] );
                }
                break;
            case( bessel ):
                for ( int s = 0; s < stages; s++ )
                {
//                    DBG( "stage " << s );
                    cascade[ s ].setQFactor( besselQCoefficients[ s ][ m_nOrders - 1 ] );
                }
                break;
            case( chebyshev ):
                for ( int s = 0; s < stages; s++ )
                {
                    //                    DBG( "stage " << s );
                    cascade[ s ].setQFactor( chebyshev1dbQCoefficients[ s ][ m_nOrders - 1 ] );
                }
                break;
            default:
                for ( int s = 0; s < stages; s++ )
                {
//                    DBG( "stage " << s );
                    cascade[ s ].setQFactor( butterworthQCoefficients[ s ][ m_nOrders - 1 ] );
                }
                break;
        }
    
        

    }
    
    // coefficients for butterworth filters of up to 10 orders
    // [ stage ] [ order ]
    // just for avoiding any possible divide by 0 I have set default values (i.e. not in use values) to '1'
    // from https://www.researchgate.net/publication/264896983_An_UHF_frequency-modulated_continuous_wave_wind_profiler-receiver_and_audio_module_development
    static constexpr T butterworthQCoefficients[ MAX_NUM_STAGES ][ MAX_ORDER ] =
    {
        { 0.7071, 0.7071, 1, 0.5412, 0.618, 0.5176, 0.555, 0.5098, 0.5321, 0.5062 }, // stage 1
        { 1, 1, 1, 1.3065, 1.6182, 0.7071, 0.8019, 0.6013, 0.6527, 0.5612 }, // stage 2
        { 1, 1, 1, 1, 1, 1.9319, 2.2471, 0.9, 1, 0.7071 }, // stage 3
        { 1, 1, 1, 1, 1, 1, 1, 2.5628, 2.8785, 1.1013 }, // stage 4
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 3.197 } // stage 5
    };
    
    static constexpr T butterworthFSFCoefficients[ MAX_NUM_STAGES ][ MAX_ORDER ] =
    { // table didn't give first order...
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // stage 1
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // stage 2
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // stage 3
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // stage 4
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 } // stage 5
    };
    
    static constexpr T besselQCoefficients[ MAX_NUM_STAGES ][ MAX_ORDER ] =
    { // table didn't give first order...
        { 1, 0.5773, 0.6910, 0.5219, 0.5635, 0.5103, 0.5324, 0.5060, 0.5197, 0.5040 }, // stage 1
        { 1, 1, 1, 0.8055, 0.9165, 0.6112, 0.6608, 1.2258, 0.5894, 0.5380 }, // stage 2
        { 1, 1, 1, 1, 1, 1.0234, 1.1262, 0.7109, 0.7606, 0.6200 }, // stage 3
        { 1, 1, 1, 1, 1, 1, 1, 0.5596, 1.3220, 0.8100 }, // stage 4
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1.4150 } // stage 5
    };
    
    static constexpr T besselFSFCoefficients[ MAX_NUM_STAGES ][ MAX_ORDER ] =
    { // table didn't give first order...
        { 1, 1.2736, 1.4524, 1.4192, 1.5611, 1.6060, 1.7174, 1.7837, 1.8794, 1.9490 }, // stage 1
        { 1, 1, 1.3270, 1.5912, 1.7607, 1.6913, 1.8235, 2.1953, 1.9488, 1.9870 }, // stage 2
        { 1, 1, 1, 1, 1.5069, 1.9071, 2.0507, 1.9591, 2.0815, 2.0680 }, // stage 3
        { 1, 1, 1, 1, 1, 1, 1.6853, 1.8376, 2.3235, 2.2210 }, // stage 4
        { 1, 1, 1, 1, 1, 1, 1, 1, 1.8575, 2.4850 } // stage 5
    };
    
    static constexpr T chebyshev1dbQCoefficients[ MAX_NUM_STAGES ][ MAX_ORDER ] =
    { // table didn't give first order...
        { 1, 0.9565, 2.0176, 0.7845, 1.3988, 0.7608, 1.2967, 0.7530, 1.1964, 0.7495 }, // stage 1
        { 1, 1, 1, 3.5600, 5.5538, 2.1977, 3.1544, 1.9564, 2.7119, 1.8639 }, // stage 2
        { 1, 1, 1, 1, 1, 8.0012, 10.9010, 2.7776, 5.5239, 3.5609 }, // stage 3
        { 1, 1, 1, 1, 1, 1, 1, 14.2445, 18.0069, 6.9419 }, // stage 4
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 22.2779 } // stage 5
    };
    
    static constexpr T chebyshev1dbFSFCoefficients[ MAX_NUM_STAGES ][ MAX_ORDER ] =
    { // table didn't give first order...
        { 1, 1.0500, 0.9971, 0.9932, 0.9941, 0.9953, 0.9963, 0.9971, 0.9976, 0.9981 }, // stage 1
        { 1, 1, 0.4942, 0.5286, 0.6652, 0.7468, 0.8084, 0.5538, 0.8805, 0.7214 }, // stage 2
        { 1, 1, 1, 1, 0.2895, 0.3532, 0.4800, 0.5838, 0.6623, 0.9024 }, // stage 3
        { 1, 1, 1, 1, 1, 1, 0.2054, 0.2651, 0.3812, 0.4760 }, // stage 4
        { 1, 1, 1, 1, 1, 1, 1, 1, 0.1593, 0.2121 } // stage 5
    };
    
    std::array< sjf_biquadWrapper < T >, MAX_NUM_STAGES > cascade;
    int m_nOrders = 3, m_nStages = 2;
    int m_design = filterDesign::butterworth;
    T m_f0 = 1000;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_biquadCascade )
};


#endif /* sjf_biquadCascade_h */




