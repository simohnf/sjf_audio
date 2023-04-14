//
//  sjf_convo.h
//
//  Created by Simon Fay on 13/04/2023.
//

#ifndef sjf_convo_h
#define sjf_convo_h
#include "sjf_audioUtilities.h"
//#include "sjf_interpolationTypes.h"
#include "sjf_interpolators.h"
#include <JuceHeader.h>

#include <vector>

//------------------------------------------------
//------------------------------------------------
//------------------------------------------------
template< class T >
class sjf_convo
{
    
public:
    sjf_convo() { };
    ~sjf_convo() {};
    
   
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_convo )
};




inline float simdInnerProduct (float* in, float* kernel, int numSamples, float y = 0.0f)
{
    constexpr size_t simdN = dsp::SIMDRegister<float>::SIMDNumElements;
    
    // compute SIMD products
    int idx = 0;
    for (; idx <= numSamples - simdN; idx += simdN)
    {
        auto simdIn = loadUnaligned (in + idx);
        auto simdKernel = dsp::SIMDRegister<float>::fromRawArray (kernel + idx);
        y += (simdIn * simdKernel).sum();
    }
    
    // compute leftover samples
    y = std::inner_product (in + idx, in + numSamples, kernel + idx, y);
    
    return y;
    }


#endif /* sjf_convo_h */
