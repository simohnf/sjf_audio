//
//  sjf_overdrive.h
//
//  Created by Simon Fay on 24/08/2022.
//

#ifndef sjf_overdrive_h
#define sjf_overdrive_h
#include <JuceHeader.h>

class sjf_overdrive {
    
public:
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
