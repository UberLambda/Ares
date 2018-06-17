#pragma once

#include "../Task/Task.hh"

namespace Ares
{

struct Core; // (#include "Core.hh")

/// The interface of engine core modules.
class Module
{
public:
    virtual ~Module() = default;

    /// Attempts to initialize the module instance; returns `false` on error.
    /// Does nothing and returns `true` if the module is already inited.
    /// **This function will be run on the main thread.**
    virtual bool init(Core& core) = 0;

    /// Does anything that has to be done for the module for this frame **on the
    /// main thread** here.
    /// Use this only for stuff that has to be done strictly on the main thread;
    /// for everything else, schedule worker thread tasks from `updateTask()`!
    virtual void mainUpdate(Core& core) = 0;

    /// Returns a task that, when **scheduled to run on a nonspecified worker
    /// thread**, will update do anything that has to be done for the module
    /// for this frame **outside of the main thread**.
    /// This task is scheduled before `mainUpdate()` is run for each module, so
    /// that worker threads have stuff to do while the main thread is busy running
    /// `mainUpdate()`s and doing I/O.
    /// The core will make sure that the `Task` has been completed before starting
    /// to the next update cycle.
    virtual Task updateTask(Core& core) = 0;

    /// Destroys an `init()`ed module instance.
    /// **This function will be run on the main thread.**
    virtual void halt(Core& core) = 0;
};

}
