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
    : state_(Dead),
      log_(nullptr), scheduler_(nullptr), scene_(nullptr), resourceLoader_(nullptr)
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

    if(log_)
    {
        // Perform one final log flush
        log_->flush();
    }

    // `~FacilitySlot<T>()` invocations will free all resources, but need to mark
    // the alias pointers as now invalid
    log_ = nullptr;
    scheduler_ = nullptr;
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

    // Init important facilities here. See `Core`'s documentation
    // NOTE: Facilities to initialize here are first `addFacility<T>()`ed, then their
    //       alias pointer is retrieved via `facility<T>()`. This way an user can
    //       customize initialization of these facilities; just `addFacility<T>()`ed
    //       them before calling `Core::init()`, and the `addFacility<T>()` calls
    //       in `Core::init()` will do nothing.
    //       This also means that the diagnostics data logged here must be gotten
    //       from the alias pointer itself, not by assuming that the core has in
    //       fact initialized the facilities!

    // Log
    {
        addFacility<Log>(ARES_CORE_LOG_MESSAGE_POOL_CAPACITY);
        log_ = facility<Log>();

        log_->addSink(stderrLogSink, this);
        ARES_log(*log_, Trace,
                 "Log: %u messages in pool", log_->messagePoolSize());

        // FIXME
    }

    ARES_log(*log_, Info, "Init");

    // Task scheduler
    {
        addFacility<TaskScheduler>(TaskScheduler::optimalNWorkers(),
                                   ARES_CORE_SCHEDULER_FIBER_POOL_CAPACITY,
                                   ARES_CORE_SCHEDULER_FIBER_STACK_SIZE);
        scheduler_ = facility<TaskScheduler>();

        ARES_log(*log_, Debug,
                 "Task scheduler: %u worker threads, %u fibers, %.1f KB fiber stacks",
                 scheduler_->nWorkers(), scheduler_->nFibers(),
                 float(scheduler_->fiberStackSize()) / 1024.0f);

        // FIXME If a `TaskScheduler` is added as a facility before the core is
        //       constructed `nWorkers`, `nFibers` or `fiberStackSize` could differ!
        //       Log values queried from `scheduler_` instead
    }

    // Scene
    {
        addFacility<Scene>(ARES_CORE_SCENE_ENTITY_CAPACITY);
        scene_ = facility<Scene>();

        ARES_log(*log_, Debug,
                 "Scene: %lu max entities",
                 scene_->nEntities());
    }

    // ResourceLoader (and its FileStore)
    {
        // FIXME Switchable `fileStore` implementation depending on use
        addFacility<FolderFileStore>(".");
        auto folderFileStore = facility<FolderFileStore>();
        auto fileStore = static_cast<FileStore*>(folderFileStore);

        ARES_log(*log_, Debug,
                 "ResourceLoader: Using FolderFileStore with root %s",
                 folderFileStore->root());

        addFacility<ResourceLoader>(fileStore);
        resourceLoader_ = facility<ResourceLoader>();
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
