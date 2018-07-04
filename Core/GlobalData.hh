#pragma once

#include <memory>
#include "Base/TypeMap.hh"

namespace Ares
{

class Log; // (#include "Debug/Log.hh")
class TaskScheduler; // (#include "Task/TaskScheduler.hh")
class Scene; // (#include "Scene/Scene.hh")
class ResourceLoader; // (#include "Data/ResourceLoader.hh")

/// Engine data stored globally, independent of which frame it is accessed by.
///
/// This includes things like loaded assets, ...
/// are alternated in a ping-pong fashion.
/// **Rendering/simulation/AI... is always one frame behind!**
/// This heavily simplifies concurrency, since modules can run at any
/// time/in any order during a frame - displaying/receiving events from a
/// consistent, readonly "past" frame data, and pushing new events at will on the
/// read/write "current" frame data.
struct GlobalData
{
    /// The main engine log.
    Log* log;

    /// The task scheduler for the engine.
    TaskScheduler* scheduler;

    /// The scene where the action is taking place.
    Scene* scene;

    /// The main resource loader.
    ResourceLoader* resLoader;

    /// Any of the engine's extra, ondemand facilities (`Window`, ...).
    TypeMap facilities;
};

}
