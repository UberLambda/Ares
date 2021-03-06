#pragma once

#include <Core/Api.h>

namespace Ares
{

class TaskScheduler;

/// A function to be run when a `Task` is run.
using TaskFunc = void(*)(TaskScheduler* scheduler, void* data);

/// An atomic task to execute.
struct ARES_API Task
{
    TaskFunc func = nullptr; ///< The function to be run when the task is run.
    void* data = nullptr; ///< Some data to pass to `func`.

    /// Returns `true` if the task currently has a valid function associated to it.
    inline operator bool() const
    {
        return func != nullptr;
    }
};

}
