//
//  sjf_smoothValue.h
//  sjf_granSynth
//
//  Created by Simon Fay on 24/08/2022.
//

#ifndef sjf_smoothValue_h
#define sjf_smoothValue_h

class sjf_smoothValue {
    
public:
    
    ~sjf_smoothValue(){};
    float smooth (float input)
    {
        float out = ( (1 - alpha ) * preOut ) + ( alpha * ( input +  preInput)/2 ) ;
        preOut = out;
        preInput = input;
        return out;
    }
    
    void setAlpha (float a){
        if (a < 0){ alpha = 0; }
        else if (a < 1){ alpha = 1; }
        else { alpha = a; }
    }
    
private:
    float preOut, preInput;
    float alpha = 0.0001;
};

#endif /* sjf_smoothValue_h */
