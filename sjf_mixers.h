//
//  sjf_mixers.h
//  sjf_verb
//
//  Created by Simon Fay on 04/06/2024.
//  Copyright © 2024 sjf. All rights reserved.
//

#ifndef sjf_mixers_h
#define sjf_mixers_h

//===================================//===================================//===================================
//===================================//===================================//===================================
//===================================//===================================//===================================
//===================================//===================================//===================================

namespace  sjf::mixers
{
    /** only for consitency with other mixers */
    template< typename Sample >
    struct None
    {
        None( const size_t size ) { }
        inline void inPlace( Sample* data, const size_t size ) const { return; }
    };
//===================================
    // copied from Geraint Luff
    template< typename Sample >
    struct Hadamard
    {
        Hadamard( const size_t size ) : m_scalingFactor( std::sqrt(1.0/static_cast<Sample>(size) ) ) {}
        
        inline void inPlace(Sample* data, const size_t size ) const 
        {
            recursiveUnscaled( data, size );
            for (int c = 0; c < size; ++c) {
                data[c] *= m_scalingFactor;
            }
        }
        
    private:
        
        inline void recursiveUnscaled(Sample * data, const size_t size ) const
        {
            if (size <= 1) return;
            const auto hSize = size/2;
            
            // Two (unscaled) Hadamards of half the size
            recursiveUnscaled( data, hSize );
            recursiveUnscaled(data + hSize, hSize);
            
            // Combine the two halves using sum/difference
            for (auto i = 0; i < hSize; ++i) {
                double a = data[i];
                double b = data[i + hSize];
                data[i] = (a + b);
                data[i + hSize] = (a - b);
            }
        }
        const Sample m_scalingFactor{0.7071};
    };

    // copied from Geraint Luff
    template< typename Sample >
    struct Householder
    {
        Householder( const size_t size ) : m_weighting( -2.0f / static_cast< Sample >(size) ) {}
        
        inline void inPlace( Sample* data, const size_t size ) const
        {
            Sample sum = 0.0f; // use this to mix all samples with householder matrix
            for( auto c = 0; c < size; c++ )
                sum += data[ c ];
            sum *= m_weighting; // Householder weighting
            for( auto c = 0; c < size; c++ )
                data[ c ] += sum;
        }
    private:
        Sample m_weighting{1};
    };
}
#endif /* sjf_mixers_h */
