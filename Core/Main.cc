#include "Core.hh"
#include "Debug/Log.hh"
#include "Base/Utils.hh"

#include "Visual/Window.hh"
#include "Gfx/GfxModule.hh"
#ifndef NDEBUG
#   include "Debug/DebugModule.hh"
#endif

using namespace Ares;


/// The instance of the engine core.
static Core core;

#define glog (*core.g().log)


/// Adds required facilities and modules to `core`. Returns `false` on error.
static bool addCoreModulesAndFacilities()
{
    unsigned int nModulesAttachedHere = 0;


    // Window facility
    // TODO: Load videomode and title (app name) from config file
    VideoMode targetVideoMode;
    targetVideoMode.fullscreenMode = VideoMode::Windowed;
    targetVideoMode.resolution = {800, 600};
    targetVideoMode.refreshRate = 0; // (don't care)

    ARES_log(glog, Trace, "Creating window");
    core.g().facilities.add<Window>(Window::GL33, targetVideoMode, "Ares");
    if(!core.g().facilities.get<Window>()->operator bool())
    {
        ARES_log(glog, Fatal, "Failed to create window");
        return false;
    }

    // GfxModule [requires Window facility]
    ARES_log(glog, Trace, "Attaching GfxModule");
    core.attachModule(intoRef<Module>(new GfxModule()));
    nModulesAttachedHere ++;

#ifndef NDEBUG
    ARES_log(glog, Warning, "!! DEBUG BUILD !!");

    // DebugModule
    {
        core.attachModule(intoRef<Module>(new DebugModule()));
        nModulesAttachedHere ++;
    }
#else
    ARES_log(glog, Trace, "Release/RelWithDebInfo build");
#endif


    if(core.nAttachedModules() == nModulesAttachedHere)
    {
        return true;
    }
    else
    {
        ARES_log(glog, Fatal,
                 "Some module[s] or facilit[ies] failed to initialize, aborting!");
        return false;
    }
}


int main(int argc, char** argv)
{
    if(!core.init())
    {
        // Core initialization error
        // TODO Show a message box to inform the user of this
        // The core's log will print and flush appropriate error messages; just exit here
        return EXIT_FAILURE;
    }

    glog.flush();

    bool modsOk = addCoreModulesAndFacilities();
    glog.flush();
    if(!modsOk)
    {
        // Facility or module initialization error
        return EXIT_FAILURE;
    }

    // Main loop, run on main thread. This will return only when the main loop is
    // done.
    bool runOk = core.run();

    ARES_log(glog, Info, "Shutdown");
    glog.flush();

    return runOk ? EXIT_SUCCESS : EXIT_FAILURE;
}
