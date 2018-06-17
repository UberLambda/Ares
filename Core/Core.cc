#include "Core.hh"

#include "Debug/Log.hh"
#include "Task/TaskScheduler.hh"
#include "Scene/Scene.hh"
#include "CoreConfig.h"

namespace Ares
{

Core::Core()
    : state_(Dead)
{
}

Core::~Core()
{
    if(state_ == Running)
    {
        halt();
    }

    state_ = Dead;

    if(log_)
    {
        // Perform one final log flush
        log_->flush();
    }

    // Smart pointers will free all resources
}


/// A basic log sink that output to `stderr`.
static void stderrLogSink(const LogMessage* message, void* data)
{
    static constexpr const char levelChars[] = { 'T', 'D', 'I', 'W', 'E', 'F' };

    char levelChar = levelChars[message->level];
    fprintf(stderr, "[%c]: %s:%u: %s\n",
            levelChar, message->sourceFile, message->sourceLine,
            message->content);
}


bool Core::init()
{
    if(state_ == Inited)
    {
        // Nothing to do
        return true;
    }

    // Log
    {
        size_t nMessagesInPool = ARES_CORE_LOG_MESSAGE_POOL_CAPACITY;

        log_.reset(new Log(nMessagesInPool));
        log_->addSink(stderrLogSink, this);

        ARES_log(*log_, Trace, "Log: %u messages in pool", nMessagesInPool);
    }

    ARES_log(*log_, Info, "Init");

    // Task scheduler
    {
        unsigned int nWorkers = TaskScheduler::optimalNWorkers();
        unsigned int nFibers = ARES_CORE_SCHEDULER_FIBER_POOL_CAPACITY;
        size_t fiberStackSize = ARES_CORE_SCHEDULER_FIBER_STACK_SIZE;
        ARES_log(*log_, Debug,
                 "Task scheduler: %u worker threads, %u fibers, %.1f KB fiber stacks",
                 nWorkers, nFibers, float(fiberStackSize) / 1024.0f);

        scheduler_.reset(new TaskScheduler(nWorkers, nFibers, fiberStackSize));
    }

    // Scene
    {
        size_t maxEntities = ARES_CORE_SCENE_ENTITY_CAPACITY;
        ARES_log(*log_, Debug,
                 "Scene: %lu entities maximum", maxEntities);
        scene_.reset(new Scene(ARES_CORE_SCENE_ENTITY_CAPACITY));
    }

    // TODO On error, print some description of it, flush the log, and return `false`

    state_ = Inited;
    return true;
}

bool Core::run()
{
    if(state_ != Inited)
    {
        // Can't run a dead or already-running core
        return false;
    }

    state_ = Running;
    ARES_log(*log_, Info, "Running");

    // Main loop
    TaskVar frameVar{0};
    while(state_ == Running)
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
        log_->flush();
    }

    ARES_log(*log_, Info, "Done running");
    log_->flush();
    return true;
}

void Core::halt()
{
    auto nextState = state_ != Dead ? Inited : Dead;
    state_ = nextState;
}

}
