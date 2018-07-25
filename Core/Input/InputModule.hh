#pragma once

#include <Core/Api.h>
#include <Core/Module/Module.hh>

namespace Ares
{

class Window; // (#include "Visual/Window.hh")
class InputMapper; // (#include "Input/InputMapper.hh")

/// A module used for polling user + OS input.
class ARES_API InputModule : public Module
{
    Window* window_; // (retrieved from `core.g().facilities` on init)
    InputMapper* inputMapper_; // (retrieved or created from `core.g().facilities` on init)

public:
    InputModule();
    ~InputModule() override;

    bool init(Core& core) override;
    void mainUpdate(Core& core) override;
    Task updateTask(Core& core) override;
    void halt(Core& core) override;
};

}
