#pragma once

#include <vector>
#include <Core/Api.h>
#include <Core/Visual/Axis.hh>

namespace Ares
{

/// A mapping from a series of (input axis, scale) binding pair to
/// an output axis. See `InputMapper`.
struct ARES_API InputMapping
{
    static constexpr const unsigned int MAX_BINDINGS = 4; ///< The maximum number of binding pairs.

    AxisName outputAxis; ///< The name of the output axis in the output map.
    struct ARES_API 
    {
        AxisName inputAxis{"NULL"}; ///< The input axis to bind.
                                    ///  Make it "NULL" if this mapping is not to be used.
        float scale = 1.0f; ///< The factor to scale the input axis by.

    } bindings[4]; ///< The (input axis, scale) bindings to the output axis.

    Axis min = -1.0f; /// < The floor value to clamp the output to.
    Axis max = 1.0f; /// < The ceiling value to clamp the output to.
};

/// A mapper from axes in an input `AxisMap` to output ones.
/// Each output `Axis` is calculated as `clamp((inputAxis1 * scale1) + (inputAxis2 * scale2)
/// + ... + (inputAxisN * scaleN)), min, max)` depending on the `n` (inputAxis, scale) binding
/// pairs found in the mapper's `InputMapping`s.
class ARES_API InputMapper
{
public:
    /// An editable list of input mappings.
    using InputMappingList = std::vector<InputMapping>;

private:
    AxisMap outMap_;
    InputMappingList mappings_;

public:
    InputMapper() = default;
    ~InputMapper() = default;

    /// Returns a reference to the list of mappings used by this mapper.
    inline InputMappingList& mappings()
    {
        return mappings_;
    }

    /// Updates the output axes by recalculating them based on the input ones
    /// found in `inputMap` and the mappings in `mappings()`.
    void update(const AxisMap& inputMap);

    /// Returns the output `Axis` with the given name, calculated at the last
    /// `update()` call. Returns `0` for unrecognized `name`s.
    inline Axis operator[](const AxisName& name) const
    {
        return outMap_[name];
    }
};

}
