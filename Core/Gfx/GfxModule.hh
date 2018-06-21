#pragma once

#include "../Module/Module.hh"

namespace Ares
{

class GfxModule : public Module
{
public:
    GfxModule();
    ~GfxModule() override;

    bool init(Core& core) override;
    void mainUpdate(Core& core) override;
    Task updateTask(Core& core) override;
    void halt(Core& core) override;
};

}
