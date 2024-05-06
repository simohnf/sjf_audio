//
//  sjf_multiChannelEffect.h
//  sjf_verb
//
//  Created by Simon Fay on 05/05/2024.
//  Copyright © 2024 sjf. All rights reserved.
//

#ifndef sjf_multiChannelEffect_h
#define sjf_multiChannelEffect_h


namespace sjf
{
    template< typename Sample >
    class multiChannelEffect
    {
    public:
        virtual ~multiChannelEffect(){};
        virtual void inPlace( std::vector< Sample >& samples ){};
//        virtual void inPlace( std::vector< double >& samples ){};
    };
}

#endif /* sjf_multiEffect_h */
