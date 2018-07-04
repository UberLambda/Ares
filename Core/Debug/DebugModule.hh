#pragma once

#include "../Module/Module.hh"

namespace Ares
{

/// A module used for gathering/displaying debug data.
class DebugModule : public Module
{
public:
    DebugModule();
    ~DebugModule() override;

    bool init(Core& core) override;
    void mainUpdate(Core& core) override;
    Task updateTask(Core& core) override;
    void halt(Core& core) override;
};

}
