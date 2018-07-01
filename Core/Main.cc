#include "Core.hh"
#include "Debug/Log.hh"
#include "Base/Platform.h"
#include "Base/Utils.hh"

#include "Gfx/GfxModule.hh"

using namespace Ares;


/// The instance of the engine core.
static Core core;


#if ARES_PLATFORM_IS_POSIX
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <atomic>

/// Set to the signal caught by the signal handler.
static std::atomic<int> caughtSignal{INT_MAX};
static std::atomic<unsigned int> nSignalsCatches{0};
static constexpr const unsigned int MAX_N_SIGNAL_CATCHES = 10;

static void signalHandler(int signal)
{
    if(caughtSignal != INT_MAX)
    {
        // A signal was already caught; do not attempt to do anything else.
        // This is to make sure that this handler does not cause other signals
        // to be raised, causing a potentially infinite recursion.

        if(++nSignalsCatches > MAX_N_SIGNAL_CATCHES)
        {
            // Signals keep getting caught but the main loop does not terminate,
            // we're stuck here! `abort()` the program without even trying to
            // cleanup
            abort();
        }

        return;
    }

    caughtSignal = signal;
    core.halt();
}

#endif


int main(int argc, char** argv)
{
    if(!core.init())
    {
        // Core initialization error
        // TODO Show a message box to inform the user of this
        // The core's log will print and flush appropriate error messages; just exit here
        return EXIT_FAILURE;
    }

    Log& glog = *core.g().log;

#if ARES_PLATFORM_IS_POSIX
    static constexpr const int signalsToCatch[] =
    {
        SIGINT, // (<Ctrl+C>)
        SIGTERM, // (quit)
        SIGQUIT, // (quit + coredump)
        SIGILL, // (illegal instruction)
        SIGFPE, // (FPU or ALU error)
        SIGSEGV, // (segmentation fault)
        SIGSTKFLT, // (stack fault)
        SIGSYS, // (illegal syscall)
    };

    ARES_log(glog, Trace, "POSIX: Registering signal handlers");
    for(int signalToCatch : signalsToCatch)
    {
        struct sigaction action;
        memset(&action, 0, sizeof(action));
        action.sa_handler = signalHandler;
        if(sigaction(signalToCatch, &action, nullptr) != 0)
        {
            ARES_log(glog, Error,
                     "POSIX: Failed to register handler for signal %d [%s]",
                     signalToCatch, strsignal(signalToCatch));
        }
    }

    glog.flush();
#endif

    // Add modules
    core.attachModule(intoRef<Module>(new GfxModule()));

    glog.flush();

    // Main loop, run on main thread. This will return only when the main loop is
    // done.
    bool runOk = core.run();

#if ARES_PLATFORM_IS_POSIX
    if(caughtSignal != INT_MAX)
    {
        int sig = caughtSignal.load();
        bool doAbort;
        switch(sig)
        {
        case SIGINT:
        case SIGTERM:
            ARES_log(glog, Info,
                     "SignalHandler: Caught signal %d [%s]",
                     sig, strsignal(sig));
            doAbort = false;
        break;

        default:
            ARES_log(glog, Fatal,
                     "SignalHandler: Caught signal %d [%s], aborting!",
                     sig, strsignal(sig));
            doAbort = true;
        break;
        }

        glog.flush();

        if(doAbort)
        {
            // TODO Print stack trace?
            abort();
        }
    }
#endif

    ARES_log(glog, Info, "Shutdown");
    glog.flush();

    return runOk ? EXIT_SUCCESS : EXIT_FAILURE;
}
