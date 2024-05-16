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
//    template< typename Sample, int TABLE_SIZE, typename Functor  >
//    struct table
//    {
//
//        constexpr table() : m_table() { for ( int i = 0; i < TABLE_SIZE; i++ ) { m_table[ i ] = func( static_cast<Sample>(i)/SIZE ); } }
//        ~table(){}
//
//        Sample getVal( Sample phase )
//        {
//            auto findex = phase * SIZE;
//            Sample x0, x1, x2, x3, mu;
//            auto ind1 = static_cast< long >( findex );
//            mu = findex - ind1;
//            x0 = m_table[ ( (ind1-1) & m_wrapMask ) ];
//            x1 = m_table[ ind1 & m_wrapMask ];
//            x2 = m_table[ ( (ind1+1) & m_wrapMask ) ];
//            x3 = m_table[ ( (ind1+2) & m_wrapMask ) ];
//            switch ( m_interpType )
//            {
//                case sjf_interpolators::interpolatorTypes::none:
//                    return m_table[ static_cast<long>(findex)&m_wrapMask ];
//                case sjf_interpolators::interpolatorTypes::linear :
//                    return sjf_interpolators::linearInterpolate( mu, x1, x2 );
//                case sjf_interpolators::interpolatorTypes::cubic :
//                    return sjf_interpolators::cubicInterpolate( mu, x0, x1, x2, x3 );
//                case sjf_interpolators::interpolatorTypes::pureData :
//                    return sjf_interpolators::fourPointInterpolatePD( mu, x0, x1, x2, x3 );
//                case sjf_interpolators::interpolatorTypes::fourthOrder :
//                    return sjf_interpolators::fourPointFourthOrderOptimal( mu, x0, x1, x2, x3 );
//                case sjf_interpolators::interpolatorTypes::godot :
//                    return sjf_interpolators::cubicInterpolateGodot( mu, x0, x1, x2, x3 );
//                case sjf_interpolators::interpolatorTypes::hermite :
//                    return sjf_interpolators::cubicInterpolateHermite( mu, x0, x1, x2, x3 );
//                default:
//                    return sjf_interpolators::linearInterpolate( mu, x1, x2 );
//            }
//        }
//        /** Set the interpolation Type to be used, the interpolation type see @sjf_interpolators */
//        void setInterpolationType( sjf_interpolators::interpolatorTypes interpType ) { m_interpType = interpType; }
//    private:
//
//        sjf_interpolators::interpolatorTypes m_interpType{sjf_interpolators::interpolatorTypes::linear};
//        static constexpr int m_wrapMask{TABLE_SIZE-1};
//        Sample m_table[ TABLE_SIZE ];
//        static constexpr Sample SIZE{TABLE_SIZE};
//        Functor func;
//    };
//

    template< typename Sample, long TABLE_SIZE, typename Functor >
    struct table
    {
        constexpr table() : m_table() { for ( int i = 0; i < TABLE_SIZE; i++ ) { m_table[ i ] = func( static_cast<Sample>(i)/SIZE ); } }
        ~table(){}

        Sample getVal( Sample phase ) { return m_interpolator( m_table, TABLE_SIZE, (phase*SIZE) ); }
        void setInterpolationType( sjf::interpolation::interpolatorTypes interpType ){ m_interpolator.setInterpolationType( interpType ); }
    private:
        static constexpr int m_wrapMask{TABLE_SIZE-1};
        Sample m_table[ TABLE_SIZE ];
        static constexpr Sample SIZE{TABLE_SIZE};
        Functor func;
        sjf::interpolation::interpolator<Sample> m_interpolator;
    };

}
#endif /* sjf_table_h */
