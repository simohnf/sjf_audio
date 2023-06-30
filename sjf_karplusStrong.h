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
#include <limits>

#define MINIMUM_FREQUENCY 5.0
#define MINIMUM_PERIOD 0.1 // minimum period before extending
template< class T >
class sjf_onepole
{
public:
    sjf_onepole(){}
    ~sjf_onepole(){}
    
    T process( T x )
    {
        m_y0 += m_cf * ( x - m_y0 );
        return m_y0;
    }
    
    T processHp( T x )
    {
        m_y0 += m_cf * ( x - m_y0 );
        return x - m_y0;
    }
    void clear()
    {
        m_y0 = 0;
    }
    
    void setCutoff( T f, T sr )
    {
        m_cf = sin (2. * M_PI * f / sr );
    }
private:
    T m_y0 = 0, m_cf = 0.5;
    
};

template < class T >
class sjf_threePointHighPass
{
public:
    sjf_threePointHighPass(){}
    ~sjf_threePointHighPass(){}
    
    T process( T x )
    {
        m_y1 = m_a0*x + m_a1*m_x1 + m_b1*m_y1;
        m_x1 = x;
        return m_y1;
    }
    
    void setCutoff( T f, T sr )
    {
        auto w2 = sin (2. * M_PI * f / sr ) / 2;
        m_a0 = 1.0 / ( 1 + w2 );
        m_a1 = -1.0 * m_a0;
        m_b1 = m_a0 * ( 1  - w2 );
    }
    
    void clear()
    {
        m_x1 = 0;
        m_y1 = 0;
    }
    
private:
    T m_x1 = 0, m_y1 = 0;
    T m_a0 = 0.5, m_a1 = 0.5, m_b1 = 0.5;
};

class sjf_karplusStrongVoice
{
public:
    sjf_karplusStrongVoice()
    {
        m_delay.initialise( m_SR / MINIMUM_FREQUENCY );
        m_delay.setInterpolationType( sjf_interpolators::interpolatorTypes::linear );
        m_dcBlock.setCutoff( 30.0, m_SR );
        m_attackDamping.setCutoff( 10000.0, m_SR );
        m_mediumDamping.setCutoff( 10000.0, m_SR );
        setNoteOffReleaseTime( 50.0 * 0.001 * m_SR );
        
    }
    ~sjf_karplusStrongVoice(){}
    
    void prepare( double sampleRate )
    {
        m_SR = sampleRate;
        m_delay.initialise( m_SR / MINIMUM_FREQUENCY );
        m_delay.clearDelayline();
        m_dcBlock.setCutoff( 30.0, m_SR );
        m_attackDamping.setCutoff( 10000.0, m_SR );
        m_mediumDamping.setCutoff( 10000.0, m_SR );
        setNoteOffReleaseTime( 50.0 * 0.001 * m_SR );
    }
    
    void triggerNewNote( int midiPitch, int midiVelocity )
    {
        m_busyFlag = true;
        m_triggerRelease = false;
        m_triggerAttack = true;
        m_attackIndex = 0;
        m_attackMin = abs(m_lastOutSamp);
        
        m_pitch = midiPitch;
        auto fundamental = midiToFrequency< double >( m_pitch );
//        auto periodS = 1.0 / fundamental;
        m_period = m_SR /fundamental;
        m_velocity = sjf_scale< double >( static_cast< double >( midiVelocity ) / 127.0, 0, 1, 0.3, 1 );
        
        m_attackDamping.setCutoff( m_attackBright * m_velocity, m_SR );
        m_attackDamping.clear();
        
        auto mediumCutoff = std::fmin( 10000.0 , fundamental * ( ( m_mediumBright * 12.0 ) + 4.0 ) );
        m_mediumDamping.setCutoff( mediumCutoff, m_SR );
        
        m_dcBlock.setCutoff( fundamental * 0.1, m_SR );
        
        m_delay.setDelayTimeSamps( m_period );
        if ( m_extendHighFrequencies && m_period < m_SR * MINIMUM_PERIOD )
        {
            auto periodScale = 2;
            while ( m_period * periodScale < m_SR * MINIMUM_PERIOD )
                periodScale += 1;
            initialise_table( m_SR, m_period, periodScale, m_velocity );
        }
        else
        {
            auto periodScale = 1;
            initialise_table( m_SR, m_period, periodScale, m_velocity );
        }
    }
    
    void slurNote( int midiPitch, int midiVelocity )
    {
        m_busyFlag = true;
        m_triggerRelease = false;
        
        m_pitch = midiPitch;
        auto fundamental = midiToFrequency< double >( m_pitch );
        m_period = m_SR /fundamental;
        m_velocity = sjf_scale< double >( static_cast< double >( midiVelocity ) / 127.0, 0, 1, 0.3, 1 );
        
        m_attackDamping.setCutoff( m_attackBright * m_velocity, m_SR );
        
        auto mediumCutoff = std::fmin( 10000.0 , fundamental * ( ( m_mediumBright * 12.0 ) + 4.0 ) );
        m_mediumDamping.setCutoff( mediumCutoff, m_SR );
        
        m_dcBlock.setCutoff( fundamental * 0.1, m_SR );
        
        m_delay.setDelayTimeSamps( m_period );
    }
    
    void setNoteOffReleaseTime( double releaseInMS )
    {
        m_releaseTime = releaseInMS * 0.001 * m_SR;
    }
    
    void triggerNoteOff( )
    {
        if ( m_triggerRelease )
            return;
        m_triggerRelease = true;
        m_triggerAttack = false;
        m_releaseIndex = 0;
    }
    
