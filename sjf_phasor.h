//
//  sjf_phasor.h
//  sjf_granSynth
//
//  Created by Simon Fay on 24/08/2022.
//

#ifndef sjf_phasor_h
#define sjf_phasor_h

class sjf_phasor{
public:
    sjf_phasor() { calculateIncrement() ; };
    sjf_phasor(float sample_rate, float f) { initialise(sample_rate, f); };
    
    ~sjf_phasor() {};
    
    void initialise(float sample_rate, float f)
    {
        SR = sample_rate;
        setFrequency( f );
    };
    
    void setSampleRate( float sample_rate)
    {
        SR = sample_rate;
        calculateIncrement();
    }
    void setFrequency(float f)
    {
        frequency = f;
        
        if (frequency >= 0)
        {
            negFreq = false;
            increment = frequency / SR ;
        }
        else
        {
            negFreq = true;
            increment = -1*frequency / SR ;
        }
        
        
        
    };
    
    float getFrequency(){ return frequency ;};
    
    float output()
    {
        if (!negFreq)
        {
            float p = position;
            position += increment;
            if (position >= 1){ position -= 1; }
            return p;
        }
        else
        {
            float p = position;
            position += increment;
            while (position >= 1){ position -= 1; }
            return 1 - p;
        }
    };
    
    void setPhase(float p)
    {
        if (p < 0) {p = 0;}
        else if (p > 1){ p = 1 ;}
        position = p;
    };
    float getPhase(){
        return position;
    }
private:
    void calculateIncrement(){ increment = ( frequency / SR ); };
    
    float frequency = 440;
    float SR = 44100;
    float increment;
    float position = 0;
    bool negFreq = false;
};

#endif /* sjf_phasor_h */
