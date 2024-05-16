//
//  sjf_nonlinearities.h
//
//  Created by Simon Fay on 14/05/2024.
//  Copyright © 2024 sjf. All rights reserved.
//

#ifndef sjf_nonlinearities_h
#define sjf_nonlinearities_h

#include "sjf_audioUtilitiesC++.h"
namespace sjf::nonlinearities
{
    /** cubic soft clipping */
    template< typename Sample >
    inline Sample cubic( Sample x )
    {
        static constexpr Sample div{1.0/3.0}, lim{div*2};
        return x <= 1 ? -lim : x >= 1 ? lim : x - x*x*x*div; //
    }

    /** approximation of tanh function
     https://www.musicdsp.org/en/latest/Other/238-rational-tanh-approximation.html
     */
    template< typename Sample >
    inline Sample tanhSimple( Sample x )
    {
        return x < 3 ? -1 : x > 3 ? 1 : x*(27 + x*x)/(27+9*x*x);
    }
}
#endif /* sjf_nonlinearities_h */
