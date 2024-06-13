//
//  sjf_seriesAP.h
//
//  Created by Simon Fay on 27/03/2024.
//


#ifndef sjf_rev_seriesAllpass_h
#define sjf_rev_seriesAllpass_h

#include "../sjf_rev.h"

namespace sjf::rev
{
    /**
     An array of one multiply allpass filters connected in series
     input signal is passed through each all pass filter in turn
     */
    template < typename Sample, interpolation::interpolatorTypes interpType = interpolation::interpolatorTypes::pureData >
//, typename INTERPOLATION_FUNCTOR = interpolation::fourPointInterpolatePD< Sample > >
    class seriesAllpass
    {
    public:
        seriesAllpass( const size_t nStages = 8 ) noexcept : NSTAGES(nStages)
        {
            m_aps.resize( NSTAGES );
            m_coefs.resize( NSTAGES, 0.7 );
            m_delayTimesSamps.resize( NSTAGES );
            m_damping.resize( NSTAGES, 0 );
            
            initialise( 4410 ); // default sample rate
            for ( auto & d : m_delayTimesSamps )
                d = ( rand01()*2048 ) + 1024; // random delay times just so it's initialised
        }
        ~seriesAllpass(){} 

        /**
         This must be called before first use in order to set basic information such as maximum delay length
         */
        void initialise( const size_t maxDelayInSamples )
        {
            auto mDT = sjf_isPowerOf( maxDelayInSamples, 2 ) ? maxDelayInSamples : sjf_nearestPowerAbove( maxDelayInSamples, 2ul );
            for ( auto & a : m_aps )
                a.initialise( mDT );
        }
        
        /**
         This sets all of the allpass coefficients to the give value
         */
        void setCoefs( const Sample coef )
        {
            for ( auto & c : m_coefs )
                c = coef;
        }
        
        /**
         This sets all of the allpass coefficients
         */
        void setCoefs( const vect< Sample >& coefs )
        {
            assert( coefs.size() == NSTAGES );
            for ( auto i = 0; i < NSTAGES; ++i )
                m_coefs[ i ] = coefs[ i ];
        }
        
        /**
         This sets the coefficient for an inndividual allpass to the give value
         */
        void setCoef( const Sample coef, const size_t apNum )
        {
            m_coefs[ apNum ] = coef;
        }
        
        
        
        /**
         Set all of the delayTimes
         */
        void setDelayTimes( const vect< Sample >& dt )
        {
            assert( dt.size() == NSTAGES );
            for ( auto i = 0; i < NSTAGES; ++i )
                m_delayTimesSamps[ i ] = dt[ i ];
        }
        
        /**
         Set all of the delayTime of an individual allpass
         */
        void setDelayTime( const Sample dt, const size_t apNum )
        {
            m_delayTimesSamps[ apNum ] = dt;
        }
        
        /**
         Set all of the damping coefficients
         */
        void setDamping( const vect< Sample >& damp )
        {
            assert( damp.size() == NSTAGES );
            for ( auto i = 0; i < NSTAGES; ++i )
                m_damping[ i ] = damp[ i ];
        }
        
        /**
         Set  the damping coefficient for an individual allpass
         */
        void setDamping( const Sample damp, const size_t apNum )
        {
            m_damping[ apNum ] = damp;
        }
        
        /**
         Set  all of the damping coefficients to the same value
         */
        void setDamping( const Sample damp )
        {
            for ( auto & d: m_damping )
                d = damp;
        }

        /**
         This processes a single delay, it should be called once for every sample in the block
         Input:
            sample to process
         output:
            Processed sample
         */
        inline Sample process( Sample x )
        {
            for ( auto i = 0; i < NSTAGES; ++i )
                x = m_aps[ i ].process( x, m_delayTimesSamps[ i ], m_coefs[ i ] );
            return x;
        }
        
        
        /**
         This processes a single delay with seperate damping coefficient for each allpass, it should be called once for every sample in the block
         Input:
            sample to process
         output:
            Processed sample
         */
        inline Sample processD( Sample x )
        {
            for ( auto i = 0; i < NSTAGES; ++i )
                x = m_aps[ i ].process( x, m_delayTimesSamps[ i ], m_coefs[ i ], m_damping[ i ] );
            return x;
        }
        
        /**
         This processes a single delay with a single damping coefficient for all of the allpass filters, it should be called once for every sample in the block
         Input:
            sample to process
                damping coefficient
         output:
            Processed sample
         */
        inline Sample process( Sample x, Sample damping )
        {
            for ( auto i = 0; i < NSTAGES; ++i )
                x = m_aps[ i ].process( x, m_delayTimesSamps[ i ], m_coefs[ i ], damping );
            return x;
        }
        

    private:
        const size_t NSTAGES;
        vect< filters::oneMultAP< Sample, interpType > > m_aps;
        vect< Sample > m_coefs, m_delayTimesSamps, m_damping;
    };
    
    //============//============//============//============//============//============
    //============//============//============//============//============//============
    //============//============//============//============//============//============
    //============//============//============//============//============//============
}

#endif /* sjf_rev_APLoop_h */
