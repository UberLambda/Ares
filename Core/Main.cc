#include <atomic>
#include "Task/TaskScheduler.hh"
#include "Debug/Log.hh"
#include "Base/Platform.h"

using namespace Ares;


#if ARES_PLATFORM_IS_POSIX
#include <signal.h>
#include <stdlib.h>

static struct
{
    Log* log;
    std::atomic<bool>* running;

} signalData;

/// Set to `true` if a signal was previously caught; in that case, terminate
/// immediately
static std::atomic<bool> signalCaught{false};

static void signalHandler(int signal)
{
    if(signalCaught)
    {
        // A signal was already caught; do not attempt to do anything else.
        // This is to make sure that this handler does not cause other signals
        // to be raised, causing a potentially infinite recursion.
        return;
    }
    signalCaught = true;

    // === Signal handler: flush log, print debug info, try to cleanup, and abort program ===

    auto& log = *signalData.log;
    log.flush();

    switch(signal)
    {
    case SIGINT:
        // <Ctrl+C>: graceful termination
        ARES_log(log, Info,
                 "SignalHandler: Caught SIGINT, exiting");
        log.flush();

        signalData.running->store(false);
    break;

    default:
        // Anything else: abort
        ARES_log(log, Fatal,
                 "SignalHandler: Caught signal %d [%s], aborting!", signal, strsignal(signal));
        log.flush();

        // TODO Print stack trace?
        abort();
    break;
    };
}

#endif

static void stderrLogSink(const LogMessage* message, void* data)
{
    static constexpr const char levelChars[] = { 'T', 'D', 'I', 'W', 'E', 'F' };

    char levelChar = levelChars[message->level];
    fprintf(stderr, "[%c]: %s:%u: %s\n",
            levelChar, message->sourceFile, message->sourceLine,
            message->content);
}


int main(int argc, char** argv)
{
    Log log;
    log.addSink(stderrLogSink);

    std::atomic<bool> running{true};

#if ARES_PLATFORM_IS_POSIX
    static constexpr const int signalsToCatch[] =
    {
        SIGINT, // (<Ctrl+C>)
        SIGQUIT, // (quit + coredump)
        SIGILL, // (illegal instruction)
        SIGFPE, // (FPU or ALU error)
        SIGSEGV, // (segmentation fault)
        SIGSTKFLT, // (stack fault)
        SIGSYS, // (illegal syscall)
    };

    ARES_log(log, Trace, "POSIX: Registering signal handlers");
    signalData =
    {
        .log = &log,
        .running = &running,
    };
    for(int signalToCatch : signalsToCatch)
    {
        signal(signalToCatch, signalHandler);
    }
#endif

    ARES_log(log, Info, "Startup");
    log.flush();

    auto nWorkers = TaskScheduler::optimalNWorkers();
    ARES_log(log, Debug, "Task scheduler: Using %u worker threads", nWorkers);
    log.flush();

    TaskScheduler scheduler(nWorkers);

    ARES_log(log, Info, "Entering main loop");
    log.flush();


    TaskVar frameVar{0};

    while(running)
    {
        // TODO: Schedule frame tasks here, put `frameVar` as var

        // TODO: Do main thread stuff that HAS to be done once per frame here
        // (poll events, swap buffers...)

        // Do other, non-essential stuff that can be done while we wait for the
        // frame tasks to finish; note that they could actually have finished
        // already!
        while(frameVar.load() != 0)
        {
            // TODO: Do something, don't spinlock
        }

        // Flush all of this frame's log messages here.
        // No more messages can be enqueued until next frame, so this operation
        // is guaranteed to finish at some point.
        log.flush();
    }


    ARES_log(log, Info, "Shutdown");
    log.flush();

    return 0;
}
