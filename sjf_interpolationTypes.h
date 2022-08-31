//
//  sjf_interpolationTypes.h
//
//  Created by Simon Fay on 24/08/2022.
//

#ifndef sjf_interpolationTypes_h
#define sjf_interpolationTypes_h

#include <JuceHeader.h>
#include <vector>
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
inline
float linearInterpolate(std::vector<float>& buffer, float read_pos)
{
    auto bufferSize = buffer.size();
    double y1; // this step value
    double y2; // next step value
    double mu; // fractional part between step 1 & 2
    
    float findex = read_pos;
    if(findex < 0){ findex+= bufferSize;}
    else if(findex > bufferSize){ findex-= bufferSize;}
    
    int index = findex;
    mu = findex - index;
    
    y1 = buffer[ index % bufferSize ];
    y2 = buffer[ (index + 1) % bufferSize ];
    
    return y1 + mu*(y2-y1) ;
}
//==============================================================================
inline
float cubicInterpolate(std::vector<float> buffer, float read_pos)
{
    auto bufferSize = buffer.size();
    double y0; // previous step value
    double y1; // this step value
    double y2; // next step value
    double y3; // next next step value
    double mu; // fractional part between step 1 & 2
    
    float findex = read_pos;
    if(findex < 0){ findex+= bufferSize;}
    else if(findex > bufferSize){ findex-= bufferSize;}
    
    int index = findex;
    mu = findex - index;
    
    if (index == 0)
    {
        y0 = buffer[ bufferSize - 1 ];
    }
    else
    {
        y0 = buffer[ index - 1 ];
    }
    y1 = buffer[ index % bufferSize ];
    y2 = buffer[ (index + 1) % bufferSize ];
    y3 = buffer[ (index + 2) % bufferSize ];
    double a0,a1,a2,a3,mu2;
    
    mu2 = mu*mu;
    a0 = y3 - y2 - y0 + y1;
    a1 = y0 - y1 - a0;
    a2 = y2 - y0;
    a3 = y1;
    
    return (a0*mu*mu2 + a1*mu2 + a2*mu + a3);
}
//==============================================================================
inline
float cubicInterpolate(juce::AudioBuffer<float>& buffer, int channel, float read_pos)
{
    auto bufferSize = buffer.getNumSamples();
    double y0; // previous step value
    double y1; // this step value
    double y2; // next step value
    double y3; // next next step value
    double mu; // fractional part between step 1 & 2
    
    float findex = read_pos;
    if(findex < 0){ findex+= bufferSize;}
    else if(findex > bufferSize){ findex-= bufferSize;}
    
    int index = findex;
    mu = findex - index;
    
    if (index == 0)
    {
        y0 = buffer.getSample(channel, bufferSize - 1);
    }
    else
    {
        y0 = buffer.getSample(channel, index - 1);
    }
    y1 = buffer.getSample(channel, index % bufferSize);
    y2 = buffer.getSample(channel, (index + 1) % bufferSize);
    y3 = buffer.getSample(channel, (index + 2) % bufferSize);
    double a0,a1,a2,a3,mu2;
    
    mu2 = mu*mu;
    a0 = y3 - y2 - y0 + y1;
    a1 = y0 - y1 - a0;
    a2 = y2 - y0;
    a3 = y1;
    
    return (a0*mu*mu2 + a1*mu2 + a2*mu + a3);
}
//==============================================================================
inline
float fourPointInterpolatePD(juce::AudioBuffer<float>& buffer, int channel, float read_pos)
{
    auto bufferSize = buffer.getNumSamples();
    double y0; // previous step value
    double y1; // this step value
    double y2; // next step value
    double y3; // next next step value
    double mu; // fractional part between step 1 & 2
    
    float findex = read_pos;
    if(findex < 0){ findex+= bufferSize;}
    else if(findex > bufferSize){ findex-= bufferSize;}
    
    int index = findex;
    mu = findex - index;
    
    if (index == 0)
    {
        y0 = buffer.getSample(channel, bufferSize - 1);
    }
    else
    {
        y0 = buffer.getSample(channel, index - 1);
    }
    y1 = buffer.getSample(channel, index % bufferSize);
    y2 = buffer.getSample(channel, (index + 1) % bufferSize);
    y3 = buffer.getSample(channel, (index + 2) % bufferSize);
    
    auto y2minusy1 = y2-y1;
    return y1 + mu * (y2minusy1 - 0.1666667f * (1.0f - mu) * ( (y3 - y0 - 3.0f * y2minusy1) * mu + (y3 + 2.0f*y0 - 3.0f*y1) ) );
}
//==============================================================================
inline
float linearInterpolate(juce::AudioBuffer<float>& buffer, int channel, float read_pos)
{
    auto bufferSize = buffer.getNumSamples();
    double y1; // this step value
    double y2; // next step value
    double mu; // fractional part between step 1 & 2
    
    float findex = read_pos;
    if(findex < 0){ findex+= bufferSize;}
    else if(findex > bufferSize){ findex-= bufferSize;}
    
    int index = findex;
    mu = findex - index;
    
    y1 = buffer.getSample(channel, index % bufferSize);
    y2 = buffer.getSample(channel, (index + 1) % bufferSize);
    
    return y1 + mu*(y2-y1) ;
}
//==============================================================================
inline
float fourPointFourthOrderOptimal(juce::AudioBuffer<float>& buffer, int channel, float read_pos)
{
    //    Copied from Olli Niemitalo - Polynomial Interpolators for High-Quality Resampling of Oversampled Audio
    auto bufferSize = buffer.getNumSamples();
    double y0; // previous step value
    double y1; // this step value
    double y2; // next step value
    double y3; // next next step value
    double mu; // fractional part between step 1 & 2
    
    float findex = read_pos;
    if(findex < 0){ findex+= bufferSize;}
    else if(findex > bufferSize){ findex-= bufferSize;}
    
    int index = findex;
    mu = findex - index;
    
    if (index == 0)
    {
        y0 = buffer.getSample(channel, bufferSize - 1);
    }
    else
    {
        y0 = buffer.getSample(channel, index - 1);
    }
    y1 = buffer.getSample(channel, index % bufferSize);
    y2 = buffer.getSample(channel, (index + 1) % bufferSize);
    y3 = buffer.getSample(channel, (index + 2) % bufferSize);
    
    
    // Optimal 2x (4-point, 4th-order) (z-form)
    float z = mu - 1/2.0;
    float even1 = y2+y1, odd1 = y2-y1;
    float even2 = y3+y0, odd2 = y3-y0;
    float c0 = even1*0.45645918406487612 + even2*0.04354173901996461;
    float c1 = odd1*0.47236675362442071 + odd2*0.17686613581136501;
    float c2 = even1*-0.253674794204558521 + even2*0.25371918651882464;
    float c3 = odd1*-0.37917091811631082 + odd2*0.11952965967158000;
    float c4 = even1*0.04252164479749607 + even2*-0.04289144034653719;
    return (((c4*z+c3)*z+c2)*z+c1)*z+c0;
    
}
//==============================================================================
inline
float cubicInterpolateGodot(juce::AudioBuffer<float>& buffer, int channel, float read_pos)
{
    //    Copied from Olli Niemitalo - Polynomial Interpolators for High-Quality Resampling of Oversampled Audio
    auto bufferSize = buffer.getNumSamples();
    double y0; // previous step value
    double y1; // this step value
    double y2; // next step value
    double y3; // next next step value
    double mu; // fractional part between step 1 & 2
    
    float findex = read_pos;
    if(findex < 0){ findex+= bufferSize;}
    else if(findex > bufferSize){ findex-= bufferSize;}
    
    int index = findex;
    mu = findex - index;
    
    if (index == 0)
    {
        y0 = buffer.getSample(channel, bufferSize - 1);
    }
    else
    {
        y0 = buffer.getSample(channel, index - 1);
    }
    y1 = buffer.getSample(channel, index % bufferSize);
    y2 = buffer.getSample(channel, (index + 1) % bufferSize);
    y3 = buffer.getSample(channel, (index + 2) % bufferSize);
    double a0,a1,a2,a3,mu2;
    mu2 = mu*mu;
    
    a0 = 3 * y1 - 3 * y2 + y3 - y0;
    a1 = 2 * y0 - 5 * y1 + 4 * y2 - y3;
    a2 = y2 - y0;
    a3 = 2 * y1;
    
    return (a0 * mu * mu2 + a1 * mu2 + a2 * mu + a3) / 2;
}
//==============================================================================
inline
float cubicInterpolateHermite(juce::AudioBuffer<float>& buffer, int channel, float read_pos)
{
    auto bufferSize = buffer.getNumSamples();
    double y0; // previous step value
    double y1; // this step value
    double y2; // next step value
    double y3; // next next step value
    double mu; // fractional part between step 1 & 2
    
    float findex = read_pos;
    if(findex < 0){ findex+= bufferSize;}
    else if(findex > bufferSize){ findex-= bufferSize;}
    
    int index = findex;
    mu = findex - index;
    
    if (index == 0)
    {
        y0 = buffer.getSample(channel, bufferSize - 1);
    }
    else
    {
        y0 = buffer.getSample(channel, index - 1);
    }
    y1 = buffer.getSample(channel, index % bufferSize);
    y2 = buffer.getSample(channel, (index + 1) % bufferSize);
    y3 = buffer.getSample(channel, (index + 2) % bufferSize);
    double a0,a1,a2,a3,mu2;
    mu2 = mu*mu;
    
    a0 = y1;
    a1 = 0.5f * (y2 - y0);
    a2 = y0 - (2.5f * y1) + (2 * y2) - (0.5f * y3);
    a3 = (0.5f * (y3 - y0)) + (1.5F * (y1 - y2));
    return (((((a3 * mu) + a2) * mu) + a1) * mu) + a0;
}

#endif /* sjf_interpolationTypes_h */
