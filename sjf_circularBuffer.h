//
//  sjf_circularBuffer_h
//
//  Created by Simon Fay on 07/12/2023.
//

#ifndef sjf_circularBuffer_h
#define sjf_circularBuffer_h

#include <limits>
#include "sjf_interpolators.h"
#include "sjf_audioUtilitiesC++.h"
template< typename T >
class sjf_circularBuffer
{
    // uses bit mask as per Pirkle designing audio effect plugins p. 400;
public:
    sjf_circularBuffer()
    {
        clear();
    }
    ~sjf_circularBuffer(){}
    
    void initialise( int sizeInSamps )
    {
        m_size = sjf_nearestPowerAbove( sizeInSamps, 2 );
        m_wrapMask = m_size - 1;
        m_buffer.resize( m_size );
        clear();
    }
    
    void setSample( T samp )
    {
        m_buffer[ m_writePos ] = samp;
        m_writePos++;
        m_writePos &= m_wrapMask;
    }
    
    T getSample( T delayInSamps )
    {
        T findex = m_writePos - delayInSamps;
        findex = findex < 0 ? findex + m_size : findex;
        T x0, x1, x2, x3, mu;
        auto ind1 = static_cast< long >( findex );
        mu = findex - ind1;
        x0 = m_buffer[ ( (ind1-1) & m_wrapMask ) ];
        x1 = m_buffer[ ind1 ];
        x2 = m_buffer[ ( (ind1+1) & m_wrapMask ) ];
        x3 = m_buffer[ ( (ind1+2) & m_wrapMask ) ];
        
//        { linear = 1, cubic, pureData, fourthOrder, godot, hermite, allpass };
        switch ( m_interpType ) {
            case sjf_interpolators::interpolatorTypes::linear :
                return sjf_interpolators::linearInterpolate( mu, x1, x2 );
                break;
            case sjf_interpolators::interpolatorTypes::cubic :
                return sjf_interpolators::cubicInterpolate( mu, x0, x1, x2, x3 );
                break;
            case sjf_interpolators::interpolatorTypes::pureData :
                return sjf_interpolators::fourPointInterpolatePD( mu, x0, x1, x2, x3 );
                break;
            case sjf_interpolators::interpolatorTypes::fourthOrder :
                return sjf_interpolators::fourPointFourthOrderOptimal( mu, x0, x1, x2, x3 );
                break;
            case sjf_interpolators::interpolatorTypes::godot :
                return sjf_interpolators::cubicInterpolateGodot( mu, x0, x1, x2, x3 );
                break;
            case sjf_interpolators::interpolatorTypes::hermite :
                return sjf_interpolators::cubicInterpolateHermite( mu, x0, x1, x2, x3 );
                break;
            default:
                return sjf_interpolators::linearInterpolate( mu, x1, x2 );
                break;
        }
        
        return sjf_interpolators::fourPointInterpolatePD ( mu, x0, x1, x2, x3 );
    }
    
    T getSample( size_t delayInSamps )
    {
        auto index = m_writePos - delayInSamps;
        return m_buffer[ ( index & m_wrapMask ) ];
    }
    
    void clear()
    {
        std::fill( m_buffer.begin(), m_buffer.end(), 0 );
    }
    
    auto getSize()
    {
        return m_buffer.size();
    }
    
    void setInterpolationType( int interpType )
    {
        m_interpType = interpType;
    }
    
private:
    std::vector< T > m_buffer;
    unsigned long m_writePos = 0;
    unsigned long m_size = sjf_nearestPowerAbove( 44100, 2 );
    unsigned long m_wrapMask = m_size - 1;
    int m_interpType = sjf_interpolators::interpolatorTypes::pureData;
};

// multivoice circular buffer
// each voice is the same length and stored in one vector
template < typename T, int NVOICES >
class sjf_multiVoiceCircularBuffer
{
private:
    std::vector< T > m_buffer;
    std::array< T*, NVOICES > m_voicePtrs;
    size_t m_voiceSize = 0;
    size_t m_writePos = 0;
    size_t m_wrapMask = 0;
    int m_interpType = sjf_interpolators::interpolatorTypes::pureData;
    static constexpr T m_eps = std::numeric_limits<T>::epsilon();
public:
    sjf_multiVoiceCircularBuffer(){}
    ~sjf_multiVoiceCircularBuffer(){}
    
    void initialise( T maxLengthPerVoiceInSamps )
    {
        m_voiceSize = sjf_nearestPowerAbove( maxLengthPerVoiceInSamps, 2.0 );
        m_buffer.resize( NVOICES * m_voiceSize );
        std::fill( m_buffer.begin(), m_buffer.end(), 0 );
        m_wrapMask = m_voiceSize - 1;
        for ( auto i = 0; i < NVOICES; i++ )
            m_voicePtrs[ i ] = m_buffer.data() + ( i * m_voiceSize );
    }
    
    
    T getSample( T delayInSamps, size_t voiceNum )
    {
        auto ptrToFirstIndexOfVoice = m_voicePtrs[ voiceNum ];
        T findex = m_writePos - delayInSamps;
        findex = findex < 0 ? findex + m_voiceSize : findex;
        T x0, x1, x2, x3, mu;
        auto ind1 = static_cast< long >( findex );
        mu = findex - ind1;
        if ( mu <= m_eps )
            return ptrToFirstIndexOfVoice[ ind1 ];
        x0 = ptrToFirstIndexOfVoice[ ( (ind1-1) & m_wrapMask ) ];
        x1 = ptrToFirstIndexOfVoice[ ind1 ];
        x2 = ptrToFirstIndexOfVoice[ ( (ind1+1) & m_wrapMask ) ];
        x3 = ptrToFirstIndexOfVoice[ ( (ind1+2) & m_wrapMask ) ];
        
//        { linear = 1, cubic, pureData, fourthOrder, godot, hermite, allpass };
        switch ( m_interpType )
        {
            case sjf_interpolators::interpolatorTypes::linear :
                return sjf_interpolators::linearInterpolate( mu, x1, x2 );
                break;
            case sjf_interpolators::interpolatorTypes::cubic :
                return sjf_interpolators::cubicInterpolate( mu, x0, x1, x2, x3 );
                break;
            case sjf_interpolators::interpolatorTypes::pureData :
                return sjf_interpolators::fourPointInterpolatePD( mu, x0, x1, x2, x3 );
                break;
            case sjf_interpolators::interpolatorTypes::fourthOrder :
                return sjf_interpolators::fourPointFourthOrderOptimal( mu, x0, x1, x2, x3 );
                break;
            case sjf_interpolators::interpolatorTypes::godot :
                return sjf_interpolators::cubicInterpolateGodot( mu, x0, x1, x2, x3 );
                break;
            case sjf_interpolators::interpolatorTypes::hermite :
                return sjf_interpolators::cubicInterpolateHermite( mu, x0, x1, x2, x3 );
                break;
            default:
                return sjf_interpolators::linearInterpolate( mu, x1, x2 );
                break;
        }
    }
    
    T getSample( size_t delayInSamps, size_t voiceNum )
    {
        auto index = m_writePos - delayInSamps;
        index &= m_wrapMask;
        return m_voicePtrs[ voiceNum ][ index ];
    }
    
    void setSample( T input, size_t voiceNum )
    {
        m_voicePtrs[ voiceNum ][ m_writePos ] = input;
    }
    
    void setInterpolationType( int interpType )
    {
        m_interpType = interpType;
    }
    
    void updateWritePosition( )
    {
        m_writePos += 1;
        m_writePos &= m_wrapMask;
    }
    
};
#endif
