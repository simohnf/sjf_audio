//
//  sjf_oscillator.h
//  sjf_granSynth
//
//  Created by Simon Fay on 24/08/2022.
//

#ifndef sjf_oscillator_h
#define sjf_oscillator_h

#define PI 3.14159265

class sjf_oscillator {
    
public:
    sjf_oscillator(){
        waveTable.resize(waveTableSize);
        setSine();
    };
    ~sjf_oscillator(){};
    
    void setFrequency(float f)
    {
        initialise(SR, f);
    };
    
    void initialise(int sampleRate, float f)
    {
        SR = sampleRate;
        frequency = f;
        read_increment = frequency * waveTableSize / SR ;
    };
    
    void setSine(){
        for (int index = 0; index< waveTableSize; index++)
        {
            waveTable[index] = sin( index * 2 * PI / waveTableSize ) ;
        }
    };
    
    std::vector<float> outputBlock(int numSamples)
    {
        outBuff.resize( numSamples ) ;
        for ( int index = 0; index < numSamples; index++ )
        {
            outBuff[index] = cubicInterpolate(waveTable, read_pos);
            read_pos += read_increment;
            if (read_pos >= waveTableSize)
            {
                read_pos -= waveTableSize;
            }
        }
        return outBuff;
    };
    
    std::vector<float> outputBlock(int numSamples, float gain)
    {
        outBuff.resize( numSamples ) ;
        for ( int index = 0; index < numSamples; index++ )
        {
            outBuff[index] = cubicInterpolate(waveTable, read_pos) * gain;
            read_pos += read_increment;
            if (read_pos >= waveTableSize)
            {
                read_pos -= waveTableSize;
            }
        }
        return outBuff;
    };
    
    float outputSample(int numSamples)
    {
        float out = cubicInterpolate(waveTable, read_pos);
        read_pos += numSamples * read_increment;
        if (read_pos >= waveTableSize)
        {
            read_pos -= waveTableSize;
        }
        return out;
    };
    
private:
    float waveTableSize = 512;
    float SR = 44100;
    float read_pos = 0;
    float frequency = 440;
    float read_increment = ( frequency * SR ) / waveTableSize;
    std::vector<float> waveTable, outBuff;
};

#endif /* sjf_oscillator_h */
