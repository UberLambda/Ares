#pragma once

#include <stddef.h>
#include <atomic>
#include <vector>
#include <utility>
#include "Base/DoubleBuffered.hh"
#include "Base/Ref.hh"
#include "Module/Module.hh"
#include "GlobalData.hh"
#include "FrameData.hh"

namespace Ares
{

/// An instance of Ares' engine core.
///
/// The core contains a list of modules.
/// Modules get updated each frame and can use or register new core facilities
/// to perform their functions.
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

    struct ModuleSlot
    {
        Ref<Module> ref;
        bool inited = false;

        inline bool operator==(const ModuleSlot& other) const
        {
            return this->ref == other.ref;
        }
    };
    std::vector<ModuleSlot> modules_;

    GlobalData globalData_;
    DoubleBuffered<FrameData> frameData_;

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


    /// The core's global data.
    /// This data is always valid for the duration of the core and contains
    inline GlobalData& g()
    {
        return globalData_;
    }

    /// The core's frame data for the current frame.
    /// This data is to be modified for the current frame so that the next frame
    /// can display/act upon it.
    inline FrameData& curr()
    {
        return frameData_.current();
    }

    /// The core's frame data for the previous frame.
    /// This data is to be rendered/processed/only read in the current frame,
    /// after which it will be cleared.
    inline const FrameData& past()
    {
        return frameData_.past();
    }


    /// Attempts to attachs the module to the core. If the core is already inited
    /// and/or running, also attempts to `init()` it after attaching it - logging
    /// an error message on init error.
    /// Returns `false` and does nothing if the module is already attached or
    /// `module` is null.
    bool attachModule(Ref<Module> module);

    /// Attempts to `halt()` then detach the given module.
    /// Returns `false` and does nothing on error (no such module attached/module
    /// is null).
    /// Note that the module is just detached, not deleted; if `module`'s reference
    /// count is still > 0 the module will be kept in memory!
    bool detachModule(Ref<Module> module);
};

}
