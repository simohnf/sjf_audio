//
//  sjf_interpolators.h
//
//  Created by Simon Fay on 24/08/2022.
//

#ifndef sjf_interpolators_h
#define sjf_interpolators_h
#include "sjf_audioUtilitiesC++.h"
//#include <immintrin.h>

//==============================================================================
//==============================================================================
//==============================================================================
//                          VALUE BASED INTERPOLATIONS
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================

namespace sjf::interpolation
{
    //==============================================================================
    enum interpolatorTypes : char
    { none = 0, linear = 1, cubic, pureData, fourthOrder, godot, hermite, allpass };
    //==============================================================================
    /**
     Basic linear interpolation between 2 points
     mu - distance between points one and two ( must be between 0 --> 1 )
     x1 - value of first point
     x2 - value of second point
     */
    template <typename T>
    const T linearInterpolate ( const T mu, const T x1, const T x2 )
    { return x1 + mu*(x2-x1); }
    //==============================================================================
    /**
     Basic cubic interpolation using 4 points
     mu - distance between points one and two ( must be between 0 --> 1 )
     x0 - value of previous point
     x1 - value of first point
     x2 - value of second point
     x3 - value of point after next
     */
    template <typename T>
    const T cubicInterpolate ( const T mu, const T x0, const T x1, const T x2, const T x3 )
    {
        T a0,a1,a2,mu2;
        
        mu2 = mu*mu;
        a0 = x3 - x2 - x0 + x1;
        a1 = x0 - x1 - a0;
        a2 = x2 - x0;
        
        return (a0*mu*mu2 + a1*mu2 + a2*mu + x1);
    }
    //==============================================================================
    /**
    Four point interpolation copied from PureData source code
     mu - distance between points one and two ( must be between 0 --> 1 )
     x0 - value of previous point
     x1 - value of first point
     x2 - value of second point
     x3 - value of point after next
     */
    template <typename T>
    const T fourPointInterpolatePD ( const T mu, const T x0, const T x1, const T x2, const T x3 )
    {
        auto x2minusx1 = x2-x1;
        return x1 + mu * (x2minusx1 - 0.1666667f * (1.0f - mu) * ( (x3 - x0 - 3.0f * x2minusx1) * mu + (x3 + 2.0f*x0 - 3.0f*x1) ) );
    }
    
    //==============================================================================
    /**
     Four point fourth order interpolation copied from Olli Niemitalo - Polynomial Interpolators for High-Quality Resampling of Oversampled Audio
     mu - distance between points one and two ( must be between 0 --> 1 )
     x0 - value of previous point
     x1 - value of first point
     x2 - value of second point
     x3 - value of point after next
     */
    template <typename T>
    const T fourPointFourthOrderOptimal  ( const T mu, const T x0, const T x1, const T x2, const T x3 )
    {
        //    Copied from Olli Niemitalo - Polynomial Interpolators for High-Quality Resampling of Oversampled Audio
        // Optimal 2x (4-point, 4th-order) (z-form)
        T z = mu - 0.5f;
        T even1 = x2+x1, odd1 = x2-x1;
        T even2 = x3+x0, odd2 = x3-x0;
        T c0 = even1*0.45645918406487612 + even2*0.04354173901996461;
        T c1 = odd1*0.47236675362442071 + odd2*0.17686613581136501;
        T c2 = even1*-0.253674794204558521 + even2*0.25371918651882464;
        T c3 = odd1*-0.37917091811631082 + odd2*0.11952965967158000;
        T c4 = even1*0.04252164479749607 + even2*-0.04289144034653719;
        return (((c4*z+c3)*z+c2)*z+c1)*z+c0;
    }
    
    //==============================================================================
    /**
     Four point interpolation copied from from Godot https://stackoverflow.com/questions/1125666/how-do-you-do-bicubic-or-other-non-linear-interpolation-of-re-sampled-audio-da
     mu - distance between points one and two ( must be between 0 --> 1 )
     x0 - value of previous point
     x1 - value of first point
     x2 - value of second point
     x3 - value of point after next
     */
    template <typename T>
    const T cubicInterpolateGodot ( const T mu, const T x0, const T x1, const T x2, const T x3 )
    {
        //  Godot https://stackoverflow.com/questions/1125666/how-do-you-do-bicubic-or-other-non-linear-interpolation-of-re-sampled-audio-da
        T a0,a1,a2,a3,mu2;
        mu2 = mu*mu;
        
        a0 = 3 * x1 - 3 * x2 + x3 - x0;
        a1 = 2 * x0 - 5 * x1 + 4 * x2 - x3;
        a2 = x2 - x0;
        a3 = 2 * x1;
        
        return (a0 * mu * mu2 + a1 * mu2 + a2 * mu + a3) * 0.5f;
    }
    //==============================================================================
    /**
     Four point Hermite interpolation copied from from Godot https://stackoverflow.com/questions/1125666/how-do-you-do-bicubic-or-other-non-linear-interpolation-of-re-sampled-audio-da
     mu - distance between points one and two ( must be between 0 --> 1 )
     x0 - value of previous point
     x1 - value of first point
     x2 - value of second point
     x3 - value of point after next
     */
    template <typename T>
    const T cubicInterpolateHermite ( const T mu, const T x0, const T x1, const T x2, const T x3 )
    {
        T a1,a2,a3;
        
        a1 = 0.5f * (x2 - x0);
        a2 = x0 - (2.5f * x1) + (2 * x2) - (0.5f * x3);
        a3 = (0.5f * (x3 - x0)) + (1.5f * (x1 - x2));
        return (((((a3 * mu) + a2) * mu) + a1) * mu) + x1;
    }
    
