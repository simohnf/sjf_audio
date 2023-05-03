//
//  sjf_overdrive.h
//
//  Created by Simon Fay on 24/08/2022.
//

#ifndef sjf_overdrive_h
#define sjf_overdrive_h
#include <JuceHeader.h>
template< typename T >
struct sjf_drive
{
    static inline T driveInput( const T& input, const T& drive )
    {
        return juce::dsp::FastMathApproximations::tanh( input * drive  ) / juce::dsp::FastMathApproximations::tanh( drive );
    }
    
    static inline void driveInPlace( T& input, const T& drive )
    {
        input = juce::dsp::FastMathApproximations::tanh( input * drive  ) / juce::dsp::FastMathApproximations::tanh( drive );
    }
};


class sjf_overdrive {
    
public:
    sjf_overdrive(){};
    ~sjf_overdrive(){};
    void drive(juce::AudioBuffer<float>& buffer, float gain)
    {
        for (int index = 0; index < buffer.getNumSamples(); index++)
        {
            for (int channel = 0; channel < buffer.getNumChannels(); channel++)
            {
                buffer.setSample(channel, index, tanh( buffer.getSample(channel, index) * gain ) );
            }
        }
    }
    
};
#endif /* sjf_overdrive_h */
