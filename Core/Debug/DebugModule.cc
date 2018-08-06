#include "DebugModule.hh"

#include <Core/Core.hh>
#include <Core/Debug/Log.hh>

namespace Ares
{

DebugModule::DebugModule()
{
}

DebugModule::~DebugModule()
{
}

#define glog (*core.g().log)

bool DebugModule::init(Core& core)
{
    ARES_log(glog, Debug, "DebugModule online");
    return true;
}

void DebugModule::mainUpdate(Core& core)
{
}

Task DebugModule::updateTask(Core& core)
{
    core_ = &core;

    static const auto updateFunc = [](TaskScheduler* scheduler, void* data)
    {
        auto dbgMod = reinterpret_cast<DebugModule*>(data);

        const auto& profilerEvents = dbgMod->core_->g().profilerEvents;
        for(Profiler::TimeEvent event : profilerEvents)
        {
            // TODO IMPORTANT Output profiling data to WebSocket!
        }
    };
    return {updateFunc, this};
}

void DebugModule::halt(Core& core)
{
    ARES_log(glog, Debug, "DebugModule offline");
}

}