    //==============================================================================
    /**
     Four point Hermite interpolation copied from from Godot https://stackoverflow.com/questions/1125666/how-do-you-do-bicubic-or-other-non-linear-interpolation-of-re-sampled-audio-da
     mu - distance between points one and two ( must be between 0 --> 1 )
     x0 - value of previous point
     x1 - value of first point
     x2 - value of second point
     x3 - value of point after next
     */
    template <typename T>
    const T cubicInterpolateHermite2 ( const T mu, const T x0, const T x1, const T x2, const T x3 )
    {
        T diff = x1 - x2;
        T a1 = x2 - x0;
        T a3 = x3 - x0 + 3.0f * diff;
        T a2 = -(2 * diff + a1 + a3);
        return 0.5f * ((a3 * mu + a2) * mu + a1) * mu + x1;
    }
    //==============================================================================
    /**
     Allpass filter based interpolator copied from https://ccrma.stanford.edu/~jos/pasp/First_Order_Allpass_Interpolation.html
     Not suited for random access interpolation!!!
     */
    template <typename T>
    class sjf_allpassInterpolator
    {
    public:
        sjf_allpassInterpolator(){}
        ~sjf_allpassInterpolator(){}
        
        /**
         This can be used to caluclate with arbitrary/changing mu values without having to keep reseting stored Mu
         */
        T process( T x, T mu )
        {
            auto n = (1.0-mu) / (1.0+mu);
            m_y1 = n*(x - m_y1) + m_x1;
            m_x1 = x;
            return m_y1;
        }
        
        /**
         This can be used to caluclate with internally stored Mu value
         */
        T process ( T x )
        {
            m_y1 = m_n*(x - m_y1) + m_x1;
            m_x1 = x;
            return m_y1;
        }
        
        /**
         This allows you to set the amount of intersample delay
         */
        void setMu( T mu )
        {
            m_n = (1.0-mu) / (1.0+mu);
        }
        
        /**
         This returns the currently stored coefficient
         */
        T getCoef( )
        {
            return m_n;
        }
        
        /**
         this resets the internally stored values
         */
        void reset()
        {
            m_x1 = m_y1 = 0;
        }
        
        /**
         Calculate the coefficient for a given intersample delay amount
         */
        static T calculateCoef( T mu )
        {
            return (1.0-mu) / (1.0+mu);
        }
    private:
        T m_x1 = 0, m_y1 = 0, m_n = 0;
    };

template< typename Sample >
struct interpolator
{
    inline Sample operator()( Sample* samps, long arraySize, Sample findex )
    {
        assert( sjf_isPowerOf( arraySize, 2 ) ); // arraySize must be a power of 2 for the wrap mask to work!!!
        const auto wrapMask = arraySize - 1;
        Sample x0, x1, x2, x3, mu;
        auto ind1 = static_cast< long >( findex );
        mu = findex - ind1;
        x0 = samps[ ( (ind1-1) & wrapMask ) ];
        x1 = samps[ ind1 & wrapMask ];
        x2 = samps[ ( (ind1+1) & wrapMask ) ];
        x3 = samps[ ( (ind1+2) & wrapMask ) ];
        switch ( m_interp ) {
            case interpolatorTypes::none:
                return samps[ ind1 ];
            case interpolatorTypes::linear :
                return linearInterpolate( mu, x1, x2 );
            case interpolatorTypes::cubic :
                return cubicInterpolate( mu, x0, x1, x2, x3 );
            case interpolatorTypes::pureData :
                return fourPointInterpolatePD( mu, x0, x1, x2, x3 );
            case interpolatorTypes::fourthOrder :
                return fourPointFourthOrderOptimal( mu, x0, x1, x2, x3 );
            case interpolatorTypes::godot :
                return cubicInterpolateGodot( mu, x0, x1, x2, x3 );
            case interpolatorTypes::hermite :
                return cubicInterpolateHermite( mu, x0, x1, x2, x3 );
            default:
                return linearInterpolate( mu, x1, x2 );
        }
        
    }
    void setInterpolationType( interpolatorTypes interpType ) { m_interp = interpType;    }
private:
    interpolatorTypes m_interp{ interpolatorTypes::linear };
};




}


#endif /* sjf_interpolators_h */

