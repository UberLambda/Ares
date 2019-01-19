#pragma once

#include <memory>
#include <vector>
#include "Api.h"
#include "Base/TypeMap.hh"
#include "Debug/Profiler.hh"
#include "Event/EventMatrix.hh"

namespace Ares
{

class Log; // (#include "Debug/Log.hh")
class Profiler; // (#include "Debug/Profiler.hh")
class TaskScheduler; // (#include "Task/TaskScheduler.hh")
class Scene; // (#include "Scene/Scene.hh")
class ResourceLoader; // (#include "Data/ResourceLoader.hh")

/// Engine data stored globally, independent of which frame it is accessed by.
/// Also see FrameData.
struct ARES_API GlobalData
{
    /// The main engine log.
    Log* log;

    /// The main engine profiler.
    Profiler* profiler;

    /// The profiling events that happened last frame.
    std::vector<Profiler::TimeEvent> profilerEvents;

    /// The task scheduler for the engine.
    TaskScheduler* scheduler;

    /// The scene where the action is taking place.
    Scene* scene;

    /// The main resource loader.
    ResourceLoader* resLoader;

    /// Any of the engine's extra, ondemand facilities (`Window`, ...).
    TypeMap facilities;

    /// The engine's core event matrix where all event callbacks are registered.
    EventMatrix eventMatrix;
};

}
