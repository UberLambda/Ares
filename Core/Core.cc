#include "Core.hh"

#include <algorithm>
#include "Debug/Log.hh"
#include "Task/TaskScheduler.hh"
#include "Scene/Scene.hh"
#include "Data/FolderFileStore.hh"
#include "Data/ResourceLoader.hh"
#include "Mem/MemStats.hh"
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

    g().log->flush();

    // Detach and halt all modules that are still attached to the core, in reverse
    // order of initialization
    for(auto it = modules_.rbegin(); it != modules_.rend(); it ++)
    {
        detachModule(*it);
    }

    // Perform one final log flush
    g().log->flush();

    // Free GlobalData's resources
    delete g().resLoader;
    delete g().scene;
    delete g().scheduler;
    delete g().log;

    // `~DoubleBuffer<FrameData>()` will free everything else
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
        g().log = new Log(ARES_CORE_LOG_MESSAGE_POOL_CAPACITY);

#define glog (*g().log)

        glog.addSink(stderrLogSink, this);
        ARES_log(glog, Trace,
                 "Log: %u messages in pool", glog.messagePoolSize());

        // FIXME
    }

    ARES_log(glog, Info, "Init");

    // Mem
    {
        ARES_log(glog, Debug,
                 "Memory allocator: %s",
                 memBackendName());
    }

    // Task scheduler
    {
        g().scheduler = new TaskScheduler(TaskScheduler::optimalNWorkers(),
                                          ARES_CORE_SCHEDULER_FIBER_POOL_CAPACITY,
                                          ARES_CORE_SCHEDULER_FIBER_STACK_SIZE);

        ARES_log(glog, Debug,
                 "Task scheduler: %u worker threads, %u fibers, %.1f KB fiber stacks",
                 g().scheduler->nWorkers(), g().scheduler->nFibers(),
                 float(g().scheduler->fiberStackSize()) / 1024.0f);

        // FIXME If a `TaskScheduler` is added as a facility before the core is
        //       constructed `nWorkers`, `nFibers` or `fiberStackSize` could differ!
        //       Log values queried from `scheduler_` instead
    }

    // Scene
    {
        g().scene = new Scene(ARES_CORE_SCENE_ENTITY_CAPACITY);

        ARES_log(glog, Debug,
                 "Scene: %lu max entities",
                 g().scene->maxEntities());
    }

    // ResourceLoader (and its FileStore)
    {
        // FIXME Switchable `fileStore` implementation depending on use
        auto folderFileStore = new FolderFileStore("Resources");
        Ref<FileStore> fileStore = intoRef<FileStore>(folderFileStore);

        ARES_log(glog, Debug,
                 "ResourceLoader: Using FolderFileStore with root %s",
                 folderFileStore->root());

        g().resLoader = new ResourceLoader(std::move(fileStore));
    }

    // [Re]initialize all modules that were attached to the core before it was
    // `init()`ed
    for(auto it = modules_.begin(); it != modules_.end();)
    {
        bool itInited = initModule(it->get());
        if(itInited)
        {
            it ++;
        }
        else
        {
            // Erase uninited module
            it = modules_.erase(it);
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
    ARES_log(glog, Info, "Running");

    // Main loop
    TaskVar frameVar{0};
    while(state_ == Running)
    {
        // Schedule the update tasks for each module that can run on worker
        // threads; use `frameVar` as counter
        for(auto& module : modules_)
        {
            Task updateTask = module->updateTask(*this);
            if(updateTask)
            {
                g().scheduler->schedule(updateTask, &frameVar);
            }
        }

        // Update everything that has to be updated on the main thread for each
        // module; in the meantime, the worker tasks scheduled earlier are being
        // executed in the background...
        for(auto& module : modules_)
        {
            module->mainUpdate(*this);
        }

        // TODO: Do main thread stuff that HAS to be done once per frame here
        // (core file I/O, ...)

        // Flush a bunch of this frame's log messages here.
        // This amount of messages to be flushed should be enough to make sure
        // that the message buffer is always empty afterwards (so that we don't
        // run out of log messages in the log's message pool...)
        glog.flush(ARES_CORE_LOG_MESSAGE_POOL_CAPACITY);

        // Wait for all module update tasks to finish...
        while(frameVar.load() != 0)
        {
            // TODO: Do something more useful, don't just spinlock
        }

        // Clear the previous frame's data and swap current and previous frame
        // data. Events/commands that were in `current()` will now be in `past()`
        // for the next frame, ready to be processed; `current()` will be blank,
        // ready to be filled with new data.
        frameData_.past().clear();
        frameData_.swap();
    }

    ARES_log(glog, Info, "Done running");
    glog.flush();
    return true;
}

void Core::halt()
{
    auto nextState = state_ != Dead ? Inited : Dead;
    state_ = nextState;
}


bool Core::initModule(Module* module)
{
    if(!module)
    {
        ARES_log(glog, Error, "Attempting to init a null module");
        return false;
    }

    ARES_log(glog, Trace,
             "Initializing module @%p", module);


    if(module->init(*this))
    {
        ARES_log(glog, Trace,
                 "Attaching module @%p", module);
        return true;
    }
    else
    {
        ARES_log(glog, Error,
                 "Failed to attach module @%p, initalization error!", module);
        return false;
    }
}

bool Core::attachModule(Ref<Module> module)
{
    if(!module || std::find(modules_.begin(), modules_.end(), module) != modules_.end())
    {
        return false;
    }

    bool ok = true;
    if(state_ != Dead)
    {
        // Core already inited/running, initialize module here
        ok = initModule(module.get()); // (may fail, logs errors)
    }

    if(ok)
    {
        modules_.push_back(module); // (increases refcount)
        return true;
    }
    else
    {
        return false;
    }
}

bool Core::detachModule(Ref<Module> module)
{
    auto it = std::find(modules_.begin(), modules_.end(), module);
    if(it == modules_.end())
    {
        return false;
    }

    ARES_log(glog, Trace,
             "Halting and detaching module @%p", it->get());

    (*it)->halt(*this);
    modules_.erase(it);
    return true;
}


}
