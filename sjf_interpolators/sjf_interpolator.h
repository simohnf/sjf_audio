//
//  sjf_interpolator.h
//  sjf_verb
//
//  Created by Simon Fay on 15/05/2024.
//  Copyright © 2024 sjf. All rights reserved.
//

#ifndef sjf_interpolator_h
#define sjf_interpolator_h
#include "../sjf_audioUtilitiesC++.h"
namespace sjf::interpolation
{
    enum class interpolatorTypes { none, linear, cubic, pureData, fourthOrder, godot, hermite };
    
    template< typename Sample >
    struct interpVals { Sample mu, x0, x1, x2, x3; };
    
    template< typename Sample >
    inline interpVals<Sample> calculateVals( const Sample* samps, const long& wrapMask, const Sample& findex )
    {
#ifndef DEBUG
        assert( sjf_isPowerOf( wrapMask+1, 2 ) );
#endif
        auto ind1 = static_cast< long >( findex );
        auto mu = findex < 0 ? ( 1 + (findex-ind1) ) : findex-ind1;
        ind1 &= wrapMask;
        auto x0 = samps[ ( (ind1-1) & wrapMask ) ];
        auto x1 = samps[ ind1 ];
        auto x2 = samps[ ( (ind1+1) & wrapMask ) ];
        auto x3 = samps[ ( (ind1+2) & wrapMask ) ];
        return { mu, x0, x1, x2, x3 };
    }

    template< typename Sample >
    struct noneInterpolate
    {
        Sample operator()( const Sample& mu, const Sample& x0, const Sample& x1, const Sample& x2, const Sample& x3 ) const { return x1; }
        Sample operator()( const Sample* samps, const long& wrapMask, const Sample& findex ) const
        {
            return samps[ (static_cast<long>(findex)&wrapMask ) ];
        }
        Sample operator()( const interpVals<Sample>& vals ) const { return vals.x1; }
    };

    template< typename Sample >
    struct linearInterpolate
    {
        Sample operator()( const Sample& mu, const Sample& x0, const Sample& x1, const Sample& x2, const Sample& x3 ) const
            { return calculation( mu, x1, x2 ); }
        Sample operator()( const Sample* samps, const long& wrapMask, const Sample& findex ) const
        {
            auto vals = calculateVals( samps, wrapMask, findex );
            return calculation( vals.mu, vals.x1, vals.x2 );
        }
        Sample operator() ( const interpVals<Sample>& vals ) const { return calculation( vals.mu, vals.x1, vals.x2 ); }
    private:
        inline Sample calculation( const Sample& mu, const Sample& x1, const Sample& x2 ) const
        {
            return x1 + mu*(x2-x1);
        }
    };

    template< typename Sample >
    struct cubicInterpolate
    {
        Sample operator()( const Sample& mu, const Sample& x0, const Sample& x1, const Sample& x2, const Sample& x3 ) const  { return calculation( mu, x0, x1, x2, x3 ); }
        Sample operator()( const Sample* samps, const long& wrapMask, const Sample& findex ) const
        {
            auto vals = calculateVals( samps, wrapMask, findex );
            return calculation( vals.mu, vals.x0, vals.x1, vals.x2, vals.x3 );
        }
        Sample operator()( const interpVals<Sample>& vals ) const { return calculation( vals.mu, vals.x0, vals.x1, vals.x2, vals.x3 ); }
    private:
        inline Sample calculation( const Sample& mu, const Sample& x0, const Sample& x1, const Sample& x2, const Sample& x3 ) const
        {
            auto mu2 = mu*mu;
            auto a0 = x3 - x2 - x0 + x1;
            auto a1 = x0 - x1 - a0;
            auto a2 = x2 - x0;
            return (a0*mu*mu2 + a1*mu2 + a2*mu + x1);
        }
    };

