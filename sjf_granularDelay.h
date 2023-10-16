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
        if ( m_sampleCount >= m_grainSizeSamps )
            return;
        if ( !m_shouldPlayFlag )
            return;
        
        auto grainPhase = static_cast< float >( m_sampleCount ) / static_cast< float >( m_grainSizeSamps );
        
        auto readPos = 0.0f;
        
        if ( m_reverseFlag )
            readPos = ( ( m_writePos - m_delayTimeSamps ) + m_delayPitchCompensation ) - ( m_delayPitchCompensation * grainPhase );
        else
            readPos = ( ( m_writePos - m_delayTimeSamps ) +  ( m_delayPitchCompensation * grainPhase ) );
        
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

template < int NCHANNELS, int NVOICES >
class sjf_granularDelay
{
public:
    sjf_granularDelay()
    {
        initialise( m_SR );
        for( auto & p : m_lastPhasePositions )
            p = 0;
        
        for (auto & samp : m_lastSamples )
            samp = 0.0f;
    }
    ~sjf_granularDelay(){}
    
    void initialise( double sampleRate )
    {
        m_SR = sampleRate;
        m_phaseRamp.initialise( m_SR, m_rateHz );
        
        for ( auto & buf : m_buffers )
        {
            buf.resize( m_SR * 10 );
            for ( auto & s : buf )
                s = 0;
        }
        for (auto & samp : m_lastSamples )
            samp = 0.0f;
    }
    
    
    void process( juce::AudioBuffer<float>& buffer )
    {
        
        m_writePos = fastMod( m_writePos, m_buffers[ 0 ].size() );
        auto blockSize = buffer.getNumSamples();
        auto voicePhase = 0.0f;
        std::array< float, NCHANNELS > samples;

        for ( int indexThroughBuffer = 0; indexThroughBuffer < blockSize; indexThroughBuffer++ )
        {
            for ( auto & s : samples )
                s = 0;
            auto phasePos = m_phaseRamp.output();
            for ( auto v = 0; v < NVOICES; v++ )
            {
                // cheeck whether a new grain should be triggered
                voicePhase = phasePos * NVOICES;
                voicePhase -= static_cast< float >( v );
                voicePhase = fastMod4< float >( voicePhase, static_cast< float >( NVOICES ) );
                voicePhase *= 0.5;
                if ( voicePhase >= 0 && voicePhase <= 1 && ( m_lastPhasePositions[ v ] < 0 || m_lastPhasePositions[ v ] > 1 ) )
                {
                    auto dt = m_delayTimeSamps + ( rand01() * m_delayTimeJitter );
                    bool rev = rand01() < m_reverseChance ? true : false;
                    auto pJit = ( rand01()* 2.0f - 1 ) * m_transpositionJitter;
                    auto tr = std::pow( 2, m_transposition + pJit );
                    bool shouldPlay = m_density >= rand01() ? true : false;
                    auto grainSize = 2.0 * m_SR / ( m_rateHz / NVOICES ); // doubled grain size for crossfade
                    m_delays[ v ].triggerNewGrain( m_writePos, rev, shouldPlay, dt, tr, grainSize );
                }
                
                // run through each grain channel and add output to samples
                m_delays[ v ].process( samples, m_buffers );
                
                m_lastPhasePositions[ v ] = voicePhase;
            }
            for ( auto & buf : m_buffers )
                buf[ m_writePos ] = 0;
            for ( auto c = 0; c < NCHANNELS; c++ )
            {
                auto inSamp = buffer.getSample( c, indexThroughBuffer );
                m_buffers[ c ][ m_writePos ] += inSamp + ( samples[ c ] * m_feedback );
                auto outSamp = ( samples[ c ] * m_wet ) + ( inSamp * m_dry );
                buffer.setSample( c, indexThroughBuffer, outSamp );
            }
            
            m_writePos++;
            
        }
    }
    
    
    void setRate( float rate )
    {
        m_rateHz = rate;
    }
    
    void setFeedback( float fb )
    {
        m_feedback = fb;
    }
    
    void setCrossTalk( float ct )
    {
        m_crosstalk = ct;
    }
    
    void setDensity( float density )
    {
        m_density = density;
    }
    
    void setDelayTimeSamps( float dt )
    {
        m_delayTimeSamps = dt;
    }
    
    void setDelayTimeJitter( float dtJit )
    {
        m_delayTimeJitter = dtJit;
    }
    
    void setTransposition( float tr )
    {
        m_transposition = tr;
    }
    
    void setTranspositionJitter( float trJit )
    {
        m_transpositionJitter = trJit;
    }
    
    void setReverseChance( float rev )
    {
        m_reverseChance = rev;
    }
    
    void setMix( float wetPercentage )
    {
        m_wet = std::sqrt( wetPercentage * 0.01 );
        m_dry = std::sqrt( 1.0 - ( wetPercentage * 0.01 ) );
    }
private:
    
    
    sjf_phasor< float > m_phaseRamp;
    
    std::array< sjf_gdVoice< NCHANNELS >, NVOICES > m_delays;
    std::array< std::vector< float >, NCHANNELS > m_buffers;
    
    size_t m_writePos = 0;
    
    
    
    std::array< float, NCHANNELS > m_lastSamples;
    std::array< float, NVOICES > m_lastPhasePositions;
    
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
};

#endif /* sjf_granularDelay_h */
