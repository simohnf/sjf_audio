//
//  sjf_fbLimiters.h
//  sjf_verb
//
//  Created by Simon Fay on 07/06/2024.
//  Copyright © 2024 sjf. All rights reserved.
//

#ifndef sjf_fbLimiters_h
#define sjf_fbLimiters_h

/**
 A collection of basic building blocks for reverb design
 */
namespace sjf::rev::fbLimiters
{

    enum class fbLimiterTypes{ none, tanh };

    /** basic tanh based limiting */
    template< typename Sample >
    struct limit { inline Sample operator()( Sample x ) const { return sjf::nonlinearities::tanhSimple( x );} };

    /** Only included for consitency in template, should be optimised away (hopefully)*/
    template< typename Sample >
    struct nolimit { inline Sample operator()( Sample x ) const { return x; } };

    template< typename Sample, fbLimiterTypes type >
    struct limiter;

    template< typename Sample >
    struct limiter< Sample, fbLimiterTypes::none >
    {
        Sample operator()( Sample x ) const { return x; }
    };

    template< typename Sample >
    struct limiter< Sample, fbLimiterTypes::tanh >
    {
        Sample operator()( Sample x ) const { return x; }
    };

//========================//========================//========================//========================//========================
//========================//========================//========================//========================//========================
//========================//========================//========================//========================//========================
//========================//========================//========================//========================//========================
//========================//========================//========================//========================//========================

    

}
#endif /* sjf_fbLimiters_h */
