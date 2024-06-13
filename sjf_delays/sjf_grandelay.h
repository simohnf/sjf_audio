//
//  sjf_grandelay.h
//
//  Created by Simon Fay on 28/05/2024.
//

#ifndef sjf_gDelay_h
#define sjf_gDelay_h

#include "../sjf_table.h"
#include "../sjf_interpolators/sjf_interpolator.h"
#include "../sjf_oscillators/sjf_phasor.h"

namespace sjf::delayLine
{
    /** Simple circular buffer voice allowing changing delay times through granulation rather than interpolation */
    template < typename Sample >
    class gDelay
    {
    public:
        gDelay(){}
        ~gDelay(){}

        /**
         This must be called before first use in order to set basic information such as maximum delay lengths and sample rate
         Size should be a power of 2
         */
        void initialise( Sample sampleRate, int sizeInSamps_pow2 )
        {
            m_SR = sampleRate > 0 ? sampleRate : m_SR;
            if (!sjf_isPowerOf( sizeInSamps_pow2, 2 ) )
                sizeInSamps_pow2 = sjf_nearestPowerAbove( sizeInSamps_pow2, 2 );
            m_buffer.resize( sizeInSamps_pow2, 0 );
            m_wrapMask = sizeInSamps_pow2 - 1;
            clear();
        }
        /** Set the rate at which grains are created */
        void setGrainRate( Sample rateHz )
        {
            m_grainRate = rateHz > 0 ? rateHz : rateHz < 0 ? -rateHz : m_grainRate;
            m_phasor.setFrequency( m_grainRate, m_SR );
        }
        
        
        /**
         This sets the value of the sample at the current write position and automatically updates the write pointer
         */
        void setSample( Sample x )
        {
            m_buffer[ m_writePos ] = x;
            m_writePos += 1;
            m_writePos &= m_wrapMask;
        }


        /**
         This retrieves a sample from a previous point in the buffer
         Input is:
            the number of samples in the past to read from
         */
        inline Sample getSample( Sample delay ){ return m_interp( m_buffer.data(), m_wrapMask, m_writePos-delay ); }



        void clear() { std::fill( m_buffer.begin(), m_buffer.end(), 0 ); }
    private:
        Sample m_SR{44100}, m_grainRate{1};
        std::vector< Sample > m_buffer;
        int m_writePos = 0, m_wrapMask;
        
        
        struct windowfunc{ const Sample operator()( Sample findex ) const { return 0.5 + 0.5*gcem::cos<Sample>(findex*2.0*M_PI); } };
        static constexpr size_t TABLE_SIZE{2048};
        wavetable::tab< Sample, TABLE_SIZE, windowfunc, interpolation::interpolatorTypes::linear > m_window;
        oscillators::phasor<Sample> m_phasor{ 1, m_SR };
    };
}

#endif /* sjf_gDelay_h */
