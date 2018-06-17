#pragma once

#include <stddef.h>
#include <atomic>
#include <memory>

namespace Ares
{

class TaskScheduler; // (#include "Task/TaskScheduler.hh"
class Scene; // (#include "Scene/Scene.hh" )
class Log; // (#include "Debug/Log.hh" )

/// An instance of Ares' engine core.
class Core
{
    /// The current state of a `Core`.
    enum State
    {
        Dead, ///< Not initialized, not running.
        Inited, ///< Initialized but not (currently) running.
        Running, ///< Initializes and running.
    };

private:
    std::atomic<State> state_;
    std::unique_ptr<Log> log_;
    std::unique_ptr<TaskScheduler> scheduler_;
    std::unique_ptr<Scene> scene_;

    Core(const Core& toCopy) = delete;
    Core& operator=(const Core& toCopy) = delete;

    Core(Core&& toMove) = delete;
    Core& operator=(Core&& toMove) = delete;

public:
    /// Creates a new core instance, but does not initialize it; see `init()`.
    Core();

    /// Destroys the core instance, halting it beforehand if needed.
    ~Core();


    /// Attempts to initialize the core; returns `false` on error.
    /// **Call this from the main thread!**
    /// On success, `state()` will switch to `Inited`.
    /// Does nothing and returns `true` if the core is already initalized.
    ///
    /// See `CoreConfig.h` for some initialization parameters of inner core structures.
    bool init();

    /// Starts running the core's main loop; it will return only after `halt()`
    /// is called by an inner task/interrupt handler/something else while the core
    /// is running.
    /// Returns `false` if the core could not be run because it was dead or already
    /// running.
    /// **Call this from the main thread!**
    /// On success, `state()` will switch to `Running`.
    bool run();

    /// Marks the core as halted, stopping the main loop if it was running.
    /// `state()` will if switch back to `Inited` from `Running`, or stay `Dead`
    /// if the core was `Dead`.
    void halt();

    /// Returns the current state of the core.
    inline State state() const
    {
        return state_;
    }


    /// The core's log instance.
    inline Log& log()
    {
        return *log_;
    }

    /// The core's task scheduler.
    inline TaskScheduler& scheduler()
    {
        return *scheduler_;
    }

    /// The core's scene.
    inline Scene& scene()
    {
        return *scene_;
    }
};

}
