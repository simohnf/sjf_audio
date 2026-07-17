//
//  sjf_conductor.h
//
//  Created by Simon Fay on 13/09/2022.
//

#ifndef sjf_conductor_h
#define sjf_conductor_h

#include <JuceHeader.h>

// class to act as rhythmic generator for sequence based devices
//                  TO DO
// -- Improve rhythmic accuracy using sample based logic (as per gransynth)
//          -- output new triggers at sample point
// -- Possibly incorporate AAIM rhythm gen style logic
// -- Create enums for division and tuplet type
class sjf_conductor
{
public:
    //==============================================================================
    sjf_conductor(){}; // default constructor
    //==============================================================================
    ~sjf_conductor(){}; // default destructor
    //==============================================================================
    // simple calculation of current step based on host position
    // NOTE:not sample accurate so limited to block size!
    int getCurrentStep( double hostPosition )
    {
        if (!m_isOnFlag){ return -1; }
        m_currentStep = static_cast<int>(hostPosition * m_div * m_tuplet) % m_nSteps;
        return m_currentStep;
    }
    //==============================================================================
    // output of current step for GUI display
    int getCurrentStep(){ return m_currentStep; }
    //==============================================================================
    // sets the rhythmic division type
    void setDivision( int rhythmicDivision )
    {
        rhythmicDivision -= 1;
        m_div = pow(2, static_cast<float>(rhythmicDivision));
    }
    //==============================================================================
    // sets the current tuplet type
    void setTuplet( int tupletType )
    {
        switch(tupletType)
        {
            case 1:
                m_tuplet = 1.0f; // tuplets
                break;
            case 2:
                m_tuplet = 0.75f; // dotted
                break;
            case 3:
                m_tuplet = 3.0f; // triplets
                break;
            case 4:
                m_tuplet = 5.0f/4.0f; // quintuplets
                break;
        }
        return;
    }
    //==============================================================================
    void setNumSteps( int nSteps ){ m_nSteps = nSteps; } // sets the number of beats to count through
    //==============================================================================
    void turnOn( bool onOrOff ){ m_isOnFlag = onOrOff; } // turns on/off the conductor
    //==============================================================================
    bool isOn(){ return m_isOnFlag; } // returns whether the conductor is on/off
    //==============================================================================
private:
    int m_nSteps = 32; // default number of steps
    int m_currentStep; // used to keep track of count
    float m_div = 1; // the rhythmic division value
    float m_tuplet = 1; // the tuplet value
    bool m_isOnFlag = false; // is the conductor on or off
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (sjf_conductor)
};

#endif /* sjf_conductor_h */
