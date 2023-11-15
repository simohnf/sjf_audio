//
//  sjf_sola_h
//
//  Created by Simon Fay on 28/11/2022.
//

#ifndef sjf_sola_h
#define sjf_sola_h


#include "sjf_audioUtilities.h"
template< typename T, int NCHANNELS >
class sjf_sola
{
public:
    sjf_sola(){}
    ~sjf_sola(){}
    
    void stretch( std::array< std::vector< T >, NCHANNELS > &sampleToStretch, T stretchFactor )
    {
        auto originalSize = sampleToStretch[ 0 ].size();
        // probably should think of a way of only ever increasing the size of the vector
        for ( auto & v : m_solaVect )
            v.resize( originalSize * stretchFactor );
            
        auto Sanalysis = std::min( m_hopSize,  static_cast< long long >( m_segmentSize / ( stretchFactor * 2 ) ) );
        auto Sstretch = static_cast< long long >(Sa * stretchFactor);
        auto overlapInterval = static_cast< long long >(Sstretch / 2);
        auto norm = 1.0 / static_cast< T >( overlapInterval );
//        std::vector< long long > segStarts, timeLag;
        std::vector< float > fade;
        fade.reserve( overlapInterval );
        for ( auto i = 0; i < overlapInterval; i++ )
            fade.emplace_back( 0.5 - ( 0.5 * std::cos( 2.0 * M_PI * ( static_cast< float >( i ) / static_cast< float >( (overlapInterval-1) * 2 ) ) ) ) );
        
    }
    
    void setHopSize( long long hop )
    {
#ifndef NDEBUG
        assert( hop > 0 );
#endif
        m_hopSize = hop;
    }
    
    void setSegmentSize( long long segmentSize )
    {
#ifndef NDEBUG
        assert( segmentSize > 0 );
#endif
        m_segmentSize = segmentSize;
    }
private:
    long long m_segmentSize = 2048, m_hopSize = 256;
    std::array< std::vector< T >, NCHANNELS > m_solaVect;
}

#endif /* sjf_sola_h */

