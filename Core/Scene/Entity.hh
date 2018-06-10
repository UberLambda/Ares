#pragma once

#include "../Base/NumTypes.hh"

namespace Ares
{

/// An handle that identifies an entity currently in the world.
using Entity = U32;

/// The handle to an invalid (not-existing) entity.
static constexpr const Entity INVALID_ENTITY = -1;

}
