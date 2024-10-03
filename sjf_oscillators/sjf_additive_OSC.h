//
//  sjf_additive_OSC.h
//
//  Created by Simon Fay on 04/08/2024.
//

#ifndef sjf_additive_OSC_h
#define sjf_additive_OSC_h
#include "sjf_phasor.h"
#include "../sjf_mathsApproximations.h"

namespace sjf::oscillators
{
    /** Generalised additive synthesis oscillator */
    template < typename Sample, size_t MAXNHARMS >
    class additiveOsc
    {
    public:
        
        /** set the number of harmonics used in additive synthesis */
        void setNumSinWaves( int nSinWaves )
        {
            assert( nSinWaves > 0 );
            assert( nSinWaves <= MAXNHARMS );
            m_nSinWaves = nSinWaves;
//            m_freqMults.resize(m_nSinWaves, 0);
//            m_amps.resize(m_nSinWaves, 0);
        }
        
        /** set the amplitude of an individual sinwave*/
        void setAmplitude( int num, Sample amp )
        {
            assert( amp >= 0 );
            assert( amp <= 1 );
            assert( num >= 0 );
            assert( num < m_amps.size() );
            m_amps[ num ] = amp;
        }
        
        /** set the relative frequency multiple of an indiviual sin wave */
        void setFrequencyMultiple( int num, Sample mult )
        {
            assert( mult > 0 );
            assert( num >= 0 );
            assert( num < m_freqMults.size() );
            m_freqMults[ num ] = mult;
        }
        
        void setType( oscType type, Sample SCALE = 1 )
        {
            switch (type) {
                case oscType::sin:
                    m_freqMults[ 0 ] = 1;
                    m_amps[ 0 ] = 1 * SCALE;
                    for ( auto i = 1; i < MAXNHARMS; ++i )
                    {
                        m_freqMults[ i ] = 0;
                        m_amps[ i ] = 0;
                    }
                    return;
                case oscType::saw:
                    for ( auto i = 0; i < MAXNHARMS; ++i )
                    {
                        m_freqMults[ i ] = m_sawFreqs[ i ];
                        m_amps[ i ] = m_sawAmps[ i ] * SCALE;
                    }
                    return;
                case oscType::square:
                    for ( auto i = 0; i < MAXNHARMS; ++i )
                    {
                        m_freqMults[ i ] = m_sqrFreqs[ i ];
                        m_amps[ i ] = m_sqrAmps[ i ] * SCALE;
                    }
                    return;
                case oscType::triangle:
                    for ( auto i = 0; i < MAXNHARMS; ++i )
                    {
                        m_freqMults[ i ] = m_triFreqs[ i ];
                        m_amps[ i ] = m_triAmps[ i ] * SCALE;
                    }
                    return;
                default:
                    break;
            }
        }
        /** process an individual sample */
        Sample process( Sample phase )
        {
            auto fp = phase;
            int ip = 0;
            Sample output =  0;
            for ( auto i = 0; i < m_nSinWaves; ++i )
            {
                fp = phase * m_freqMults[ i ];
                ip = fp;
                fp -= ip; // only frac part
                fp *= 2;
                fp = fp < 1 ? fp : 0.0 - (fp - 1.0);
                fp *= M_PI; // radians
//                output += std::sin( fp ) * m_amps[ i ];
                output += sjf::maths::sinApprox( fp ) * m_amps[ i ];
            }
            return output;
        }
    private:
        template< int SIZE, typename FUNCTOR >
        struct waveTable
        {
            constexpr waveTable() : m_table()
            {
                for ( auto i = 0; i < SIZE; ++i )
                    m_table[ i ] = func(i);
            }
            const Sample operator[]( const size_t index ) const
            {
                return m_table[ index ];
            }
        private:
            Sample m_table[ SIZE ];
            FUNCTOR func;
        };
        
        //==========//==========//==========//==========//==========//==========//==========//==========
        //==========//==========//==========//==========//==========//==========//==========//==========
        //==========//==========//==========//==========//==========//==========//==========//==========
        //==========//==========//==========//==========//==========//==========//==========//==========
        /* Sawtooth tables */
        struct sawFreqMult
        { /* ALL HARMONICS */
            constexpr Sample operator()( const size_t index ) const
            { return (index + 1.0); }
        };
        struct sawAmp
        {
            constexpr Sample operator()( const size_t index ) const
            { return 1.0 / static_cast< Sample >( index + 1 ); }
        };
        //==========//==========//==========//==========//==========//==========//==========//==========
        //==========//==========//==========//==========//==========//==========//==========//==========
        //==========//==========//==========//==========//==========//==========//==========//==========
        //==========//==========//==========//==========//==========//==========//==========//==========
        /* Square tables */
        struct sqrFreqMult
        { /* ODD HARMONICS */
            constexpr Sample operator()( const size_t index ) const
            { return (index*2 + 1.0); }
        };
        struct sqrAmp
        {
            constexpr Sample operator()( const size_t index ) const
            { return 1.0 / static_cast< Sample >( index*2 + 1 ); }
        };
        //==========//==========//==========//==========//==========//==========//==========//==========
        //==========//==========//==========//==========//==========//==========//==========//==========
        //==========//==========//==========//==========//==========//==========//==========//==========
        //==========//==========//==========//==========//==========//==========//==========//==========
        /* Triangle tables */
        struct triFreqMult
        { /* ODD HARMONICS */
            constexpr Sample operator()( const size_t index ) const
            { return (index*2 + 1.0); }
        };
        template< size_t MAXSIZE >
        struct triAmp
        {
            constexpr Sample operator()( const size_t index ) const
            {
                Sample v = (index*2 + 1);
                v *= v;
                v = 1.0 / v;
                return m_odds[index] ? -v : v ;
            }
        private:
            template< size_t SIZE >
            struct isOdd
            {
                constexpr isOdd() : m_table()
                {
                    bool odd = false;
                    for ( auto i = 0; i < SIZE; ++i ){ m_table[ i ] = odd; odd = !odd; }
                }
                constexpr bool operator[] ( const size_t index ) const { return m_table[ index ]; }
            private:
                bool m_table[ SIZE ];
            };

            static constexpr isOdd<MAXSIZE> m_odds{};
        };
        //==========//==========//==========//==========//==========//==========//==========//==========
        //==========//==========//==========//==========//==========//==========//==========//==========
        //==========//==========//==========//==========//==========//==========//==========//==========
        //==========//==========//==========//==========//==========//==========//==========//==========
        int m_nSinWaves{ MAXNHARMS };
        std::array< Sample, MAXNHARMS > m_freqMults, m_amps;
        static constexpr waveTable< MAXNHARMS, sawFreqMult > m_sawFreqs;
        static constexpr waveTable< MAXNHARMS, sqrFreqMult > m_sqrFreqs;
        static constexpr waveTable< MAXNHARMS, triFreqMult > m_triFreqs;
        static constexpr waveTable< MAXNHARMS, sawAmp > m_sawAmps;
        static constexpr waveTable< MAXNHARMS, sqrAmp > m_sqrAmps;
        static constexpr waveTable< MAXNHARMS, triAmp< MAXNHARMS> > m_triAmps;
    };
}


#endif /* sjf_additive_OSC_h */





