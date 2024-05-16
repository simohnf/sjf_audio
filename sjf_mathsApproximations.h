//
//  sjf_mathsApproximations.h
//  sjf_verb
//
//  Created by Simon Fay on 14/05/2024.
//  Copyright © 2024 sjf. All rights reserved.
//

#ifndef sjf_mathsApproximations_h
#define sjf_mathsApproximations_h

namespace sjf::maths
{
    template< typename Sample >
    inline Sample sinApprox( Sample x )
    {
        bool neg = x < 0;
        x = std::abs( x );
        auto pix  = M_PI - x;
        auto num = 16 * x * pix;
        auto den = 5 * M_PI * M_PI - 4*x * pix;
        return neg ? -num/den : num/den;
    }
}

#endif /* sjf_mathsApproximations_h */
