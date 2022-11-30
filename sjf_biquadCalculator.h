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
        m_angFreqFactor = 2 * m_pi / m_SR;
        DBG( "m_SR " << m_SR );
        DBG( "m_pi " << m_pi );
        DBG("m_angFreqFactor " << m_angFreqFactor);
        
    }
    
    void setFrequency( T fr )
    {
        m_f0 = fr;
        calculateIntermediateValues();
    }
    
    void setQFactor( T Q )
    {
        m_Q = Q;
        calculateIntermediateValues();
    }
    
    void setdBGain( T gain )
    {
        m_dBGain = gain;
        calculateIntermediateValues();
    }
    
    std::vector< T > getCoefficients()
    {
        switch (m_type)
        {
            case lowpass:
                if ( m_isFirstOrder )
                {
                    lowpassCoefficients1st();
                }
                else
                {
                    lowpassCoefficients();
                }
                break;
            case highpass:
                if ( m_isFirstOrder )
                {
                    highpassCoefficients1st();
                }
                else
                {
                    highpassCoefficients();
                }
                break;
            case lowshelf:
                
                break;
            case highshelf:
                
                break;
            case bandpass:
                
                break;
            case bandnotch:
                
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
        lowpass = 1, highpass, lowshelf, highshelf, bandpass, bandnotch
    };
    
private:
    void calculateIntermediateValues()
    {
        m_W0 = m_f0 * m_angFreqFactor;
        DBG( "m_W0 " << m_W0 );
        m_cosW0 = cos( m_W0 );
        DBG( "m_cosW0 " << m_cosW0 );
        m_sinW0 = sin( m_W0 );
        DBG( "m_sinW0 " << m_sinW0 );
        m_alpha = m_sinW0 / ( 2 * m_Q );
    }
    
    // 2nd order lowpass
    void lowpassCoefficients()
    {
        T inverseA0 = 1 / ( 1 + m_alpha ); // normalise to get only 5 variables
        
        m_coeffs [ 0 ] = ( 1 - m_cosW0 ) * 0.5 * inverseA0; // b0
        m_coeffs [ 1 ] = ( 1 - m_cosW0 ) * inverseA0; // b1
        m_coeffs [ 2 ] = m_coeffs [ 0 ]; // b2
        m_coeffs [ 3 ] = -2 * (m_cosW0 * inverseA0); // a1 ---> precision seems to be lost by multiply by 2???
        m_coeffs [ 4 ] = ( 1 - m_alpha ) * inverseA0; // a2
    }
    
    // 1st order lowpass from https://www.proquest.com/openview/57ded8cf431b6f1c50a2c9e10742a5ad/1?pq-origsite=gscholar&cbl=2026666
    void lowpassCoefficients1st()
    {
        T twoTanW0 = 2 * m_sinW0 / ( 1 + m_cosW0 ); // 2 * tan (W0 / 2)
        T invTwoTanW0Plus2 = 1 / ( 2 + twoTanW0 ); // 1 / (2 + 2tanW0)
        m_coeffs [ 0 ] = twoTanW0  * invTwoTanW0Plus2; // b0
        m_coeffs [ 1 ] = m_coeffs [ 0 ]; // b1
        m_coeffs [ 2 ] =  0; // b2
        m_coeffs [ 3 ] = ( twoTanW0 - 2 ) * invTwoTanW0Plus2; // a1
        m_coeffs [ 4 ] = 0; // a2
    }
    
    // 2nd order highpass
    void highpassCoefficients()
    {
        T inverseA0 = 1 / ( 1 + m_alpha );
        
        m_coeffs [ 0 ] = ( 1 + m_cosW0 ) * 0.5 * inverseA0; // b0
        m_coeffs [ 1 ] = -1 * ( 1 + m_cosW0 ) * inverseA0; // b1
        m_coeffs [ 2 ] = m_coeffs [ 0 ]; // b2
        m_coeffs [ 3 ] = -2 * m_cosW0 * inverseA0; // a1
        m_coeffs [ 4 ] = ( 1 - m_alpha ) * inverseA0; // a2
    }
    
    // 1st order highpass from https://www.proquest.com/openview/57ded8cf431b6f1c50a2c9e10742a5ad/1?pq-origsite=gscholar&cbl=2026666
    void highpassCoefficients1st()
    {
        T twoTanW0 = 2 * m_sinW0 / ( 1 + m_cosW0 ) ; // 2 * tan W0 --> trig identity for tan(2Ø)
        T invTwoTanW0Plus2 = 1 / ( 2 + twoTanW0 ); // 1 / (2 + 2tanW0)
        m_coeffs [ 0 ] = 2 * invTwoTanW0Plus2; // b0
        m_coeffs [ 1 ] = -1 * m_coeffs [ 0 ]; // b1
        m_coeffs [ 2 ] =  0; // b2
        m_coeffs [ 3 ] = ( twoTanW0 - 2 ) * invTwoTanW0Plus2; // a1
        m_coeffs [ 4 ] = 0; // a2
    }
    
    
    
    // parameters
    T m_pi = atan(1)*4; // pi
    
    T m_f0 = 1000, m_Q = 1, m_SR = 44100, m_dBGain; // user variables
    T m_A, m_W0, m_cosW0, m_sinW0, m_alpha; // intermediate variables
    T m_angFreqFactor; // other calculation Variables
    bool m_isFirstOrder = false;
    int m_type = 1;
    std::vector < T > m_coeffs;
    
    

    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_biquadCalculator )
};


#endif /* sjf_biquadCalculator_h */




