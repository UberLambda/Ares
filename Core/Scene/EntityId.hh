#pragma once

#include <Core/Base/NumTypes.hh>

namespace Ares
{

/// An handle that identifies an entity currently in the world.
using EntityId = U32;

/// The handle to an invalid (not-existing) entity.
static constexpr const EntityId INVALID_ENTITY_ID = -1;

}
