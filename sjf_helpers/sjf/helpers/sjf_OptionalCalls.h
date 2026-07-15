//
// Created by Simon Fay on 15/07/2026.
//

#include <JuceHeader.h>
namespace sjf::optional_calls
{
    template <typename T, typename Info>
    auto setPositionInfo(T& processor, const Info& info, int)
    -> decltype(processor.setPositionInfo(info), void())
    {
        processor.setPositionInfo(info);
    }

    // Priority 2: Fallback that matches everything else
    template <typename T, typename Info>
    void setPositionInfo(T& processor, const Info& info, long)
    {
        // Do nothing
    }

}