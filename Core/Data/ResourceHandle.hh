#pragma once

#include "../Base/NumTypes.hh"

namespace Ares
{

/// An handle to a particular resource of type `T` from a `ResourceLoader`.
/// `ResourceHandle<T>(-1)` means "invalid handle".
///
/// Resource handles are globally unique for a `ResourceLoader`. This means that
/// handles for different resources will always be different, even among different
/// types, and that if a resource is fully unloaded and then reloaded again it
/// will get a new handle.
template <typename T>
using ResourceHandle = U32;

}
