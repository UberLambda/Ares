#include "GfxModule.hh"

#include "../Core.hh"

namespace Ares
{

GfxModule::GfxModule()
{
}

bool GfxModule::init(Core& core)
{
    // FIXME IMPLEMENT
    return true;
}

void GfxModule::halt(Core& core)
{
    // FIXME IMPLEMENT
}

GfxModule::~GfxModule()
{
}


void GfxModule::mainUpdate(Core& core)
{
    // FIXME IMPLEMENT
}

Task GfxModule::updateTask(Core& core)
{
    auto updateFunc = [](TaskScheduler* scheduler, void* data)
    {
        // FIXME IMPLEMENT
    };
    return {updateFunc, this};
}

}
