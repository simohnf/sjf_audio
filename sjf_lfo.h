//
//  sjf_lfo.h
//
//  Created by Simon Fay on 17/10/2022.
//

#ifndef sjf_lfo_h
#define sjf_lfo_h
#define PI 3.14159265

#include "/Users/simonfay/Programming_Stuff/sjf_audio/sjf_phaseRateMultiplier.h"
#include "/Users/simonfay/Programming_Stuff/sjf_audio/sjf_audioUtilities.h"
#include "/Users/simonfay/Programming_Stuff/sjf_audio/sjf_triangle.h"
#include "/Users/simonfay/Programming_Stuff/sjf_audio/sjf_noiseOSC.h"
#include "/Users/simonfay/Programming_Stuff/sjf_audio/sjf_lpfFirst.h"

class sjf_lfo
{
public:
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    sjf_lfo() { setBpm( 120 ); };
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~sjf_lfo(){};
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    float output()
    {
        if ( m_isSyncedToTempo )
        {
            // work directly with position if synced to tempo
            m_phase = m_rateMultiplier.rateChange( m_pos );
        }
        else // if Hz based lfo
        {
            // base frequency is always 1hz for simplicity of calculations
            m_phase = m_rateMultiplier.rateChange( m_count / m_SR );
        }
        calculateOutput();
        
        m_lastPhase = m_phase;
        if ( m_isSyncedToTempo ) // work directly with position if synced to tempo
        {
            m_pos += m_increment; // increase position per sample
            if ( m_pos >= 1.0 )
            {
                m_pos -= (int)m_pos; // make sure position is always betweeo 0 --> 1
            }
        }
        m_count++; // if Hz based count is just simply increased by one sample
        while ( m_count >= m_SR )
        {
            m_count -= m_SR;
        }
        
        if ( m_lfoType == noise1 )
        { // smooth out random changes slightly
            return lpf.filterInput ( m_out ) + m_offset;
        }
        return m_out + m_offset;
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void setSampleRate( float sr )
    {
        m_SR = sr;
        lpf.setCutoff( sin( 20.0f * 2.0f * 3.141593f / m_SR ) );
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void setLFOtype( int type )
    {
        m_lfoType = type;
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void setTriangleDuty( float d )
    {
        m_triangle.setDuty( d );
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void setRateChange( float r )
    {
        m_rateMultiplier.setRate( r );
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void setSyncDivision ( int div )
    {
        if (div == m_syncVal)
        {
            return;
        }
        m_syncVal = div;
        float rate;
        switch( div )
        {
            case eightWholeNotes:
                rate = 8.0 / 8.0;
                break;
            case sevenWholeNotes:
                rate = 7.0 / 8.0;
                break;
            case sixWholeNotes:
                rate = 6.0 / 8.0;
                break;
            case fiveWholeNotes:
                rate = 5.0 / 8.0;
                break;
            case fourWholeNotes:
                rate = 4.0 / 8.0;
                break;
            case threeWholeNotes:
                rate = 3.0 / 8.0;
                break;
            case twoWholeNotes:
                rate = 2.0 / 8.0;
                break;
            case sevenQuarterNotes:
                rate = 7.0 / ( 8.0 * 4.0 );
                break;
            case dottedWholeNote:
                rate = 6.0 / ( 8.0 * 4.0 );
                break;
            case fiveQuarterNotes:
                rate = 5.0 / ( 8.0 * 4.0 );
                break;
            case oneWholeNote:
                rate = 1.0 / 8.0;
                break;
            case sevenEightNotes:
                rate = 7.0 / ( 8.0 * 8.0 );
                break;
            case dottedHalfNote:
                rate = 6.0 / ( 8.0 * 8.0 );
                break;
            case fiveEightNotes:
                rate = 5.0 / ( 8.0 * 8.0 );
                break;
            case oneHalfNote:
                rate = 4.0 / ( 8.0 * 8.0 );
                break;
            case dottedQuarterNote:
                rate = 3.0 / ( 8.0 * 8.0 );
                break;
            case oneQuarterNote:
                rate = 2.0 / ( 8.0 * 8.0 );
                break;
            case dottedEightNote:
                rate = 1.5 / ( 8.0 * 8.0 );
                break;
            case oneEightNote:
                rate = 1.0 / ( 8.0 * 8.0 );
                break;
            case wholeNoteTriplet:
                rate = 2.0 /( 3.0 * 8.0 );
                break;
            case halfNoteTriplet:
                rate = 0.5 * 2.0 / ( 3.0 * 8.0 );
                break;
            case quarterNoteTriplet:
                rate = 0.25 * 2.0 / ( 3.0 * 8.0 );
                break;
            default:
                rate = 0.5;
                break;
        }
        DBG("RATE " << rate );
        m_rateMultiplier.setRate( rate );
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void setSyncDivision( const int& nBeats, const int& beatName, const int& beatType )
    {
        float rate = 1.0f;
        switch ( beatType )
        {
            case tuplet:
                break;
            case dotted:
                rate *= 1.5f;
            case triplet:
                rate *= 2.0f;
                rate /=3.0f;
                break;
            case quintuplet:
                rate *= 4.0f;
                rate /= 5.0f;
                break;
            case septuplet:
                rate *= 4.0f;
                rate /= 7.0f;
                break;
            default:
                break;
        }
        switch ( beatName )
        {
            case wholeNote:
                rate /= 8.0f;
                break;
            case halfNote:
                rate /= 16.0f;
                break;
            case quarterNote:
                rate /= 32.0f;
                break;
            case eightNote:
                rate /= 64.0f;
                break;
            case sixteenthNote:
                rate /= 128.0f;
                break;
            default:
                rate /= 32.0f;
        }
        rate *= nBeats;
        m_rateMultiplier.setRate( rate );
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void setOffset( float o )
    {
        m_offset = o;
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void setPosition( float pos )
    {
        // ensure sync to tempo at start of each block
        // work directly with phase if synced to tempo
        // each phase loop is synced to a quarter note
        if( m_isSyncedToTempo )
        {
            pos *= m_syncFactor;  // longest possible length when synced is 32 quarter notes
            m_pos = pos - (int)pos; // just get fractional part
            DBG( "m_pos " << m_pos );
        }
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void setBpm( float bpm )
    {
        if ( m_bpm != bpm )
        {
            m_bpm = bpm;
            float period = m_maxSyncBeats * 60.0f / m_bpm; // period in seconds // max length is 32 beats
            float sampsperperiod = period * m_SR; // number of samples in one period
            m_increment = 1.0f / sampsperperiod; // increment per sample when synced to tempo
        }
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void isSyncedToTempo( bool isSyncedToTempo )
    {
        m_isSyncedToTempo = isSyncedToTempo;
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    enum lfoType
    {
        sine = 1, triangle, noise1, noise2, sah
    };
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    enum syncDivisions
    {
        eightWholeNotes = 1, sevenWholeNotes, sixWholeNotes, fiveWholeNotes, fourWholeNotes, threeWholeNotes, twoWholeNotes, sevenQuarterNotes, dottedWholeNote, fiveQuarterNotes, oneWholeNote, sevenEightNotes, dottedHalfNote, wholeNoteTriplet, fiveEightNotes, oneHalfNote, dottedQuarterNote, halfNoteTriplet, oneQuarterNote, dottedEightNote, quarterNoteTriplet, oneEightNote
    };
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    enum beatNames
    {
        wholeNote = 1, halfNote, quarterNote, eightNote, sixteenthNote 
    };
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    enum beatTypes
    {
        tuplet = 1, dotted, triplet, quintuplet, septuplet
    };
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    
    
private:
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void setInvertedRate( float invRate )
    {
        m_rateMultiplier.setInvertedRate( invRate );
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void calculateOutput()
    {
        switch( m_lfoType )
        {
            case sine:
                m_out = sin( 2*PI*m_phase );
                break;
            case triangle:
                m_out = m_triangle.output( m_phase );
                break;
            case noise1:
                if ( m_phase < m_lastPhase * 0.5 ) { m_out = ( rand01() * 2.0f ) -1.0f; }
                break;
            case noise2:
                m_out = m_noise2.output( m_phase );
                break;
            case sah:
                // put sah logic in here
                break;
            default:
                m_out = sin( 2*PI*m_phase );
                break;
        }
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    float m_count = 0, m_SR = 44100.0f, m_phase = 0.0f, m_out = 0.0f, m_lastPhase = 1.0f, m_offset = 0.0f, m_bpm = 0, m_pos, m_increment;
    const float m_maxSyncBeats = 32.0f;
    const float m_syncFactor = 1.0f/m_maxSyncBeats;
    bool m_isSyncedToTempo = false;
    int  m_lfoType = 0, m_syncVal = -1;
    sjf_phaseRateMultiplier m_rateMultiplier, m_sahRateMultiplier;
    sjf_triangle m_triangle;
    sjf_noiseOSC m_noise2;
    sjf_lpfFirst lpf; // just for safety
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( sjf_lfo )
};
#endif /* sjf_lfo_h */

