#pragma once

#include <stddef.h>
#include <atomic>
#include <memory>
#include <vector>

namespace Ares
{

class TaskScheduler; // (#include "Task/TaskScheduler.hh"
class Scene; // (#include "Scene/Scene.hh" )
class Log; // (#include "Debug/Log.hh" )
class Module; // (#include "Module/Module.hh" )

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

    std::vector<Module*> modules_;


    Core(const Core& toCopy) = delete;
    Core& operator=(const Core& toCopy) = delete;

    Core(Core&& toMove) = delete;
    Core& operator=(Core&& toMove) = delete;


    /// Attempts to `init()` a module for this core; returns `false` and logs
    /// some information on error.
    bool initModule(Module* module);

public:
    /// Creates a new core instance, but does not initialize it; see `init()`.
    Core();

    /// Destroys the core instance, halting it beforehand if needed.
    /// All modules that are still attached are `halt()`ed.
    ~Core();


    /// Attempts to initialize the core, then all of the modules attached to it;
    /// returns `false` on error.
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


    /// Attempts to attachs the module to the core. If the core is already inited
    /// and/or running, also attempts to `init()` it after attaching it - logging
    /// an error message on init error.
    /// Returns `false` and does nothing if the module is already attached or
    /// `module` is null.
    /// **WARNING**: The module must stay valid for the entire lifetime of the
    ///              `Core`, or atleast until it is detached!
    bool attachModule(Module* module);

    /// Attempts to `halt()` then detach the given module.
    /// Returns `false` and does nothing on error (no such module attached/module
    /// is null).
    bool detachModule(Module* module);



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
