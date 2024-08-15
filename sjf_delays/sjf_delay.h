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
    template < typename Sample, interpolation::interpolatorTypes interpType >
    class delay
    {
    public:
        delay(){}
        delay( std::vector< Sample >& buffer ) : m_buffer(buffer), m_wrapMask(buffer.size()-1) {}
        
        ~delay(){}
        
        /**
         This must be called before first use in order to set basic information such as maximum delay lengths and sample rate
         Size should be a power of 2. You shouldn't have to call this when passing a preexisting buffer to the constructor
         */
        void initialise( long sizeInSamps_pow2 )
        {
            if (!sjf_isPowerOf( sizeInSamps_pow2, 2 ) )
                sizeInSamps_pow2 = sjf_nearestPowerAbove( sizeInSamps_pow2, 2l );
            m_buffer.resize( sizeInSamps_pow2, 0 );
            m_wrapMask = sizeInSamps_pow2 - 1;
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
            m_writePos &= m_wrapMask;
            m_buffer[ m_writePos ] = x;
            ++m_writePos;
        }
        
        /** Clear the buffer */
        void clear() { std::fill( m_buffer.begin(), m_buffer.end(), 0 ); }
        
    private:
        std::vector< Sample > m_buffer;
        long  m_writePos = 0, m_wrapMask;
        interpolation::interpolator<Sample, interpType> m_interp;
        
    };

//==================//==================//==================//==================//==================//==================
//==================//==================//==================//==================//==================//==================
//==================//==================//==================//==================//==================//==================
//==================//==================//==================//==================//==================//==================

    template < typename Sample, interpolation::interpolatorTypes interpType = interpolation::interpolatorTypes::pureData >
    class multiChannelDelay
    {
    public:
        multiChannelDelay( const size_t nChannels ) : NCHANNELS( nChannels ) { }
        multiChannelDelay( const size_t nChannels, std::vector< Sample >& buffer ) : NCHANNELS( nChannels ), m_buffer(buffer), m_channelOffset(m_buffer.size()/NCHANNELS), m_wrapMask(m_channelOffset-1) { }
        ~multiChannelDelay(){}
        
        /**
         This must be called before first use in order to set basic information such as maximum delay lengths and sample rate
         Size should be a power of 2. You shouldn't have to call this when passing a preexisting buffer to the constructor
         */
        void initialise( long sizeInSamps_pow2 )
        {
            if (!sjf_isPowerOf( sizeInSamps_pow2, 2 ) )
                sizeInSamps_pow2 = sjf_nearestPowerAbove( sizeInSamps_pow2, 2l );
            m_channelOffset = sizeInSamps_pow2;
            assert( m_channelOffset * NCHANNELS < m_buffer.max_size() );
            m_buffer.resize( m_channelOffset * NCHANNELS, 0 );
            m_wrapMask = m_channelOffset - 1;
        }
        
        /**
         This retrieves a sample from a previous point in the buffer
         Input is:
            the channel to read
            the number of samples in the past to read from
         */
        inline Sample getSample( size_t channel, Sample delay ) const
        {
            return m_interp( m_buffer.data() + m_channelOffset*channel, m_wrapMask, m_writePos-delay );
        }
        
        /**
         This sets the value of the sample at the current write position and automatically updates the write pointer
         */
        void setSample( size_t channel, Sample x )
        {
            assert( channel < NCHANNELS );
            m_buffer[ m_channelOffset*channel + m_writePos ] = x;
        }
        
        void updateWritePos()
        {
            ++m_writePos;
            m_writePos &= m_wrapMask;
        }
        
        /** Clear the buffer */
        void clear() { std::fill( m_buffer.begin(), m_buffer.end(), 0 ); }
        
    private:
        const size_t NCHANNELS;
        std::vector< Sample > m_buffer;
        long  m_writePos{0}, m_wrapMask{0}, m_channelOffset{0};
        interpolation::interpolator<Sample, interpType> m_interp;
    };
}

#endif /* sjf_delay_h */
