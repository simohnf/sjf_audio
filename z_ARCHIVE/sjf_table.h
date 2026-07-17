//
//  sjf_table.h
//
//  Created by Simon Fay on 13/05/2024.
//  Copyright © 2024 sjf. All rights reserved.
//

#ifndef sjf_table_h
#define sjf_table_h

#include "sjf_interpolators/sjf_interpolator.h"
#include "sjf_audioUtilitiesC++.h"

namespace sjf::wavetable
{
template< typename Sample, long TABLE_SIZE, typename Functor, interpolation::interpolatorTypes interpType >
//typename InterpFunctor >
    struct tab
    {
        constexpr tab() : m_table() { for ( int i = 0; i < TABLE_SIZE; i++ ) { m_table[ i ] = func( static_cast<Sample>(i)/SIZE ); } }
        ~tab(){}

        const inline Sample getVal( Sample phase ) const { return m_interpolator( m_table, WRAPMASK, (phase*SIZE) ); }

    private:
        Sample m_table[ TABLE_SIZE ];
        static constexpr Sample SIZE{ TABLE_SIZE };
        static constexpr long WRAPMASK{TABLE_SIZE-1};
        const Functor func;
//        const InterpFunctor m_interpolator;
        interpolation::interpolator<Sample, interpType> m_interpolator;
    };

//    /** wavetable */
//    template< typename Sample, long TABLE_SIZE, typename Functor >
//    struct table
//    {
//        constexpr table() : m_table() { for ( int i = 0; i < TABLE_SIZE; i++ ) { m_table[ i ] = func( static_cast<Sample>(i)/SIZE ); } }
//        ~table(){}
//
//        const inline Sample getVal( Sample phase ) const { return m_interpolator( m_table, TABLE_SIZE, (phase*SIZE) ); }
//        void setInterpolationType( sjf::interpolation::interpolatorTypes interpType ){ m_interpolator.setInterpolationType( interpType ); }
//    private:
//        Sample m_table[ TABLE_SIZE ];
//        static constexpr Sample SIZE{TABLE_SIZE};
//        const Functor func;
//        const sjf::interpolation::interpolator<Sample> m_interpolator;
//    };
//
//    template< typename Sample, size_t TABLE_SIZE >
//    struct sinTable
//    {
//        inline Sample getVal( Sample phase ) { return tab.getVal( phase ); }
//        void setInterpolationType( sjf::interpolation::interpolatorTypes interpType ){ tab.setInterpolationType( interpType ); }
//    private:
//        struct func{ Sample operator()( Sample findex ){ return gcem::sin<Sample>(findex*2.0*M_PI); } };
//        table< Sample, TABLE_SIZE, func > tab;
//    };
//
//    template< typename Sample, size_t TABLE_SIZE >
//    struct cosTable
//    {
//        inline Sample getVal( Sample phase ) { return tab.getVal( phase ); }
//        void setInterpolationType( sjf::interpolation::interpolatorTypes interpType ){ tab.setInterpolationType( interpType ); }
//    private:
//        struct func{ Sample operator()( Sample findex ){ return gcem::cos<Sample>(findex*2.0*M_PI); } };
//        table< Sample, TABLE_SIZE, func > tab;
//    };
}
#endif /* sjf_table_h */
