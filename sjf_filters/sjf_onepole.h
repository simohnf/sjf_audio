//
//  sjf_onepole.h
//  sjf_verb
//
//  Created by Simon Fay on 13/05/2024.
//  Copyright © 2024 sjf. All rights reserved.
//

#ifndef sjf_onepole_h
#define sjf_onepole_h
namespace sjf::filters
{
    /**
     basic one pole lowpass/highpass filter, but set so that the higher the coefficient the lower the cut off frequency, this just makes it useful for setting damping in a reverb loop
        this class is set so that higher coefficients equate to higher cutoff frequencies!
     */
    template < typename Sample >
    class onepole
    {
    private:
        Sample m_lastOut = 0;
    public:
        onepole(){}
        ~onepole(){}
        
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
            m_lastOut += coef*( x - m_lastOut );
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
        Sample processHP( Sample x, Sample coef ) { return x - process( x, coef ); }
        
        /**
         Reset the currently stored value. Can be used to clear delay line to 0 or to set to an initial value
         */
        void reset( Sample val = 0 ) { m_lastOut = val; }
    };
}

#endif /* sjf_onepole_h */
