#pragma once

#include "../Module/Module.hh"
#include "Dll.hh"

namespace Ares
{

/// A `Module` loaded from a external shared library, containing application-specific code.
class AppModule : public Module
{
public:
    /// The type of the function in the loaded shared library that returns the
    /// inner module for this `AppModule`.
    using LoadFunc = Module*(*)();

    /// The type of the function in the loaded shared library that destroys the
    /// module that was returned by `PFN_LoadAppModuleFunc`.
    using UnloadFunc = void(*)(Module* loadedModule);

    /// The name of the `LoadFunc` to call in the loaded shared library.
    static constexpr const char* LOAD_FUNC_NAME = "ARES_loadAppModule";

    /// The name of the `UnloadFunc` to call in the loaded shared library.
    static constexpr const char* UNLOAD_FUNC_NAME = "ARES_unloadAppModule";

private:
    Dll dll_;
    LoadFunc dllLoadFunc_;
    UnloadFunc dllUnloadFunc_;
    Module* dllModule_;

public:
    /// Creates a module that will load application code from the shared library
    /// at `dllPath`.
    AppModule(const Path& dllPath);
    ~AppModule() override;

    bool init(Core& core) override;
    void mainUpdate(Core& core) override;
    Task updateTask(Core& core) override;
    void halt(Core& core) override;
};

}
