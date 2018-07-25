#include "InputModule.hh"

#include <Core/Core.hh>
#include <Core/Visual/Window.hh>
#include <Core/Input/InputMapper.hh>
#include <Core/Debug/Log.hh>

namespace Ares
{

InputModule::InputModule()
    : window_(nullptr), inputMapper_(nullptr)
{
}

InputModule::~InputModule()
{
}


#define glog (*core.g().log)

bool InputModule::init(Core& core)
{
    window_ = core.g().facilities.get<Window>();
    if(!window_ || !window_->operator bool())
    {
        ARES_log(glog, Error,
                 "InputModule requires a Window facility but %s",
                 window_ ? "it failed to initialize" : "it was not added");
        return false;
    }

    inputMapper_ = core.g().facilities.get<InputMapper>();
    if(!inputMapper_)
    {
        // Create a new empty mapper
        (void)core.g().facilities.add<InputMapper>();
        inputMapper_ = core.g().facilities.get<InputMapper>();
    }

    return true;
}

void InputModule::mainUpdate(Core& core)
{
    // Poll all user (keyboard, mouse, joystick...) events + OS events (window moved, ...)
    // and calculate `window_.axisMap()`
    window_->pollEvents();

    // Calculate virtual (mapped) axes based on the real axes in `window_.axisMap()`
    inputMapper_->update(window_->axisMap());
}

Task InputModule::updateTask(Core& core)
{
    return {nullptr, nullptr};
}

void InputModule::halt(Core& core)
{
}

}
