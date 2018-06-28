#include "Core.hh"

#include <algorithm>
#include "Debug/Log.hh"
#include "Task/TaskScheduler.hh"
#include "Scene/Scene.hh"
#include "Data/FolderFileStore.hh"
#include "Data/ResourceLoader.hh"
#include "Module/Module.hh"
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
    else if(state_ == Dead)
    {
        return;
    }

    state_ = Dead;

    log_->flush();

    // Detach and halt all modules that are still attached to the core
    for(Module* module : modules_)
    {
        detachModule(module);
    }

    // Free all resources still in the loader
    size_t nCleanedRes = resourceLoader_->cleanup();
    ARES_log(*log_, Debug,
             "ResourceLoader: %lu resources still loaded on shutdown, cleaned up",
             nCleanedRes);

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

    // Resource loader
    {
        // TODO IMPORTANT Use an archive format/a PhysFS-like `FileStore` instead of raw files!
        Path rootAssetPath = ".";
        ARES_log(*log_, Debug,
                 "FileStore: FolderFileStore(root=\"%s\")", rootAssetPath);
        fileStore_.reset(new FolderFileStore(rootAssetPath));
        resourceLoader_.reset(new ResourceLoader(fileStore_.get()));
    }

    // [Re]initialize all modules that were attached to the core before it was
    // `init()`ed
    for(Module* module : modules_)
    {
        if(!initModule(module))
        {
            // The initialization of a module failed
            return false;
        }
    }

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
        // Schedule the update tasks for each module that can run on worker
        // threads; use `frameVar` as counter
        for(Module* module : modules_)
        {
            scheduler_->schedule(module->updateTask(*this), &frameVar);
        }

        // Update everything that has to be updated on the main thread for each
        // module; in the meantime, the worker tasks scheduled earlier are being
        // executed in the background...
        for(Module* module : modules_)
        {
            module->mainUpdate(*this);
        }

        // TODO: Do main thread stuff that HAS to be done once per frame here
        // (core file I/O, ...)

        // Flush a bunch of this frame's log messages here.
        // This amount of messages to be flushed should be enough to make sure
        // that the message buffer is always empty afterwards (so that we don't
        // run out of log messages in the log's message pool...)
        log_->flush(ARES_CORE_LOG_MESSAGE_POOL_CAPACITY);

        // Wait for all module update tasks to finish...
        while(frameVar.load() != 0)
        {
            // TODO: Do something more useful, don't just spinlock
        }
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


bool Core::initModule(Module* module)
{
    if(module->init(*this))
    {
        ARES_log(*log_, Trace,
                 "Attaching module @%p", module);
        return true;
    }
    else
    {
        ARES_log(*log_, Error,
                 "Failed to attach module @%p, initalization error!", module);
        return false;
    }
}

bool Core::attachModule(Module* module)
{
    if(!module || std::find(modules_.begin(), modules_.end(), module) != modules_.end())
    {
        return false;
    }

    if(state_ != Dead)
    {
        // Core already inited/running, initialize module here
        (void)initModule(module); // (may fail, logs errors)
    }

    modules_.push_back(module);
    return true;
}

bool Core::detachModule(Module* module)
{
    auto it = std::find(modules_.begin(), modules_.end(), module);
    if(it == modules_.end())
    {
        return false;
    }

    ARES_log(*log_, Trace,
             "Halting and detaching module @%p", *it);

    (*it)->halt(*this);
    modules_.erase(it);
    return true;
}


AutoDetach::~AutoDetach()
{
    if(module_)
    {
        core_->detachModule(module_);
        delete module_; module_ = nullptr;
    }
}


}