    template< typename Sample >
    struct fourPointInterpolatePD
    {
        Sample operator()( const Sample& mu, const Sample& x0, const Sample& x1, const Sample& x2, const Sample& x3 ) const { return calculation( mu, x0, x1, x2, x3 ); }
        Sample operator()( const Sample* samps, const long& wrapMask, const Sample& findex ) const
            { auto vals = calculateVals( samps, wrapMask, findex );  return calculation( vals.mu, vals.x0, vals.x1, vals.x2, vals.x3 ); }
        Sample operator()( const interpVals<Sample>& vals ) const { return calculation( vals.mu, vals.x0, vals.x1, vals.x2, vals.x3 ); }
    private:
        inline Sample calculation( const Sample& mu, const Sample& x0, const Sample& x1, const Sample& x2, const Sample& x3 ) const
        {
            auto x2minusx1 = x2-x1;
            return x1 + mu * (x2minusx1 - 0.1666667f * (1.0f - mu) * ( (x3 - x0 - 3.0f * x2minusx1) * mu + (x3 + 2.0f*x0 - 3.0f*x1) ) );
        }
    };

    template< typename Sample >
    struct fourPointFourthOrderOptimal
    {
        Sample operator()( const Sample& mu, const Sample& x0, const Sample& x1, const Sample& x2, const Sample& x3 ) const { return calculation( mu, x0, x1, x2, x3 ); }
        Sample operator()( const Sample* samps, const long& wrapMask, const Sample& findex ) const
            { auto vals = calculateVals( samps, wrapMask, findex ); return calculation( vals.mu, vals.x0, vals.x1, vals.x2, vals.x3 ); }
        Sample operator()( const interpVals<Sample>& vals ) const { return calculation( vals.mu, vals.x0, vals.x1, vals.x2, vals.x3 ); }
    private:
        inline Sample calculation( const Sample& mu, const Sample& x0, const Sample& x1, const Sample& x2, const Sample& x3 ) const
        {
            //    Copied from Olli Niemitalo - Polynomial Interpolators for High-Quality Resampling of Oversampled Audio
            // Optimal 2x (4-point, 4th-order) (z-form)
            auto z = mu - 0.5f;
            auto even1 = x2+x1, odd1 = x2-x1;
            auto even2 = x3+x0, odd2 = x3-x0;
            auto c0 = even1*0.45645918406487612 + even2*0.04354173901996461;
            auto c1 = odd1*0.47236675362442071 + odd2*0.17686613581136501;
            auto c2 = even1*-0.253674794204558521 + even2*0.25371918651882464;
            auto c3 = odd1*-0.37917091811631082 + odd2*0.11952965967158000;
            auto c4 = even1*0.04252164479749607 + even2*-0.04289144034653719;
            return (((c4*z+c3)*z+c2)*z+c1)*z+c0;
        }
    };

    template< typename Sample >
    struct cubicInterpolateGodot
    {
        Sample operator()( const Sample& mu, const Sample& x0, const Sample& x1, const Sample& x2, const Sample& x3 ) const { return calculation( mu, x0, x1, x2, x3 ); }
        Sample operator()( const Sample* samps, const long& wrapMask, const Sample& findex ) const
            { auto vals = calculateVals( samps, wrapMask, findex ); return calculation( vals.mu, vals.x0, vals.x1, vals.x2, vals.x3 ); }
        Sample operator()( const interpVals<Sample>& vals ) const { return calculation( vals.mu, vals.x0, vals.x1, vals.x2, vals.x3 ); }
    private:
        inline Sample calculation( const Sample& mu, const Sample& x0, const Sample& x1, const Sample& x2, const Sample& x3 ) const
        {
            //  Godot https://stackoverflow.com/questions/1125666/how-do-you-do-bicubic-or-other-non-linear-interpolation-of-re-sampled-audio-da
//            Sample a0,a1,a2,a3,mu2;
            auto mu2 = mu*mu;
            auto a0 = 3 * x1 - 3 * x2 + x3 - x0;
            auto a1 = 2 * x0 - 5 * x1 + 4 * x2 - x3;
            auto a2 = x2 - x0;
            auto a3 = 2 * x1;
            return (a0 * mu * mu2 + a1 * mu2 + a2 * mu + a3) * 0.5f;
        }
    };

