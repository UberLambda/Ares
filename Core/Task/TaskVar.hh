#pragma once

#include <atomic>
#include "../Base/NumTypes.hh"

namespace Ares
{

/// The type of values contained in `TaskVar`s.
using TaskVarValue = U64;

/// An atomic value to be used with `TaskScheduler`s.
using TaskVar = std::atomic<TaskVarValue>;

}
