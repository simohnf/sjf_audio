//
//  sjf_reverseDelay.h
//  sjf_verb
//
//  Created by Simon Fay on 10/05/2024.
//  Copyright © 2024 sjf. All rights reserved.
//

#ifndef sjf_reverseDelay_h
#define sjf_reverseDelay_h

#include "sjf_delay.h"

namespace sjf::delayLine
{
    template < typename Sample, interpolation::interpolatorTypes interpType /*, typename INTERPOLATION_FUNCTOR = interpolation::fourPointInterpolatePD< Sample >*/ >
    class reverseDelay
    {
    public:
        reverseDelay(){ initialise( 44100 ); }
        
        reverseDelay( reverseDelay&&) noexcept = default;
        reverseDelay& operator=( reverseDelay&&) noexcept = default;
        
        ~reverseDelay(){}
        
        
        /** Call this before using to initialise the delay lines */
        void initialise( int maxDelayInSamps )
        {
            m_delay.initialise( maxDelayInSamps );
            maxDT = sjf_nearestPowerAbove(maxDelayInSamps, 2);
        }
        
        /** Call this before using to initialise the delay lines */
        void initialise( int maxDelayInSamps, Sample rampLengthSamps )
        {
            m_delay.initialise( maxDelayInSamps );
            maxDT = sjf_nearestPowerAbove(maxDelayInSamps, 2);
            setRampLength( rampLengthSamps );
        }
        
        
        /** This sets the length of the fade in/out when reversed */
        void setRampLength( Sample rampLengthSamps )
        {
            if ( m_rampLen == rampLengthSamps ){ return; }
            m_rampLen = rampLengthSamps;
            m_nRampSegs = m_dtSamps/m_rampLen;
        }
        
        /** Set the delaytime and related calculations */
        void setDelayTime( Sample delayInSamps )
        {
            delayInSamps = delayInSamps > 1 ? delayInSamps : 1;
            if( m_dtSamps == delayInSamps*2 ){ return; } // Save a little CPU
            m_dtSamps = delayInSamps*2;
            m_invDT = 1.0/m_dtSamps;
            m_nRampSegs = m_dtSamps/m_rampLen;
        }
        
        /**
         This sets the value of the sample at the current write position and automatically updates the write pointer
         */
        void setSample( Sample x ) { m_delay.setSample( x ); }
        /**
         This retrieves a sample from a previous point in the buffer
         Input is:
            the number of samples in the past to read from
         */
        Sample getSample( ) const { return m_delay.getSample( m_dtSamps ); }
        
        /**
         This retrieves a sample from a previous point in the buffer put reads the buffer in reverse
         Input is:
            the number of samples in the past to read from
         */
        Sample getSampleReversed( )
        {
            Sample outSamp = m_delay.getSample( m_revCount + 1 ) * sjf::utilities::phaseEnvelope( (m_revCount * m_invDT), m_nRampSegs );
            m_revCount += 2;
            m_revCount = m_revCount < m_dtSamps ? m_revCount : 0;
            return outSamp;
        }
        
    private:
        
        
        delay< Sample, interpType > m_delay;
        Sample m_SR{44100}, m_rampLen{45}, m_dtSamps{4410}, m_invDT{ 1/m_dtSamps}, m_nRampSegs{m_dtSamps/m_rampLen}, m_revCount{0};
        int maxDT{ sjf_nearestPowerAbove(static_cast<int>(m_SR), 2) };
    };
}
#endif /* sjf_reverseDelay_h */
