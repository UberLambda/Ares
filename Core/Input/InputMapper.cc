#include "InputMapper.hh"

#include <glm/vec4.hpp>
#include <glm/geometric.hpp>

namespace Ares
{

void InputMapper::update(const AxisMap& inputMap)
{

    // NOTE Querying an unrecognized axis from `inputMap` always returns `0`.
    //      Hence by querying an axis named something like "NULL" you will always get `0`,
    //      so `inputAxis * scale = 0 * n = 0`, so the final dot product will not change if a
    //      binding is undefined - exactly what is needed!
    static_assert(InputMapping::MAX_BINDINGS == 4,
                 "This [SIMD] dot product code only works for 4 bindings, need to rewrite it");

    for(auto it = mappings_.begin(); it != mappings_.end(); it ++)
    {
        // - Fetch input axes and scales
        // - Calculate dot product (a1 * s1 + a2 * s2 + a3 * s3 + a4 * s4)
        // - Store dot product on output map
        // (Warning: the following code could contain traces of milk, peanuts, manual loop unrolling)
        glm::tvec4<Axis> inputs(inputMap[it->bindings[0].inputAxis],
                                inputMap[it->bindings[1].inputAxis],
                                inputMap[it->bindings[2].inputAxis],
                                inputMap[it->bindings[3].inputAxis]);
        glm::tvec4<Axis> scales(it->bindings[0].scale,
                                it->bindings[1].scale,
                                it->bindings[2].scale,
                                it->bindings[3].scale);
        Axis unclampedOutput = glm::dot(inputs, scales);
        outMap_[it->outputAxis] = glm::clamp(unclampedOutput, it->min, it->max);
    }
}

}
