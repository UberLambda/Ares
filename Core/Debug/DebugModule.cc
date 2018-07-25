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
    static const auto updateFunc = [](TaskScheduler* scheduler, void* data)
    {
    };
    return {updateFunc, this};
}

void DebugModule::halt(Core& core)
{
    ARES_log(glog, Debug, "DebugModule offline");
}

}
