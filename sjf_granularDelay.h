//
//  sjf_granularDelay.h
//
//  Created by Simon Fay on 04/10/2023.
//

#ifndef sjf_granularDelay_h
#define sjf_granularDelay_h

#include <vector>
#include <JuceHeader.h>
#include "sjf_audioUtilities.h"
#include "sjf_interpolators.h"
#include "sjf_wavetables.h"
#include "../sjf_audio/sjf_phasor.h"
#include "../sjf_audio/sjf_audioUtilities.h"


//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------

// Basic algorithm seems to work
// TO DO
//      - correct phasor rate and grain trigger calculations
//          -- phasor rate should be calculated as one cycle for all voices to be triggered

template < int NCHANNELS >
class sjf_gdVoice
{
public:
    sjf_gdVoice(){}
    ~sjf_gdVoice(){}
    
    void triggerNewGrain( size_t writePos, bool rev, bool play, float dt, float transpos, float grainSize )
    {
        m_writePos = writePos;
        m_reverseFlag = rev;
        m_shouldPlayFlag = play;
        m_delayTimeSamps = dt;
        m_transposition = transpos;
        m_grainSizeSamps = grainSize;
        m_delayPitchCompensation = m_transposition * m_grainSizeSamps;
        m_sampleCount = 0;
    }
    
    void process( std::array< float, NCHANNELS >& samples, std::array< std::vector< float >, NCHANNELS >& buffers )
    {
        if ( m_sampleCount >= m_grainSizeSamps || !m_shouldPlayFlag )
            return;
        
        auto grainPhase = static_cast< float >( m_sampleCount ) / static_cast< float >( m_grainSizeSamps );
        
        auto readPos = m_writePos - m_delayTimeSamps;
//        readPos += m_sampleCount;
        if ( m_reverseFlag )
            readPos += m_delayPitchCompensation - ( m_delayPitchCompensation * grainPhase );
        else
            readPos += ( m_delayPitchCompensation * grainPhase );
//
        auto delBufferSize = buffers[ 0 ].size();
        fastMod3< float >( readPos, delBufferSize );
        auto win = m_win.getValue( m_windowSize * grainPhase );
        auto pos1 = static_cast< int >( readPos );
        auto mu = readPos - ( static_cast< float >( pos1 ) );
        auto pos0 = fastMod4< int >( pos1 - 1, static_cast< int >( delBufferSize ) );
        auto pos2 = fastMod4< int >( pos1 + 1, static_cast< int >( delBufferSize ) );
        auto pos3 = fastMod4< int >( pos2 + 1, static_cast< int >( delBufferSize ) );
        for ( auto c = 0; c < NCHANNELS; c++ )
            samples[ c ] += sjf_interpolators::fourPointInterpolatePD( mu, buffers[ c ][ pos0 ], buffers[ c ][ pos1 ], buffers[ c ][ pos2 ], buffers[ c ][ pos3 ]) * win;
        m_sampleCount++;
    }
    
private:
    
    size_t m_writePos = 0;
    bool m_reverseFlag = false;
    bool m_shouldPlayFlag = false;
    float m_delayTimeSamps = 11025;
    float m_transposition = 1;
    float m_grainSizeSamps = 11025;
    float m_delayPitchCompensation = m_transposition * m_grainSizeSamps;
    int m_sampleCount = 0;
    
    static constexpr float m_windowSize = 1024;
    static constexpr sjf_hannArray< float, static_cast< int >( m_windowSize ) > m_win;
    
    
};



//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------

template < int NVOICES >
class sjf_granularDelay
{
public:
    sjf_granularDelay()
    {
#ifndef NDEBUG
        assert ( NVOICES % 2 == 0 ); // ensure that number of voices is a multiple of 2 for crossfading
#endif
        initialise( m_SR );
        
//        for (auto & samp : m_lastSamples )
//            samp = 0.0f;
    }
    ~sjf_granularDelay(){}
    