    double processSample( int indexThroughCurrentBuffer )
    {
        if ( m_triggerRelease  && m_releaseIndex > m_releaseTime )
        {
            m_busyFlag = false;
            m_lastOutSamp = 0;
            return m_lastOutSamp;
        }
        
        m_lastOutSamp = m_delay.getSample2();
        
        if ( m_blend != 1.0 )
        {
            if ( m_blend == 0 )
            {
                m_lastOutSamp *= -1.0;
            }
            else
                m_lastOutSamp *= rand01() > m_blend ? -1.0 : 1;
        }
        
        
        if ( m_triggerRelease  && m_releaseIndex <= m_releaseTime )
        {
            m_lastOutSamp *= static_cast<double>( m_releaseTime - m_releaseIndex ) / static_cast<double>( m_releaseTime );
            m_releaseIndex += 1;
            if ( m_releaseIndex > m_releaseTime )
                m_busyFlag = false;
        }

        m_lastOutSamp = m_drive > 0.0 ? std::tanh( m_lastOutSamp * m_drive ) * m_driveComp : m_lastOutSamp;
        
        m_lastOutSamp = m_mediumDamping.process( m_dcBlock.process( m_lastOutSamp) );
        m_delay.setSample2( m_lastOutSamp );
        
        if ( m_triggerAttack )
        {
            auto amp = attackEnvelope();
            m_lastOutSamp *= amp;
            if ( m_attackIndex >= m_attackSamps )
                m_triggerAttack = false;
        }
        return m_lastOutSamp;
    }
    
    void setAttackTime( double attInSamps )
    {
        m_attackSamps = attInSamps;
    }
    
    int getCurrentPitch()
    {
        return m_pitch;
    }
    
    void setDrive( double drive )
    {
#ifndef NDEBUG
        assert( drive >= 0 && drive <= 100 );
#endif
        m_drive = std::abs( drive * 0.01 );
        
        m_driveComp = m_drive == 0 ? 0 : 1.0 / std::tanh( m_drive );
    }
    
    void setMediumBrightness( double bright )
    {
#ifndef NDEBUG
        assert( bright >= 0 && bright <= 100 );
        DBG( " ifndef " << bright );
#endif
        m_mediumBright = bright * 0.01;
        DBG( " m_mediumBright " << m_mediumBright );
    }
    
    void setAttackBrightness( double bright )
    {
#ifndef NDEBUG
        assert( bright >= 0 && bright <= 100 );
        DBG( "att bright ifndef " << bright );
#endif
        m_attackBright = std::fmin( 10000.0, 250.0 * std::pow( 2.0, bright * 0.01 * 6.322 ) );
        DBG( "m_attackBright " << m_attackBright );
    }
    
    void setBlend( double blend )
    {
#ifndef NDEBUG
        assert( blend >= 0 && blend <= 100 );
        DBG( "att bright ifndef " << blend );
#endif
        m_blend = blend * 0.01;
    }
    
    void shouldExtendHighFrequencies( bool trueIfExtendHighs )
    {
        m_extendHighFrequencies = trueIfExtendHighs;
    }
    
    
    void shouldRandomiseWavetable( bool trueIfRandomWavetable )
    {
        m_randomWavetable = trueIfRandomWavetable;
    }
    
    
    bool isBusy()
    {
        return m_busyFlag;
    }
    
    
private:
    double attackEnvelope( )
    {
        auto amp = (m_attackSamps > 0) && (m_attackIndex < m_attackSamps) ? (m_attackIndex / m_attackSamps) : 1;
        m_attackIndex += 1;
        return amp;
    }
    void initialise_table( double sampleRate, int period, int periodScale, double amplitude )
    {
        std::vector< double > bits;
        bits.resize( period );
        if ( m_randomWavetable )
        {
            static constexpr auto longsize = sizeof( long ) * CHAR_BIT;
            auto pos = 0;
            std::bitset<  longsize  > randNum;
            while ( pos < period )
            {
                randNum = std::rand();
                for( auto i = 0; i < randNum.size(); i++ )
                {
                    auto val = randNum[ i ] ? amplitude : -amplitude;
                    bits[ pos ] = val;
                    pos += 1;
                    if ( pos == period )
                        break;
                }
            }
        }
        else
        {
            for ( auto i = 0; i < bits.size(); i++ )
                bits[ i ] = amplitude;
        }
        
        for ( auto i = 0; i < period * periodScale; i++ )
        {
            auto val = m_attackDamping.process( bits[ fastMod( i, period ) ] );
            m_delay.setSample( i, val );
        }
        m_delay.updateBufferPosition( period );
    }

    sjf_delayLine< double > m_delay;
    sjf_onepole< double > m_attackDamping, m_mediumDamping;
    sjf_threePointHighPass< double > m_dcBlock;
    double m_SR = 44100, m_period = ( m_SR / 220 ), m_velocity = 0.7, m_sustain = 1, m_mediumBright = 1, m_attackBright = 20000.0;
    double m_att = 0, m_dec = 1, m_sus = 0, m_rel = 0;
    double m_attackSamps = 0, m_attackIndex = 0;
    double m_lastOutSamp = 0, m_attackMin = 0;
    double m_blend = 1;
    double m_drive = 0, m_driveComp = 0;
    int m_pitch = 0, m_releaseTime = m_SR, m_releaseIndex = 0;
    
    bool m_triggerRelease = false, m_extendHighFrequencies = true, m_randomWavetable = true, m_triggerAttack = false, m_busyFlag = false;
};


#endif /* sjf_karplusStrong_h */
