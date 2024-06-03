//
//  sjf_delay.h
//
//  Created by Simon Fay on 27/03/2024.
//

#ifndef sjf_delay_h
#define sjf_delay_h

#include "../sjf_interpolators/sjf_interpolator.h"


namespace sjf::delayLine
{
    /**
     basic circular buffer based delay line
     */
    template < typename Sample, typename INTERPOLATION_FUNCTOR = interpolation::fourPointInterpolatePD<Sample> >
    class delay
    {
    public:
        delay() {}
        ~delay(){}
        
        /**
         This must be called before first use in order to set basic information such as maximum delay lengths and sample rate
         Size should be a power of 2
         */
        void initialise( long sizeInSamps_pow2 )
        {
            if (!sjf_isPowerOf( sizeInSamps_pow2, 2 ) )
                sizeInSamps_pow2 = sjf_nearestPowerAbove( sizeInSamps_pow2, 2l );
            m_buffer.resize( sizeInSamps_pow2, 0 );
            m_wrapMask = sizeInSamps_pow2 - 1;
            clear();
        }
        
        /**
         This retrieves a sample from a previous point in the buffer
         Input is:
            the number of samples in the past to read from
         */
        inline Sample getSample( Sample delay ) const { return m_interp( m_buffer.data(), m_wrapMask, m_writePos-delay ); }
        
        /**
         This sets the value of the sample at the current write position and automatically updates the write pointer
         */
        void setSample( Sample x )
        {
            m_buffer[ m_writePos ] = x;
            m_writePos += 1;
            m_writePos &= m_wrapMask;
        }
        
        /** Clear the buffer */
        void clear() { std::fill( m_buffer.begin(), m_buffer.end(), 0 ); }
        
    private:
        std::vector< Sample > m_buffer;
        long  m_writePos = 0, m_wrapMask;
        const INTERPOLATION_FUNCTOR m_interp;
        
    };
}

#endif /* sjf_delay_h */
