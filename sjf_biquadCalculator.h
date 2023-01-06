//
//  sjf_biquadCalculator_h
//
//  Created by Simon Fay on 28/11/2022.
//

#ifndef sjf_biquadCalculator_h
#define sjf_biquadCalculator_h



// coefficientCalculator For biquad filter
// info from https://www.w3.org/TR/audio-eq-cookbook/
//

// changed to
/* all coefficient calculations from
 "Zölzer, U., Amatriain, X., Arfib, D., Bonada, J., De Poli, G., Dutilleux, P., Evangelista, G., Keiler, F., Loscos, A., Rocchesso, D. and Sandler, M., 2002. DAFX-Digital audio effects. John Wiley & Sons."
 */

template <class T>
class sjf_biquadCalculator {
public:
    sjf_biquadCalculator()
    {
        m_coeffs.resize( 5 );
        initialise ( 44100 );
    };
    ~sjf_biquadCalculator(){ };
    
    void initialise( T sampleRate )
    {
        m_SR = sampleRate;
        m_angFreqFactor = m_pi / m_SR;
    }
    
    void setFrequency( T fr )
    {
        m_f0 = fr;
        m_K = tan( fr * m_angFreqFactor );
//        calculateIntermediateValues();
    }
    
    void setQFactor( T Q )
    {
        m_Q = Q;
//        calculateIntermediateValues();
    }
    
    void setdBGain( T gain )
    {
        m_dBGain = gain;
        m_V0 = pow(10, (gain/20) );
//        calculateIntermediateValues();
    }
    
    std::vector< T > getCoefficients()
    {
        switch (m_type)
        {
            case lowpass:
                lowpassCoefficients();
                break;
            case highpass:
                highpassCoefficients();
                break;
            case allpass:
                allpassCoefficients();
                break;
            case lowshelf:
                lowshelfCoefficients();
                break;
            case highshelf:
                highshelfCoefficients();
                break;
            case bandpass:
                bandpassCoefficients();
                break;
            case bandcut:
                bandcutCoefficients();
                break;
            case peak:
                peakCoefficients();
                break;
        }
        return m_coeffs;
    }
    
    void setFilterType( int type )
    {
        m_type = type;
    }
    
    void setOrder( bool isFirstOrder )
    {
        m_isFirstOrder = isFirstOrder;
    }
    
   enum filterType
    {
        lowpass = 1, highpass, allpass, lowshelf, highshelf, bandpass, bandcut, peak
    };
    
private:
    
    /* all coefficient calculations from
    "Zölzer, U., Amatriain, X., Arfib, D., Bonada, J., De Poli, G., Dutilleux, P., Evangelista, G., Keiler, F., Loscos, A., Rocchesso, D. and Sandler, M., 2002. DAFX-Digital audio effects. John Wiley & Sons."
     */
    
    // lowpass
    void lowpassCoefficients()
    {
        if ( m_isFirstOrder )
        {
            T denominatorReciprocal = 1.0 / ( m_K + 1.0 );
            m_coeffs [ 0 ] = m_K * denominatorReciprocal; // b0
            m_coeffs [ 1 ] = m_coeffs [ 0 ]; // b1
            m_coeffs [ 2 ] =  0; // b2
            m_coeffs [ 3 ] = ( m_K - 1 ) * denominatorReciprocal; // a1
            m_coeffs [ 4 ] = 0; // a2
        }
        else
        {
            T K2 = m_K * m_K;
            T K2Q = K2 * m_Q;
            T denominatorReciprocal = 1.0 / ( K2Q + m_K + m_Q );
            
            m_coeffs [ 0 ] = K2Q * denominatorReciprocal; // b0
            m_coeffs [ 1 ] = 2.0 * K2Q * denominatorReciprocal; // b1
            m_coeffs [ 2 ] = m_coeffs [ 0 ]; // b2
            m_coeffs [ 3 ] = 2.0 * m_Q * ( K2 - 1.0 ) * denominatorReciprocal; // a1
            m_coeffs [ 4 ] = ( K2Q - m_K + m_Q ) * denominatorReciprocal; // a2
        }
    }
    
