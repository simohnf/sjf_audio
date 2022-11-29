//
//  sjf_biquadCascade_h
//
//  Created by Simon Fay on 28/11/2022.
//

#ifndef sjf_biquadCascade_h
#define sjf_biquadCascade_h

#include "/Users/simonfay/Programming_Stuff/sjf_audio/sjf_biquadWrapper.h"

// implementation of biquadCascade
//
template <class T>
class sjf_biquadCascade {
public:
    sjf_biquadCascade( ) { };
    ~sjf_biquadCascade() { };
    
    T filterInput( T input )
    {
        for ( int i = 0; i < m_nOrders; i++ )
        {
            input = cascade[ i ].filterInput( input );
        }
        return input;
    }
    
    void setNumOrders( int nOrders )
    {
        jassert( nOrders > 0 && nOrders <= 5);
    }
    
    enum filterDesign
    {
        butterworth
    };
   
private:
    
    
    // coefficients for butterworth filters of up to 10 orders
    // [ stage ] [ order ]
    // just for avoiding any possible divide by 0 I have set default values (i.e. not in use values) to '1'
    const T butterworthCoefficients[ 5 ][ 10 ] =
    {
        { 0.7071, 0.7071, 1, 0.5412, 0.618, 0.5176, 0.555, 0.5098, 0.5321, 0.5062 }, // stage 1
        { 1, 1, 1, 1.3065, 1.6182, 0.7071, 0.8019, 0.6013, 0.6527, 0.5612 },
        { 1, 1, 1, 1, 1, 1.9319, 2.2471, 0.9, 1, 0.7071 },
        { 1, 1, 1, 1, 1, 1, 1, 2.5628, 2.8785, 1.1013 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 3.197 } // stage 5
    };
    
    std::array< sjf_biquadWrapper < T >, 5 > cascade;
    int m_nOrders = 1;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_biquadCascade )
};


#endif /* sjf_biquadCascade_h */




