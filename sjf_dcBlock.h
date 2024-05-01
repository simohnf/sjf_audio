//
//  sjf_dcBlock.h
//  sjf_verb
//
//  Created by Simon Fay on 30/04/2024.
//

#ifndef sjf_dcBlock_h
#define sjf_dcBlock_h
namespace sjf::filters
{
    template< typename Sample >
    class dcBlock
    {
    public:
        dcBlock(){}
        ~dcBlock(){}
        
        Sample process( Sample x )
        {
            m_y1  = x - m_x1 + COEF*m_y1;
            m_x1 = x;
            return m_y1;
        }
    private:
        Sample m_x1 = 0, m_y1 = 0;
        static constexpr Sample COEF = 0.9997;
    };

}

#endif /* sjf_dcBlock_h */
