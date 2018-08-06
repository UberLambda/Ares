#pragma once

#include <Core/Api.h>
#include <Core/Module/Module.hh>

namespace Ares
{

struct Core; // #include "Core.hh"

/// A module used for gathering/displaying debug data.
class ARES_API DebugModule : public Module
{
    Core* core_; ///< Set each time `updateTask()` is called

public:
    DebugModule();
    ~DebugModule() override;

    bool init(Core& core) override;
    void mainUpdate(Core& core) override;
    Task updateTask(Core& core) override;
    void halt(Core& core) override;
};

}
