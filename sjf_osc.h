//
//  sjf_osc.h
//
//  Created by Simon Fay on 07/10/2022.
//
//==============================================================================
#ifndef sjf_osc_h
#define sjf_osc_h

class sjf_osc
{
public:
    sjf_osc()
    {
        setSine();
        m_increment = m_frequency * m_wavetableSize / m_SR;
    }
    ~sjf_osc(){}
    
    void initialise( int sampleRate )
    {
        if ( sampleRate > 0 ) { m_SR = sampleRate; }
        setFrequency( m_frequency );
    }
    
    void initialise( int sampleRate, float frequency )
    {
        if ( sampleRate > 0 ) { m_SR = sampleRate; }
        setFrequency( frequency );
    }
    
    void setFrequency( float frequency )
    {
        m_frequency = frequency;
        m_increment = m_frequency * m_wavetableSize / m_SR;
    }
    
    float getSample( )
    {
        m_readPos +=  m_increment;
        while ( m_readPos >= m_wavetableSize )
        { m_readPos -= m_wavetableSize; }
        return linearInterpolate( m_readPos );
    }
    
    float getSampleHQ( )
    {
        m_readPos +=  m_increment;
        while ( m_readPos >= m_wavetableSize )
        { m_readPos -= m_wavetableSize; }
        return cubicInterpolate( m_readPos );
    }
    
    
private:
    
    void setSine()
    {
        for (int index = 0; index< m_wavetableSize; index++)
        {
            m_table[index] = sin( index * 2 * m_PI / m_wavetableSize ) ;
        }
    }
    
    float cubicInterpolate(float readPos)
    {
        //        double y0; // previous step value
        //        double y1; // this step value
        //        double y2; // next step value
        //        double y3; // next next step value
        //        double mu; // fractional part between step 1 & 2
        findex = readPos;
        if(findex < 0){ findex+= m_wavetableSize;}
        else if(findex > m_wavetableSize){ findex-= m_wavetableSize;}
        
        index = findex;
        mu = findex - index;
        
        if (index == 0)
        {
            y0 = m_table[ m_wavetableSize - 1 ];
        }
        else
        {
            y0 = m_table[ index - 1 ];
        }
        y1 = m_table[ index % m_wavetableSize ];
        y2 = m_table[ (index + 1) % m_wavetableSize ];
        y3 = m_table[ (index + 2) % m_wavetableSize ];
        
        
        mu2 = mu*mu;
        a0 = y3 - y2 - y0 + y1;
        a1 = y0 - y1 - a0;
        a2 = y2 - y0;
        a3 = y1;
        
        return (a0*mu*mu2 + a1*mu2 + a2*mu + a3);
    }
    
    float linearInterpolate( float read_pos )
    {
        //        auto bufferSize = buffer.size();
        //        double y1; // this step value
        //        double y2; // next step value
        //        double mu; // fractional part between step 1 & 2
        
        findex = read_pos;
        if(findex < 0){ findex+= m_wavetableSize;}
        else if(findex > m_wavetableSize){ findex-= m_wavetableSize;}
        
        index = findex;
        mu = findex - index;
        
        y1 = m_table[ index % m_wavetableSize ];
        y2 = m_table[ (index + 1) % m_wavetableSize ];
        
        return y1 + mu*(y2-y1) ;
    }
    
    const static int m_wavetableSize = 512;
    double m_PI = 3.14159265;
    float m_SR = 44100.0f, m_frequency = 1.0f, m_readPos = 0, m_increment;
    std::array< float, m_wavetableSize > m_table;
    
    // variables for cubic interpolation (to save allocation time)
    double a0, a1, a2, a3, mu, mu2;
    double y0, y1, y2, y3; // fractional part between step 1 & 2
    float findex;
    int index;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_osc )
};

#endif /* sjf_osc_h */
