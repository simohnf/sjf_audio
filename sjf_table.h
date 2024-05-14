//
//  sjf_table.h
//
//  Created by Simon Fay on 13/05/2024.
//  Copyright © 2024 sjf. All rights reserved.
//

#ifndef sjf_table_h
#define sjf_table_h
#include "sjf_interpolators.h"
#include "sjf_audioUtilitiesC++.h"

namespace sjf::wavetable
{

    template< typename Sample, int TABLE_SIZE, typename Functor  >
    struct table
    {
        using getFunc = sjf::utilities::classMemberFunctionPointer<table,Sample,Sample>;
        constexpr table() : m_table() { for ( int i = 0; i < TABLE_SIZE; i++ ) { m_table[ i ] = func( static_cast<Sample>(i)/SIZE ); } }
        ~table(){}
        
        
        
        Sample getVal( Sample phase ) { return getValue( phase * SIZE ); }
        /** Set the interpolation Type to be used, the interpolation type see @sjf_interpolators */
        void setInterpolationType( sjf_interpolators::interpolatorTypes interpType )
        {
            switch ( interpType ) {
                case 0:
                    getValue = &table::noInterp;
                    return;
                case sjf_interpolators::interpolatorTypes::linear :
                    getValue = &table::linInterp;
                    return;
                case sjf_interpolators::interpolatorTypes::cubic :
                    getValue = &table::cubicInterp;
                    return;
                case sjf_interpolators::interpolatorTypes::pureData :
                    getValue = &table::pdInterp;
                    return;
                case sjf_interpolators::interpolatorTypes::fourthOrder :
                    getValue = &table::fourPointInterp;
                    return;
                case sjf_interpolators::interpolatorTypes::godot :
                    getValue = &table::godotInterp;
                    return;
                case sjf_interpolators::interpolatorTypes::hermite :
                    getValue = &table::hermiteInterp;
                    return;
                default:
                    getValue = &table::linInterp;
                    return;
            }
        }
    private:
        inline Sample noInterp( Sample findex ){ return m_table[ findex ]; }
        
        inline Sample linInterp( Sample findex )
        {
            Sample x1, x2, mu;
            auto ind1 = static_cast< long >( findex );
            mu = findex - ind1;
            x1 = m_table[ ind1 & m_wrapMask ];
            x2 = m_table[ ( (ind1+1) & m_wrapMask ) ];
            
            return sjf_interpolators::linearInterpolate( mu, x1, x2 );
        }
        
        inline Sample cubicInterp( Sample findex )
        {
            Sample x0, x1, x2, x3, mu;
            auto ind1 = static_cast< long >( findex );
            mu = findex - ind1;
            x0 = m_table[ ( (ind1-1) & m_wrapMask ) ];
            x1 = m_table[ ind1 & m_wrapMask ];
            x2 = m_table[ ( (ind1+1) & m_wrapMask ) ];
            x3 = m_table[ ( (ind1+2) & m_wrapMask ) ];
            return sjf_interpolators::cubicInterpolate( mu, x0, x1, x2, x3 );
        }
        
        inline Sample pdInterp( Sample findex )
        {
            Sample x0, x1, x2, x3, mu;
            auto ind1 = static_cast< long >( findex );
            mu = findex - ind1;
            x0 = m_table[ ( (ind1-1) & m_wrapMask ) ];
            x1 = m_table[ ind1 & m_wrapMask ];
            x2 = m_table[ ( (ind1+1) & m_wrapMask ) ];
            x3 = m_table[ ( (ind1+2) & m_wrapMask ) ];
            return sjf_interpolators::fourPointInterpolatePD( mu, x0, x1, x2, x3 );
        }

        
        inline Sample fourPointInterp( Sample findex )
        {
            Sample x0, x1, x2, x3, mu;
            auto ind1 = static_cast< long >( findex );
            mu = findex - ind1;
            x0 = m_table[ ( (ind1-1) & m_wrapMask ) ];
            x1 = m_table[ ind1 & m_wrapMask ];
            x2 = m_table[ ( (ind1+1) & m_wrapMask ) ];
            x3 = m_table[ ( (ind1+2) & m_wrapMask ) ];
            return sjf_interpolators::fourPointFourthOrderOptimal( mu, x0, x1, x2, x3 );
        }
        
        inline Sample godotInterp( Sample findex )
        {
            Sample x0, x1, x2, x3, mu;
            auto ind1 = static_cast< long >( findex );
            mu = findex - ind1;
            x0 = m_table[ ( (ind1-1) & m_wrapMask ) ];
            x1 = m_table[ ind1 & m_wrapMask ];
            x2 = m_table[ ( (ind1+1) & m_wrapMask ) ];
            x3 = m_table[ ( (ind1+2) & m_wrapMask ) ];
            return sjf_interpolators::cubicInterpolateGodot( mu, x0, x1, x2, x3 );
        }
        
        inline Sample hermiteInterp( Sample findex )
        {
            Sample x0, x1, x2, x3, mu;
            auto ind1 = static_cast< long >( findex );
            mu = findex - ind1;
            x0 = m_table[ ( (ind1-1) & m_wrapMask ) ];
            x1 = m_table[ ind1 & m_wrapMask ];
            x2 = m_table[ ( (ind1+1) & m_wrapMask ) ];
            x3 = m_table[ ( (ind1+2) & m_wrapMask ) ];
            return sjf_interpolators::cubicInterpolateHermite( mu, x0, x1, x2, x3 );
        }
        
        /** get Value from the table */
        getFunc getValue{this,&table::linInterp};
        
        static constexpr int m_wrapMask{TABLE_SIZE-1};
        Sample m_table[ TABLE_SIZE ];
        static constexpr Sample SIZE{TABLE_SIZE};
        Functor func;
    };
}
#endif /* sjf_table_h */
