//
//  sjf_delay.h
//
//  Created by Simon Fay on 27/03/2024.
//

#ifndef sjf_delay_h
#define sjf_delay_h

//#include "../sjf_audioUtilitiesC++.h"
//#include "../sjf_interpolators.h"
//#include "../gcem/include/gcem.hpp"
//
//#include "sjf_rev_consts.h"

#include "../sjf_filters.h"
#include "../sjf_interpolators/sjf_interpolator.h"
namespace sjf::delayLine
{
    /**
     basic circular buffer based delay line
     */
    template < typename Sample >
    class delay
    {
    public:
        delay()
        {
            initialise( 4096 ); // ensure this are initialised to something
        }
        ~delay(){}
        
        
        delay( delay&&) noexcept = default;
        delay& operator=( delay&&) noexcept = default;
        
        /**
         This must be called before first use in order to set basic information such as maximum delay lengths and sample rate
         Size should be a power of 2
         */
        void initialise( int sizeInSamps_pow2 )
        {
            if (!sjf_isPowerOf( sizeInSamps_pow2, 2 ) )
                sizeInSamps_pow2 = sjf_nearestPowerAbove( sizeInSamps_pow2, 2 );
            m_buffer.resize( sizeInSamps_pow2, 0 );
            m_wrapMask = sizeInSamps_pow2 - 1;
        }
        
        /**
         This retrieves a sample from a previous point in the buffer
         Input is:
            the number of samples in the past to read from
         */
//        utilities::classMemberFunctionPointer< delay, Sample, Sample > getSample{ this, &delay::linInterp };
        inline Sample getSample( Sample delay )
        {
            return m_interpolator( m_buffer.data(), m_buffer.size(), m_writePos-delay );
        }
        
//            return m_interpolate( m_writePos-delay );
//            auto rp = getPosition( ( m_writePos - delay ) );
//            switch (m_interType) {
//                case sjf_interpolators::interpolatorTypes::none:
//                    return noInterp( rp );
//                case sjf_interpolators::interpolatorTypes::linear :
//                    return linInterp( rp );
//                case sjf_interpolators::interpolatorTypes::cubic :
//                    return cubicInterp( rp );
//                case sjf_interpolators::interpolatorTypes::pureData :
//                    return pdInterp( rp );
//                case sjf_interpolators::interpolatorTypes::fourthOrder :
//                    return fourPointInterp( rp );
//                case sjf_interpolators::interpolatorTypes::godot :
//                    return godotInterp( rp );
//                case sjf_interpolators::interpolatorTypes::hermite :
//                    return hermiteInterp( rp );
//                default:
//                    return linInterp( rp );
//            };
//        }
        
        /**
         This sets the value of the sample at the current write position and automatically updates the write pointer
         */
        void setSample( Sample x )
        {
            m_buffer[ m_writePos ] = x;
            m_writePos += 1;
            m_writePos &= m_wrapMask;
        }
        
        /** Set the interpolation Type to be used, the interpolation type see @sjf_interpolators */
        void setInterpolationType( interpolation::interpolatorTypes interpType ) { m_interpolator.setInterpolationType( interpType ); }
        
    private:
        inline Sample noInterp( Sample findex )
        {
            findex = static_cast<int>(m_writePos - findex);
            return m_buffer[ findex ];
        }
        
        inline Sample linInterp( Sample findex )
        {
            findex = m_writePos - findex;
            Sample x1, x2, mu;
            auto ind1 = static_cast< long >( findex );
            mu = findex - ind1;
            x1 = m_buffer[ ind1 & m_wrapMask ];
            x2 = m_buffer[ ( (ind1+1) & m_wrapMask ) ];
            
            return sjf_interpolators::linearInterpolate( mu, x1, x2 );
        }
        
        inline Sample cubicInterp( Sample findex )
        {
            findex = m_writePos - findex;
            Sample x0, x1, x2, x3, mu;
            auto ind1 = static_cast< long >( findex );
            mu = findex - ind1;
            x0 = m_buffer[ ( (ind1-1) & m_wrapMask ) ];
            x1 = m_buffer[ ind1 & m_wrapMask ];
            x2 = m_buffer[ ( (ind1+1) & m_wrapMask ) ];
            x3 = m_buffer[ ( (ind1+2) & m_wrapMask ) ];
            return sjf_interpolators::cubicInterpolate( mu, x0, x1, x2, x3 );
        }
        
        inline Sample pdInterp( Sample findex )
        {
            findex = m_writePos - findex;
            Sample x0, x1, x2, x3, mu;
            auto ind1 = static_cast< long >( findex );
            mu = findex - ind1;
            x0 = m_buffer[ ( (ind1-1) & m_wrapMask ) ];
            x1 = m_buffer[ ind1 & m_wrapMask ];
            x2 = m_buffer[ ( (ind1+1) & m_wrapMask ) ];
            x3 = m_buffer[ ( (ind1+2) & m_wrapMask ) ];
            return sjf_interpolators::fourPointInterpolatePD( mu, x0, x1, x2, x3 );
        }

        
        inline Sample fourPointInterp( Sample findex )
        {
            findex = m_writePos - findex;
            Sample x0, x1, x2, x3, mu;
            auto ind1 = static_cast< long >( findex );
            mu = findex - ind1;
            x0 = m_buffer[ ( (ind1-1) & m_wrapMask ) ];
            x1 = m_buffer[ ind1 & m_wrapMask ];
            x2 = m_buffer[ ( (ind1+1) & m_wrapMask ) ];
            x3 = m_buffer[ ( (ind1+2) & m_wrapMask ) ];
            return sjf_interpolators::fourPointFourthOrderOptimal( mu, x0, x1, x2, x3 );
        }
        
        inline Sample godotInterp( Sample findex )
        {
            findex = m_writePos - findex;
            Sample x0, x1, x2, x3, mu;
            auto ind1 = static_cast< long >( findex );
            mu = findex - ind1;
            x0 = m_buffer[ ( (ind1-1) & m_wrapMask ) ];
            x1 = m_buffer[ ind1 & m_wrapMask];
            x2 = m_buffer[ ( (ind1+1) & m_wrapMask ) ];
            x3 = m_buffer[ ( (ind1+2) & m_wrapMask ) ];
            return sjf_interpolators::cubicInterpolateGodot( mu, x0, x1, x2, x3 );
        }
        
        inline Sample hermiteInterp( Sample findex )
        {
            findex = m_writePos - findex;
            Sample x0, x1, x2, x3, mu;
            auto ind1 = static_cast< long >( findex );
            mu = findex - ind1;
            x0 = m_buffer[ ( (ind1-1) & m_wrapMask ) ];
            x1 = m_buffer[ ind1 & m_wrapMask];
            x2 = m_buffer[ ( (ind1+1) & m_wrapMask ) ];
            x3 = m_buffer[ ( (ind1+2) & m_wrapMask ) ];
            return sjf_interpolators::cubicInterpolateHermite( mu, x0, x1, x2, x3 );
        }

        Sample getPosition( Sample pos )
        {
            int p = pos;
            Sample mu = pos - p;
            p &= m_wrapMask;
            return ( static_cast< Sample >( p ) + mu );
        }
        
        
    private:
        std::vector< Sample > m_buffer;
        int m_writePos = 0, m_wrapMask;
        sjf::interpolation::interpolator<Sample> m_interpolator;
        
    };
}

#endif /* sjf_delay_h */
