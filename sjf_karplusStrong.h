//
//  sjf_karplusStrong.h
//  sjf_karplusStrong
//
//  Created by Simon Fay on 19/06/2023.
//  Copyright © 2023 sjf. All rights reserved.
//

#ifndef sjf_karplusStrong_h
#define sjf_karplusStrong_h
#include "sjf_delayLine.h"
#include "sjf_audioUtilitiesC++.h"
#include "sjf_lpf.h"

class sjf_envelopeOneShot
{
public:
    sjf_envelopeOneShot()
    {
        createTable( m_attSamps, m_decSamps, m_sus, m_relSamps );
    }
    ~sjf_envelopeOneShot(){}
    void prepare( double sampleRate )
    {
        m_SR = sampleRate;
    }
    
    void setEnvelope( double attackMS, double decayMS, double sustain, double releaseMS )
    {

#ifndef NDEBUG
        assert( attackMS > 0 );
        assert( decayMS > 0 );
        assert( sustain > 0 );
        assert( releaseMS > 0 );
#endif
        auto sampPerMS = m_SR * 0.001;
        m_attSamps = attackMS * sampPerMS;
        m_decSamps = decayMS * sampPerMS;
        m_sus = sustain;
        m_relSamps = releaseMS * sampPerMS;
        createTable( m_attSamps, m_decSamps, m_sus, m_relSamps );
    }
    
    void triggerNewEnvelope()
    {
        m_pos = 0;
    }
    
    double outputEnvelope()
    {
        if ( m_pos >= m_table.size() )
            return 0;
        auto output = m_table[ m_pos ];
        m_pos += 1;
        return output;
    }
    
    size_t getPosition()
    {
        return m_pos;
    }
    
private:
    void createTable( int attSamps, int decSamps, int sustain, int relSamps )
    {
        auto length = attSamps + decSamps + relSamps;
        m_table.resize( length );
        for ( auto i = 0; i < m_attSamps; i++ )
            m_table[ i ] = static_cast< double >( i ) / m_attSamps;
        
        for ( auto i = 0; i < m_decSamps; i++ )
        {
            auto val = (m_decSamps - i) / m_decSamps;
            val = sjf_scale< double >( val, 0, 1, m_sus, 1 );
            auto pos = i + m_attSamps;
            m_table[ pos ] = val;
        }
        
        for ( auto i = 0; i < m_relSamps; i++ )
        {
            auto val = m_sus * ( m_relSamps - i ) / m_relSamps;
            auto pos = i + m_attSamps + m_decSamps;
            m_table[ pos ] = val;
        }
    }
    
    int m_attSamps = 44, m_decSamps = 441, m_relSamps = 44;
    double m_sus = 0.707, m_SR = 44100;
    std::vector< double > m_table;
    size_t m_pos = 0;
};

class sjf_karplusStrongVoice
{
public:
    sjf_karplusStrongVoice()
    {
        initialise_noiseBurst( m_SR );
        m_attackDamping.setCutoff( 0.999 );
    }
    ~sjf_karplusStrongVoice(){}
    
    void prepare( double sampleRate )
    {
        m_SR = sampleRate;
        initialise_noiseBurst( m_SR );
        m_attackEnvelope.prepare( m_SR );
        
    }
    
    void triggerNewNote( int midiPitch, int midiVelocity )
    {
        m_period = m_SR / midiToFrequency< double >( midiPitch );
        m_velocity = static_cast< double >( midiVelocity ) / 127;
        m_attackEnvelope.triggerNewEnvelope();
        m_delay.setDelayTimeSamps( m_period );
    }
    
    double processSample( int indexThroughCurrentBuffer )
    {
        // output == delayed plus noise burst
        auto outputSamp = m_mediumDamping.filterInput( m_delay.getSample( indexThroughCurrentBuffer ) ) * m_sustain;
//        auto excitation = 0.0;
        auto attEnv = m_attackEnvelope.outputEnvelope();
        if ( attEnv > 0 )
        {
            outputSamp += m_noise[ m_attackEnvelope.getPosition() ] * attEnv * m_velocity;
        }
//        outputSamp += ;
        m_delay.setSample( indexThroughCurrentBuffer, outputSamp );
        
        return outputSamp;
        
    }
    
    
    void setEnvelope( double attackMS, double decayMS, double sustain, double releaseMS )
    {
        m_attackEnvelope.setEnvelope( attackMS, decayMS, sustain, releaseMS );
    }
    
    
    
private:
    void initialise_noiseBurst( double sampleRate )
    {
        auto nSamps = static_cast< int >( sampleRate * 1.0/20.0 );
        if ( nSamps == m_noise.size() )
            return;
        m_noise.resize( nSamps ); // default to 44100 sample rate
        for ( auto i = 0; i < m_noise.size(); i++ )
            m_noise[ i ] = ( rand01()* 2.0 ) - 1.0;
    }
    
    std::vector< double > m_noise;
    sjf_delayLine< double > m_delay;
    sjf_lpf< double > m_mediumDamping, m_attackDamping;
    
    sjf_envelopeOneShot m_attackEnvelope;
    
    double m_SR = 44100, m_period = ( m_SR / 220 ), m_velocity = 100, m_sustain = 0.99;
    
};


#endif /* sjf_karplusStrong_h */