    void highpassCoefficients()
    {
        if ( m_isFirstOrder )
        {
            T denominatorReciprocal = 1.0 / ( m_K + 1.0 );
            m_coeffs [ 0 ] = denominatorReciprocal; // b0
            m_coeffs [ 1 ] = -1 * denominatorReciprocal; // b1
            m_coeffs [ 2 ] =  0; // b2
            m_coeffs [ 3 ] = ( m_K - 1 ) * denominatorReciprocal; // a1
            m_coeffs [ 4 ] = 0; // a2
        }
        else
        {
            T K2 = m_K * m_K;
            T K2Q = K2 * m_Q;
            T denominatorReciprocal = 1.0 / ( K2Q + m_K + m_Q );
            
            m_coeffs [ 0 ] = m_Q * denominatorReciprocal; // b0
            m_coeffs [ 1 ] = -2.0 * m_Q * denominatorReciprocal; // b1
            m_coeffs [ 2 ] = m_coeffs [ 0 ]; // b2
            m_coeffs [ 3 ] = 2.0 * m_Q * ( K2 - 1.0 ) * denominatorReciprocal; // a1
            m_coeffs [ 4 ] = ( K2Q - m_K + m_Q ) * denominatorReciprocal; // a2
        }
    }
    
    void allpassCoefficients()
    {
        if ( m_isFirstOrder )
        {
            T denominatorReciprocal = 1.0 / ( m_K + 1.0 );
            m_coeffs [ 0 ] = ( m_K - 1.0 ) * denominatorReciprocal; // b0
            m_coeffs [ 1 ] = 1.0; // b1
            m_coeffs [ 2 ] =  0; // b2
            m_coeffs [ 3 ] = m_coeffs [ 0 ]; // a1
            m_coeffs [ 4 ] = 0; // a2
        }
        else
        {
            T K2 = m_K * m_K;
            T K2Q = K2 * m_Q;
            T denominatorReciprocal = 1.0 / ( K2Q + m_K + m_Q );
            
            m_coeffs [ 0 ] = (K2Q - m_K + m_Q) * denominatorReciprocal; // b0
            m_coeffs [ 1 ] = 2.0 * m_Q * (K2 - 1.0 ) * denominatorReciprocal; // b1
            m_coeffs [ 2 ] = 1; // b2
            m_coeffs [ 3 ] = m_coeffs [ 1 ]; // a1
            m_coeffs [ 4 ] = m_coeffs [ 0 ]; // a2
        }
    }
    
    
    void lowshelfCoefficients()
    {
        if ( m_isFirstOrder ) // 1st order modified from DAFX p. 62 --> 63
        {
            T halfH0 = ( m_V0 - 1.0 ) * 0.5;
            T C;
            if ( m_V0 < 1 ) // C for cut
            {
                C = ( m_K - m_V0 ) / ( m_K + m_V0 );
            }
            else // c for boost
            {
                C = ( m_K - 1.0 ) / ( m_K + 1.0 );
            }
            T halfH0C = halfH0 * C;
            m_coeffs [ 0 ] = 1.0 + halfH0 + halfH0C; // b0
            m_coeffs [ 1 ] = C + halfH0 + halfH0C; // b1
            m_coeffs [ 2 ] =  0; // b2
            m_coeffs [ 3 ] = C; // a1
            m_coeffs [ 4 ] = 0; // a2
        }
        else
        {
            T K2 = m_K * m_K;
            T root2 = 1/m_Q; // square root of 2 is replaced by Q --> 1/0.7071 == sqrt(2)
            T rootV0 = sqrt( m_V0 );
            T root2K = root2 * m_K;
            T root2rootV0K = root2K * rootV0;
            T V0K2 = m_V0 * K2;
            if ( m_V0 < 1 ) // cut
            {
                T root2V0K = root2K * m_V0;
                T denominatorReciprocal = 1.0 / ( m_V0 + root2rootV0K + K2 );
                m_coeffs [ 0 ] = ( m_V0 + root2V0K + V0K2 ) * denominatorReciprocal; // b0
                m_coeffs [ 1 ] = ( 2.0 * V0K2 - 2.0 ) * denominatorReciprocal; // b1
                m_coeffs [ 2 ] = ( m_V0 - root2V0K + V0K2 ) * denominatorReciprocal; // b2
                m_coeffs [ 3 ] = 2.0 * (K2 - m_V0 ) * denominatorReciprocal; // a1
                m_coeffs [ 4 ] = ( m_V0 - root2rootV0K + K2 ) * denominatorReciprocal; // a2
            }
            else // boost
            {
                T denominatorReciprocal = 1.0 / ( 1.0 + root2K + K2 );
                m_coeffs [ 0 ] = ( 1.0 + root2rootV0K + V0K2 ) * denominatorReciprocal; // b0
                m_coeffs [ 1 ] = ( 2.0 * V0K2 - 2.0 ) * denominatorReciprocal; // b1
                m_coeffs [ 2 ] = ( 1.0 - root2rootV0K + V0K2 ) * denominatorReciprocal; // b2
                m_coeffs [ 3 ] = ( 2.0 * K2 - 2.0 ) * denominatorReciprocal; // a1
                m_coeffs [ 4 ] = ( 1.0 - root2K + K2 ) * denominatorReciprocal; // a2
            }
        }
    }
    
