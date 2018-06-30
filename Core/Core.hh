#pragma once

#include <stddef.h>
#include <atomic>
#include <vector>
#include <utility>
#include "Base/TypeMap.hh"

namespace Ares
{

class Module; // (#include "Module/Module.hh" )
class Log; // (#include "Debug/Log.hh" )
class TaskScheduler; // (#include "Task/TaskScheduler.hh" )
class Scene; // (#include "Scene/Scene.hh" )
class FileStore; // (#include "Data/FileStore.hh" )
class ResourceLoader; // (#include "Data/ResourceLoader.hh" )

/// An instance of Ares' engine core.
///
/// The core contains a series of facilities (some of which and a list of modules.
/// Modules get updated each frame and can use or register new core facilities
/// to perform their functions.
/// Each facility is a self-contained object that performs some task
/// (ex.: a `Log` to log messages, a `Scene` to store components...).
///
/// Since `Core::facility<T>()` takes some lookup time to find the facility,
/// modules are encouraged to cache the value of that call on init.
/// Some important facilities are initialized by the core itself on `Core::init()`,
/// unless the user did not already set them up beforehand; it that case, the user-provided
/// facility will be used. Each of these important facilities has an an "alias pointer" in core,
/// i.e. a pointer caching `facility<T>()` for them at core `init()` time.
/// Important facilities are:
/// - `Log`: Aliased in `Core::log()`
/// - `TaskScheduler`: Aliased in `Core::scheduler()`
/// - `ResourceLoader`: Aliased to `Core::resourceLoader()`
///                     A `FolderFileStore` is also added, but not aliased anywhere;
///                     access it with `facility<FolderFileStore>()`
/// **WARNING**: Alias pointers are null before calling `Core::init()`!
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

    std::vector<Module*> modules_;
    TypeMap facilities_; ///< typeid(T) -> T facility slot map

    // "Alias pointers" to important facilities, see documentation of `Core`
    Log* log_;
    TaskScheduler* scheduler_;
    Scene* scene_;
    ResourceLoader* resourceLoader_;
    // End of alias pointers

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


    /// Attempts to get the facility of the core of type `T`. Returns a pointer
    /// to the facility if it is present or null otherwise.
    ///
    /// Using for storing `Log`, `Scene`, `ResourceLoader`, etc. in the core without
    /// explictly declaring one slot for each thing and allowing modules to
    /// register/retrieve facilities in a Core in a transparent way.
    ///
    /// For performance, call this function once at module initialization so that
    /// the Core knows wether to instantiate/retrieve the `T` facility and then
    /// cache the pointer (which is guaranteed to last until `Core` death) for
    /// later use. **WARNING**: The `T*` will become invalid when the core is
    /// destroyed!
    template <typename T>
    inline T* facility()
    {
        return facilities_.get<T>();
    }

    /// Attempts to move the given facility of type `T` into the core, or to
    /// construct a `T` facility for the core given some arguments.
    /// Does nothing and returns `false` if a `T` facility already exists for the core.
    /// The facility will be destroyed when the `Core` itself is destroyed.
    template <typename T, typename... TArgs>
    bool addFacility(TArgs&&... tArgs)
    {
        return facilities_.add<T>(std::forward<TArgs>(tArgs)...);
    }

    /// An alias pointer to `facility<Log>()`.
    /// See `Core`'s documentation.
    inline Log* log()
    {
        return log_;
    }

    /// An alias pointer to `facility<TaskScheduler>()`.
    /// See `Core`'s documentation.
    inline TaskScheduler* scheduler()
    {
        return scheduler_;
    }

    /// An alias pointer to `facility<Scene>()`.
    /// See `Core`'s documentation.
    inline Scene* scene()
    {
        return scene_;
    }

    /// An alias pointer to `facility<ResourceLoader>()`.
    /// See `Core`'s documentation.
    inline ResourceLoader* resourceLoader()
    {
        return resourceLoader_;
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
};

/// An utility class that will `detachModule()` and `delete` a module when the
/// object goes out of scope.
class AutoDetach
{
    Core* core_;
    Module* module_;

    AutoDetach(const AutoDetach& toCopy) = delete;
    AutoDetach& operator=(const AutoDetach& toCopy) = delete;

public:
    AutoDetach(Core& core, Module* module)
        : core_(&core), module_(module)
    {
    }

    AutoDetach(AutoDetach&& toMove)
    {
        (void)operator=(std::move(toMove));
    }
    AutoDetach& operator=(AutoDetach&& toMove)
    {
        // Destroy any old module
        this->~AutoDetach();

        // Move data over
        this->core_ = toMove.core_;
        this->module_ = toMove.module_;

        // Invalidate the moved instance
        toMove.module_ = nullptr;

        return *this;
    }

    ~AutoDetach();

    inline operator Module*()
    {
        return module_;
    }
};

}
