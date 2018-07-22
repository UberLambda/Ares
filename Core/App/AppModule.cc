#include "AppModule.hh"

#include <assert.h>
#include "../Core.hh"
#include "../Debug/Log.hh"

namespace Ares
{

AppModule::AppModule(const Path& dllPath)
    : dll_(dllPath, true),
      dllModule_(nullptr), dllLoadFunc_(nullptr), dllUnloadFunc_(nullptr)
{
    // NOTE IMPORTANT `doNotFree` is set in `Dll`'s constructor. This is because
    //      messages logged into `core.g().log` or any other pointers/refs/data
    //      somewhat shared inbetween the app library and Ares' executable
    //      **WOULD GET CORRUPTED IF ~Dll() WAS CALLED**!!
    //      The OS will clean everything up in the end anyways.
}

AppModule::~AppModule()
{
}


#define glog (*core.g().log)

bool AppModule::init(Core& core)
{
    if(!dll_)
    {
        ARES_log(glog, Error, "%s: Could not load library!", dll_.path());
        return false;
    }

    // Try to load the load func and unload func
    dllLoadFunc_ = (LoadFunc)dll_.symbol(LOAD_FUNC_NAME);
    if(!dllLoadFunc_)
    {
        ARES_log(glog, Error, "%s: Missing module load func!", dll_.path());
        return false;
    }
    dllUnloadFunc_ = (UnloadFunc)dll_.symbol(UNLOAD_FUNC_NAME);
    if(!dllUnloadFunc_)
    {
        ARES_log(glog, Error, "%s: Missing module unload func!", dll_.path());
        return false;
    }

    // Call the load func to get the inner module
    dllModule_ = dllLoadFunc_();
    if(!dllModule_)
    {
        ARES_log(glog, Error, "%s: Module load func returned null!", dll_.path());
        return false;
    }

    // Init the inner module
    bool initOk = dllModule_->init(core);
    if(!initOk)
    {
        ARES_log(glog, Error, "%s: Module init error!", dll_.path());
        return false;
    }

    ARES_log(glog, Debug, "%s: Module loaded", dll_.path());
    return true;
}

void AppModule::mainUpdate(Core& core)
{
    assert(dllModule_ && "Dll module was not loaded!");

    dllModule_->mainUpdate(core);
}

Task AppModule::updateTask(Core& core)
{
    assert(dllModule_ && "Dll module was not loaded!");

    return dllModule_->updateTask(core);
}

void AppModule::halt(Core& core)
{
    assert(dllModule_ && "Dll module was not loaded!");

    dllModule_->halt(core);

    dllUnloadFunc_(dllModule_); dllModule_ = nullptr;
    ARES_log(glog, Debug, "%s: Module unloaded", dll_.path());
}


}
