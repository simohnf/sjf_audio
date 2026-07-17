//
//  sjf_damper.h
//
//  Created by Simon Fay on 27/03/2024.
//

#ifndef sjf_damper_h
#define sjf_damper_h

namespace sjf::filters
{
    /**
     basic one pole lowpass/highpass filter, but set so that the higher the coefficient the lower the cut off frequency, this just makes it useful for setting damping in a reverb loop
        this class is set so that higher coefficients equate to lower cutoff frequencies!
     */
    template < typename Sample >
    class damper
    {
    private:
        Sample m_lastOut = 0;
    public:
        damper(){}
        ~damper(){}
        
        /**
         this should be called every sample in the block,
         The input is:
            the sample to process
            the damping coefficient ( must be >=0 and <=1 )
        Output:
                 result  of low pass
         */
        Sample process( Sample x, Sample coef )
        {
            assert ( coef >= 0 && coef <= 1 );
            m_lastOut = x + coef*( m_lastOut - x );
            return m_lastOut;
        }
        
        /**
         this should be called every sample in the block,
         The input is:
            the sample to process
            the damping coefficient ( must be >=0 and <=1 )
         Output:
                  result  of high pass
         */
        Sample processHP( Sample x, Sample coef )
        {
            assert ( coef >= 0 && coef <= 1 );
            m_lastOut = x + coef*( m_lastOut - x );
            return (x - m_lastOut);
        }
        
        /**
         Reset the currently stored value. Can be used to clear delay line to 0 or to set to an initial value
         */
        void reset( Sample val = 0 ) { m_lastOut = val; }
    };
}


#endif /* sjf_damper_h */
