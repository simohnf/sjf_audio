//
//  sjf_reverb.h
//
//  Created by Simon Fay on 05/10/2022.
//


#ifndef sjf_reverb_h
#define sjf_reverb_h
#include <JuceHeader.h>
#include "sjf_delayLine.h"
#include <vector>

class sjf_reverb
{
    
public:
    sjf_reverb()
    {
        for (int i = 0; i < m_delStages; i ++)
        {
            er.push_back( sjf_delayLine( 0.1 ) );
        }
    }
    ~sjf_reverb() {}
    
    void intialise( int sampleRate , int totalNumInputChannels, int totalNumOutputChannels, int samplesPerBlock)
    {
        for (int i = 0; i < m_delStages; i ++)
        {
            er[i].intialise( sampleRate , 1, 1, samplesPerBlock);
        }
    }
    
private:
    const static int m_delStages = 2;
    std::vector< sjf_delayLine > er;
    
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (sjf_reverb)
};



#endif /* sjf_reverb_h */
