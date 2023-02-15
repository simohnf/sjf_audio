//
//  sjf_interpolators.h
//
//  Created by Simon Fay on 24/08/2022.
//

#ifndef sjf_interpolators_h
#define sjf_interpolators_h

#include <JuceHeader.h>
#include <vector>
#include "sjf_audioUtilities.h"


//==============================================================================
//==============================================================================
//==============================================================================
//                          VALUE BASED INTERPOLATIONS
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================

namespace sjf_interpolators
{
    //==============================================================================
    enum interpolatorTypes : char
    { linear = 1, cubic, pureData, fourthOrder, godot, hermite };
    //==============================================================================
    template <typename T>
    const T linearInterpolate ( const T &mu, const T &x1, const T &x2 )
    { return x1 + mu*(x2-x1); }
    //==============================================================================
    template <typename T>
    const T cubicInterpolate ( const T &mu, const T &x0, const T &x1, const T &x2, const T &x3 )
    {
        T a0,a1,a2,mu2;
        
        mu2 = mu*mu;
        a0 = x3 - x2 - x0 + x1;
        a1 = x0 - x1 - a0;
        a2 = x2 - x0;
        
        return (a0*mu*mu2 + a1*mu2 + a2*mu + x1);
    }
    //==============================================================================
    template <typename T>
    const T fourPointInterpolatePD ( const T &mu, const T &x0, const T &x1, const T &x2, const T &x3 )
    {
        auto x2minusx1 = x2-x1;
        return x1 + mu * (x2minusx1 - 0.1666667f * (1.0f - mu) * ( (x3 - x0 - 3.0f * x2minusx1) * mu + (x3 + 2.0f*x0 - 3.0f*x1) ) );
    }
    
    //==============================================================================
    template <typename T>
    const T fourPointFourthOrderOptimal  ( const T &mu, const T &x0, const T &x1, const T &x2, const T &x3 )
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
    template <typename T>
    const T cubicInterpolateGodot ( const T &mu, const T &x0, const T &x1, const T &x2, const T &x3 )
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
    template <typename T>
    const T cubicInterpolateHermite ( const T &mu, const T &x0, const T &x1, const T &x2, const T &x3 )
    {
        T a1,a2,a3;
        
        a1 = 0.5f * (x2 - x0);
        a2 = x0 - (2.5f * x1) + (2 * x2) - (0.5f * x3);
        a3 = (0.5f * (x3 - x0)) + (1.5f * (x1 - x2));
        return (((((a3 * mu) + a2) * mu) + a1) * mu) + x1;
    }
    
    //==============================================================================
    template <typename T>
    const T cubicInterpolateHermite2 ( const T &mu, const T &x0, const T &x1, const T &x2, const T &x3 )
    {
        T diff = x1 - x2;
        T a1 = x2 - x0;
        T a3 = x3 - x0 + 3.0f * diff;
        T a2 = -(2 * diff + a1 + a3);
        return 0.5f * ((a3 * mu + a2) * mu + a1) * mu + x1;
    }
    //==============================================================================
}


#endif /* sjf_interpolators_h */