    template< typename Sample >
    struct cubicInterpolateHermite
    {
        Sample operator()( const Sample& mu, const Sample& x0, const Sample& x1, const Sample& x2, const Sample& x3 ) const { return calculation( mu, x0, x1, x2, x3 ); }
        Sample operator()( const Sample* samps, const long& wrapMask, const Sample& findex ) const
            { auto vals = calculateVals( samps, wrapMask, findex ); return calculation( vals.mu, vals.x0, vals.x1, vals.x2, vals.x3 ); }
        Sample operator()( const interpVals<Sample>& vals ) const { return calculation( vals.mu, vals.x0, vals.x1, vals.x2, vals.x3 ); }
    private:
        inline Sample calculation( const Sample& mu, const Sample& x0, const Sample& x1, const Sample& x2, const Sample& x3 ) const
        {
            //  Hermite https://stackoverflow.com/questions/1125666/how-do-you-do-bicubic-or-other-non-linear-interpolation-of-re-sampled-audio-da

            Sample diff = x1 - x2;
            Sample a1 = x2 - x0;
            Sample a3 = x3 - x0 + 3.0f * diff;
            Sample a2 = -(2 * diff + a1 + a3);
            return 0.5f * ((a3 * mu + a2) * mu + a1) * mu + x1;
        }
    };

    template< typename Sample >
    struct interpolator
    {
        inline Sample operator()( Sample* samps, long size, Sample findex ) const
            { return std::visit( interpTypeVisitor{samps,size,findex}, m_interpolators[m_interpType] ); }
        
        void setInterpolationType( interpolatorTypes interpType )
        {
            switch ( interpType ) {
                case interpolatorTypes::none:           m_interpType = 0; return;
                case interpolatorTypes::linear :        m_interpType = 1; return;
                case interpolatorTypes::cubic :         m_interpType = 2; return;
                case interpolatorTypes::pureData :      m_interpType = 3; return;
                case interpolatorTypes::fourthOrder :   m_interpType = 4; return;
                case interpolatorTypes::godot :         m_interpType = 5; return;
                case interpolatorTypes::hermite :       m_interpType = 6; return;
                default:                                m_interpType = 1; return;
            }
        }
    private:
        
        struct interpTypeVisitor
        {
            Sample* S;
            long Size;
            Sample f;

            Sample operator()( noneInterpolate<Sample>& i ) const { return i( calculateVals( S, Size, f ) ); }
            Sample operator()( linearInterpolate<Sample>& i ) const { return i( calculateVals( S, Size, f ) ); }
            Sample operator()( cubicInterpolate<Sample>& i ) const { return i( calculateVals( S, Size, f ) ); }
            Sample operator()( fourPointInterpolatePD<Sample>& i ) const { return i( calculateVals( S, Size, f ) ); }
            Sample operator()( fourPointFourthOrderOptimal<Sample>& i ) const { return i( calculateVals( S, Size, f ) ); }
            Sample operator()( cubicInterpolateGodot<Sample>& i ) const { return i( calculateVals( S, Size, f ) ); }
            Sample operator()( cubicInterpolateHermite<Sample>& i ) const { return i( calculateVals( S, Size, f ) ); }
        };
        

        
        int m_interpType = 1; // linear
        using interpVariant = std::variant< noneInterpolate<Sample>, linearInterpolate<Sample>, cubicInterpolate<Sample>, fourPointInterpolatePD<Sample>, fourPointFourthOrderOptimal<Sample>, cubicInterpolateGodot<Sample>, cubicInterpolateHermite<Sample> >;
        std::array< interpVariant, 7 > m_interpolators
        {
            noneInterpolate<Sample>(), linearInterpolate<Sample>(), cubicInterpolate<Sample>(),
            fourPointInterpolatePD<Sample>(), fourPointFourthOrderOptimal<Sample>(), cubicInterpolateGodot<Sample>(),
            cubicInterpolateHermite<Sample>()
        };
    };
}

#endif /* sjf_interpolator_h */