    void initialise( double sampleRate )
    {
        m_SR = sampleRate;
        
        for ( auto & buf : m_buffers )
        {
            buf.resize( m_SR * 10 );
            for ( auto & s : buf )
                s = 0;
        }
//        for (auto & samp : m_lastSamples )
//            samp = 0.0f;
        
        m_sampleCount = 0;
        
        m_deltaTimeSamps = std::round( m_SR / m_rateHz );
    }
    
    
    void process( juce::AudioBuffer<float>& buffer )
    {
        

        auto numOutChannels = buffer.getNumChannels();
        auto blockSize = buffer.getNumSamples();
        
        std::array< float, NCHANNELS > samples, samplesCross;

//        auto voicePhase = 0.0f;
        for ( int indexThroughBuffer = 0; indexThroughBuffer < blockSize; indexThroughBuffer++ )
        {
            m_writePos = fastMod( m_writePos, m_buffers[ 0 ].size() );
            for ( auto & s : samples )
                s = 0;
            if ( m_sampleCount >= m_deltaTimeSamps )
            {
                m_lastVoiceTriggered++;
                m_lastVoiceTriggered = fastMod4< int >( m_lastVoiceTriggered, NVOICES );
                auto dt = m_delayTimeSamps + ( m_delayTimeSamps * rand01() * m_delayTimeJitter );
                bool rev = rand01() < m_reverseChance ? true : false;
                auto pJit = ( rand01()* 2.0f - 1 ) * m_transpositionJitter;
                auto tr = std::pow( 2, m_transposition + pJit );
                bool shouldPlay = m_density >= rand01() ? true : false;
                auto grainSize = m_deltaTimeSamps * 2.0;
                DBG( "rate " << m_rateHz );
                DBG( "grainSize " << grainSize );
                DBG( "NVOICES " << NVOICES << " this voice " << m_lastVoiceTriggered );
                DBG(" ");
                m_delays[ m_lastVoiceTriggered ].triggerNewGrain( m_writePos, rev, shouldPlay, dt, tr, grainSize );
                m_sampleCount = 0;
            }
            
            // run through each grain channel and add output to samples
            for ( auto &  v : m_delays )
                v.process( samples, m_buffers );
            
            for ( auto & buf : m_buffers )
                buf[ m_writePos ] = 0;
            
            if ( m_crosstalk > 0 )
            {
                samplesCross[ 0 ] = ( samples[ 0 ] * m_crossFadeLevels[ 0 ] ) + ( samples[ 1 ] * m_crossFadeLevels[ 1 ] );
                samplesCross[ 1 ] = ( samples[ 1 ] * m_crossFadeLevels[ 0 ] ) + ( samples[ 0 ] * m_crossFadeLevels[ 1 ] );
                for ( auto c = 0; c < NCHANNELS; c++ )
                {
                    auto bufChan = fastMod4< int >( c, numOutChannels );
                    auto inSamp = buffer.getSample( bufChan, indexThroughBuffer );
                    m_buffers[ c ][ m_writePos ] += inSamp + ( samplesCross[ c ] * m_feedback );
                    auto outSamp = ( samples[ c ] * m_wet ) + ( inSamp * m_dry );
                    buffer.setSample( bufChan, indexThroughBuffer, outSamp );
                }
            }
            else
            {
                for ( auto c = 0; c < NCHANNELS; c++ )
                {
                    auto bufChan = fastMod4< int >( c, numOutChannels );
                    auto inSamp = buffer.getSample( bufChan, indexThroughBuffer );
                    m_buffers[ c ][ m_writePos ] += inSamp + ( samples[ c ] * m_feedback );
                    auto outSamp = ( samples[ c ] * m_wet ) + ( inSamp * m_dry );
                    buffer.setSample( bufChan, indexThroughBuffer, outSamp );
                }
            }

            m_writePos++;
            
            m_sampleCount++;
        }
    }
    
    
    void setRate( float rate )
    {
        m_rateHz = rate; // / static_cast< float >( NVOICES );
        m_deltaTimeSamps = std::round( m_SR / m_rateHz );
    }
    
    void setFeedback( float fbPercentage )
    {
        m_feedback = fbPercentage * 0.01f;
    }
    
    void setCrossTalk( float ct )
    {
        m_crosstalk = ct * 0.01f;
        
        m_crossFadeLevels[ 0 ] = 0.5 + ( 0.5 * std::cos( m_crosstalk * M_PI ) );
        m_crossFadeLevels[ 1 ] = 0.5 + ( 0.5 * std::cos( ( 1.0 - m_crosstalk ) * M_PI ) );
        
    }
    
    void setDensity( float densityPercentage )
    {
        m_density = densityPercentage * 0.01f;
    }
    
    void setDelayTimeSamps( float dt )
    {
        m_delayTimeSamps = dt;
    }
    
    void setDelayTimeJitter( float dtJitPercentage )
    {
        m_delayTimeJitter = dtJitPercentage * 0.01f;
    }
    
    void setTransposition( float trSemitones )
    {
        m_transposition = trSemitones / 12.0f;
    }
    
    void setTranspositionJitter( float trJitPercentage )
    {
        m_transpositionJitter = trJitPercentage * 0.01f;
    }
    
    void setReverseChance( float revPercentage )
    {
        m_reverseChance = revPercentage * 0.01f;
    }
    
    void setMix( float wetPercentage )
    {
        auto wet = wetPercentage * 0.01;
        m_wet = wet > 0.0f ? std::sqrt( wet ) : 0;
        auto dry = 1.0f - wet;
        m_dry = dry > 0.0f ? std::sqrt( dry ) : 0;
    }
private:
    static constexpr int NCHANNELS = 2;
    
    
    std::array< sjf_gdVoice< NCHANNELS >, NVOICES > m_delays;
    std::array< std::vector< float >, NCHANNELS > m_buffers;
    
    size_t m_writePos = 0;
    
    
    
    float m_rateHz = 1; // num grains triggered per second
    float m_feedback = 0;
    float m_crosstalk = 0;
    float m_density = 1;
    float m_delayTimeSamps = 22050;
    float m_delayTimeJitter = 0;
    float m_reverseChance = 0.25;
    float m_transposition = 0;
    float m_transpositionJitter = 0.25;
    float m_dry = 0;
    float m_wet = 1;
    float m_SR = 44100;
    
    
    std::array< float, NCHANNELS > m_crossFadeLevels;
    
    int m_lastVoiceTriggered = -1;
    long long m_sampleCount = 0;
    long long m_deltaTimeSamps = std::round( m_SR / m_rateHz );
};

#endif /* sjf_granularDelay_h */