    void highshelfCoefficients()
    {
        if ( m_isFirstOrder ) // 1st order modified from DAFX p. 62 --> 63
        {
            T halfH0 = ( m_V0 - 1.0 ) * 0.5;
            T C;
            if ( m_V0 < 1 ) // C for cut
            {
                C = ( m_V0 * m_K - 1.0 ) / ( m_V0 * m_K + 1 );
            }
            else // c for boost
            {
                C = ( m_K - 1.0 ) / ( m_K + 1.0 );
            }
            T halfH0C = halfH0 * C;
            m_coeffs [ 0 ] = 1.0 + halfH0 - halfH0C; // b0
            m_coeffs [ 1 ] = C - halfH0 + halfH0C; // b1
            m_coeffs [ 2 ] =  0; // b2
            m_coeffs [ 3 ] = C; // a1
            m_coeffs [ 4 ] = 0; // a2
        }
        else
        {
            T K2 = m_K * m_K;
            T root2 = 1/m_Q; // square root of 2 is replaced by Q --> 1/0.7071 == sqrt(2)
            T rootV0 = sqrt( m_V0 );
            T root2K = root2 * m_K;
            T root2rootV0K = root2K * rootV0;
            T V0K2 = m_V0 * K2;
            if ( m_V0 < 1 ) // cut
            {
                T root2V0K = root2K * m_V0;
                T denominatorReciprocal = 1.0 / ( m_V0 + root2rootV0K + K2 );
                m_coeffs [ 0 ] = ( m_V0 + root2V0K + V0K2 ) * denominatorReciprocal; // b0
                m_coeffs [ 1 ] = 2.0 * (V0K2 - m_V0 ) * denominatorReciprocal; // b1
                m_coeffs [ 2 ] = ( m_V0 - root2V0K + V0K2  ) * denominatorReciprocal; // b2
                m_coeffs [ 3 ] = (2.0 * V0K2 - 2.0 ) * denominatorReciprocal; // a1
                m_coeffs [ 4 ] = ( 1 - root2rootV0K + V0K2 ) * denominatorReciprocal; // a2
            }
            else // boost
            {
                T denominatorReciprocal = 1.0 / ( 1.0 + root2K + K2 );
                m_coeffs [ 0 ] = ( m_V0 + root2rootV0K + K2 ) * denominatorReciprocal; // b0
                m_coeffs [ 1 ] = 2.0 * (K2 - m_V0 ) * denominatorReciprocal; // b1
                m_coeffs [ 2 ] = ( m_V0 - root2rootV0K + K2 ) * denominatorReciprocal; // b2
                m_coeffs [ 3 ] = ( 2.0 * K2 - 2.0 ) * denominatorReciprocal; // a1
                m_coeffs [ 4 ] = ( 1.0 - root2K + K2 ) * denominatorReciprocal; // a2
            }
        }
    }
    
