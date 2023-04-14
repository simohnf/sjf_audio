#ifndef JUCEFIR_H_INCLUDED
#define JUCEFIR_H_INCLUDED

//#include "BaseFilter.h"


// copied from https://github.com/jatinchowdhury18/FIRBenchmarks/blob/master/src/JuceFIR.h


/** FIR processor using juce::dsp::FIR */
class JuceFIR //: public BaseFilter
{
public:
    JuceFIR() {}
    virtual ~JuceFIR() {}
    
    
    void prepare (double sampleRate, int samplesPerBlock)
    {
        filt.prepare ({ sampleRate, (juce::uint32) samplesPerBlock, 1 });
    }
    
    void processBlock (juce::AudioBuffer<float>& buffer)
    {
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        filt.process (ctx);
    }
    
    void loadIR (const juce::AudioBuffer<float>& irBuffer)
    {
        filt.coefficients = new juce::dsp::FIR::Coefficients<float> (irBuffer.getReadPointer (0), irBuffer.getNumSamples());
        filt.reset();
    }
    
private:
    juce::dsp::FIR::Filter<float> filt;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JuceFIR)
};

#endif // JUCEFIR_H_INCLUDED
