//
//  sjf_compileTimeRandom_h
//
//  Created by Simon Fay on 01/02/2023.
//

#ifndef sjf_compileTimeRandom_h
#define sjf_compileTimeRandom_h

// copied from Jason Turner https://godbolt.org/g/zbWvXK

#include <cstdint>
#include <limits>
#include "gcem/include/gcem.hpp"
#include "_compileTimeUnixTime.h"
namespace sjf_compileTimeRandom
{
    constexpr auto seed()
    {
        std::uint64_t shifted = 0;
        
        for( const auto c : __TIME__ )
        {
            shifted <<= 8;
            shifted |= c;
        }
        
        return shifted;
    }
    
    struct PCG
    {
        struct pcg32_random_t { std::uint64_t state=0;  std::uint64_t inc=seed(); };
        pcg32_random_t rng;
        typedef std::uint32_t result_type;
        
        constexpr result_type operator()()
        {
            return pcg32_random_r();
        }
        
        static result_type constexpr min()
        {
            return std::numeric_limits<result_type>::min();
        }
        
        static result_type constexpr max()
        {
            return std::numeric_limits<result_type>::min();
        }
        
    private:
        constexpr std::uint32_t pcg32_random_r()
        {
            std::uint64_t oldstate = rng.state;
            // Advance internal state
            rng.state = oldstate * 6364136223846793005ULL + (rng.inc|1);
            // Calculate output function (XSH RR), uses old state for max ILP
            std::uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
            std::uint32_t rot = oldstate >> 59u;
            return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
        }
        
    };
    
    constexpr auto get_random(int count)
    {
        PCG pcg;
        while(count > 0){
            pcg();
            --count;
        }
        
        return pcg() % RAND_MAX;
    }
    

}

namespace sjf::ctr
{

    /**
     xorshift random function for creating random numbers at compile time
     */
    constexpr unsigned long xorshift( unsigned long seed )
    {
        seed ^= (seed << 9);
        seed ^= (seed >> 7);
        seed ^= (seed << 13);
        return seed;
    }
    
    template< typename T, size_t NVALS, unsigned long SEED >
    constexpr void shuffle( std::array< T, NVALS >& arr )
    {
        T temp = 0;
        auto s = SEED;
        for ( auto i = NVALS - 1; i > 1; i-- )
        {
            s = xorshift( SEED );
            auto j = ( s % i );
            temp = arr[ i ];
            arr[ i ] = arr[ j ];
            arr[ j ] = temp;
        }
    }
    /**
     A compile time array of random values between 0 and 1.
     */
    template < typename T, size_t NVALS, unsigned long SEED >
    class rArray
    {
    public:
        constexpr rArray() : m_arr(), m_arr2()
        {
            
            size_t s = ( SEED & RAND_MAX );
            T inv = 1.0 / T( RAND_MAX );
            for ( auto i = 0; i < NVALS; i++ )
            {
                s = xorshift( s );
                m_arr[ i ] = ( ( s & RAND_MAX ) * inv );
            }
            
        }
        
        const T& operator[] ( size_t index ) const { return m_arr[ index ]; }
        
        const unsigned long getSEED() const { return SEED; }
        
        const T& getVal ( size_t index ) const { return m_arr2[ index ]; }
        
    private:
        T m_arr[ NVALS ], m_arr2[ NVALS ];
    };


}


#endif  /* sjf_compileTimeRandom_h */