    void bandpassCoefficients() // second order only
    {
        T K2 = m_K * m_K;
        T K2Q = K2 * m_Q;
        T denominatorReciprocal = 1.0 / ( K2Q + m_K + m_Q );
        m_coeffs [ 0 ] = m_K * denominatorReciprocal; // b0
        m_coeffs [ 1 ] = 0; // b1
        m_coeffs [ 2 ] = -1.0 * m_coeffs [ 0 ]; // b2
        m_coeffs [ 3 ] = 2.0 * m_Q * ( K2 - 1 ) * denominatorReciprocal; // a1
        m_coeffs [ 4 ] = ( K2Q - m_K + m_Q ) * denominatorReciprocal; // a2
    }
    
    
    void bandcutCoefficients() // second order only
    {
        T K2 = m_K * m_K;
        T K2Q = K2 * m_Q;
        T denominatorReciprocal = 1.0 / ( K2Q + m_K + m_Q );
        m_coeffs [ 0 ] = ( m_Q + K2Q ) * denominatorReciprocal; // b0
        m_coeffs [ 1 ] = 2.0 * ( K2Q - m_Q ) * denominatorReciprocal; // b0 // b1
        m_coeffs [ 2 ] = m_coeffs [ 0 ]; // b2
        m_coeffs [ 3 ] = m_coeffs [ 1 ]; // a1
        m_coeffs [ 4 ] = ( K2Q - m_K + m_Q ) * denominatorReciprocal; // a2
    }
    
    void peakCoefficients() // second order only
    {
        T K2 = m_K * m_K;
        T KoverQ = m_K / m_Q;
        
        if ( m_V0 < 1 ) // cut
        {
            T KoverV0Q = ( KoverQ / m_V0 );
            T denominatorReciprocal = 1.0 / ( 1.0 + KoverV0Q + K2 );
            m_coeffs [ 0 ] = ( 1.0 + KoverQ  + K2 ) * denominatorReciprocal; // b0
            m_coeffs [ 1 ] = 2.0 * ( K2 - 1.0 ) * denominatorReciprocal; // b0 // b1
            m_coeffs [ 2 ] = ( 1.0 - KoverQ  + K2 ) * denominatorReciprocal; // b2
            m_coeffs [ 3 ] = m_coeffs [ 1 ]; // a1
            m_coeffs [ 4 ] = ( 1.0 - KoverV0Q + K2 ) * denominatorReciprocal; // a2
        }
        else // boost
        {
            T denominatorReciprocal = 1.0 / ( 1.0 + KoverQ + K2 );
            T V0KoverQ = m_V0 * KoverQ;
            m_coeffs [ 0 ] = ( 1.0 + V0KoverQ  + K2 ) * denominatorReciprocal; // b0
            m_coeffs [ 1 ] = 2.0 * ( K2 - 1.0 ) * denominatorReciprocal; // b0 // b1
            m_coeffs [ 2 ] = ( 1.0 - V0KoverQ  + K2 ) * denominatorReciprocal; // b2
            m_coeffs [ 3 ] = m_coeffs [ 1 ]; // a1
            m_coeffs [ 4 ] = ( 1.0 - KoverQ + K2 ) * denominatorReciprocal; // a2
        }
    }
    
    
    // parameters
    T m_pi = atan(1)*4; // pi
    
    T m_f0 = 1000, m_Q = 1, m_SR = 44100, m_dBGain; // user variables
    T m_K, m_V0;
    T m_angFreqFactor; // other calculation Variables
    bool m_isFirstOrder = false;
    int m_type = 1;
    std::vector < T > m_coeffs;
    
    

    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_biquadCalculator )
};


#endif /* sjf_biquadCalculator_h */




