#include "TaskScheduler.hh"

#include <atomic>

namespace Ares
{

unsigned int TaskScheduler::optimalNWorkers()
{
    auto nHardwareThreads = std::thread::hardware_concurrency();
    return nHardwareThreads > 1 ? nHardwareThreads - 1 : 1;
}


TaskScheduler::TaskScheduler(unsigned int nWorkers, unsigned int nFibers, size_t fiberStackSize)
    : nWorkers_(nWorkers), nFibers_(nFibers),
      fiberStacks_(nFibers, fiberStackSize)
{
    workers_ = new std::thread[nWorkers_];
    workerData_ = new WorkerData[nWorkers_];

    // Initialize all fibers by giving them a stack and pointing them to execute `fiberFunc`
    auto fiberGenFunc = [this](size_t fiberIndex)
    {
        auto fiberStack = fiberStacks_[fiberIndex];
        return Fiber(fiberFunc, fiberStack, fiberStacks_.stackSize(), this);
    };
    fibers_ = AtomicPool<Fiber>(nFibers, fiberGenFunc);

    // Initialize all workers
    running_ = true;
    ready_ = false;

    for(unsigned int j = 0; j < nWorkers_; j ++)
    {
        workers_[j] = std::thread(workerLoop, this);
        // FIXME IMPLEMENT IMPORTANT Pin thread to physical thread for [much] better performance
    }

    // Unlock workers and start spinning
    ready_ = true;
}

TaskScheduler::~TaskScheduler()
{
    // Spin down all workers
    // If there were any workers sleeping waiting for `condVar_`, notify them;
    // they will stop sleeping since `running_` is now `false`
    running_ = false;
    sleepingCond_.notify_all();

    for(std::thread* worker = workers_; worker != workers_ + nWorkers_; worker ++)
    {
        worker->join();
    }

    // Free memory
    delete[] workers_; workers_ = nullptr;
    delete[] workerData_; workerData_ = nullptr;
}


void TaskScheduler::schedule(Task task, TaskVar* var)
{
    schedule(&task, 1, var);
}

void TaskScheduler::schedule(const Task* tasks, size_t n, TaskVar* var)
{
    if(n == 0)
    {
        // No tasks to add
        return;
    }

    if(var)
    {
        // Increment var by n atomically
        std::atomic_fetch_add(var, TaskVarValue(n));
    }

    // Enqueue all <tasks, var> pairs
    for(auto it = tasks; it != tasks + n; it ++)
    {
        tasks_.enqueue({*it, var});
    }

    // If any thread is `wait()`ing on `sleepingCond_` for a task to be added to
    // the queue, it will get notified of the new task[s] being added; otherwise,
    // the `notify_all()` will simply be ignored.
    sleepingCond_.notify_all();
}

void TaskScheduler::waitFor(TaskVar& var, TaskVarValue target)
{
    if(var.load() == target)
    {
        // var is already =target, nothing to wait for
        return;
    }

    auto workerIndex = localWorkerIndex();
    if(workerIndex != INVALID_INDEX)
    {
        // `waitFor()` was called from a worker: put the current fiber to sleep
        // and start executing other tasks
        // IMPORTANT This code is heavily simplified by the assumption that the
        //           waiting list is per-worker and not shared. Pushing waiting
        //           fibers to the waiting list, switching to them and returning
        //           to the previous fiber are hence all guaranteed to always
        //           "just work" without any atomics/locking since no other
        //            thread can get in the way!
        auto& workerData = workerData_[workerIndex];

        WaitingFiberSlot waitingFiberSlot =
        {
            .fiber = workerData.curFiber, // The current fiber is the one that has to sleep...
            .var = &var, // ...until `var`...
            .target = target //...reaches `target`
        };
        workerData.waitingFibers.push_back(waitingFiberSlot);

        // Then we switch to a new fiber while we wait
        workerData.curFiber = lockingGrabFiber();
        waitingFiberSlot.fiber->switchTo(*workerData.curFiber);

        // At some point we will get back here because `curFiber` switched to
        // `waitingFiberSlot.fiber` because the slot's var reached its target
        // `curFiber` can then be **reset and** freed, we're actually running on `waitingFiber`
        // now!
        // The fiber has to be reset since fibers from the pool are expected to
        // all point to `fiberFunc`.
        Fiber* waitingFiber = waitingFiberSlot.fiber;

        Fiber* evictedFiber = workerData.curFiber;
        *evictedFiber = Fiber(fiberFunc, evictedFiber->stack(), evictedFiber->stackSize(), this);

        bool evictedFiberFreed = fibers_.free(evictedFiber);
        assert(evictedFiberFreed && "Could not free waiting fiber");

        workerData.curFiber = waitingFiber; // Current fiber is now what was the waiting fiber!

        // `waitFor()` will return here and the task will keep running on `curFiber`
    }
    else
    {
        // `waitFor()` was called from another thread: spinlock
        // FIXME Replace this with something more sensible!
        //       (Would the system work better if the list of waiting fibers was
        //       a shared lockless list instead of being a private per-worker list?)
        while(var.load() != target)
        {
        }
    }
}


Fiber* TaskScheduler::lockingGrabFiber()
{
    Fiber* fiber = nullptr;
    for(size_t nAttempts = 0; !fiber; nAttempts ++)
    {
        fiber = fibers_.grab();

        if(nAttempts > GRAB_DEADLOCK_THRES)
        {
            // TODO Log and `abort()` even in release?
            assert(false && "Deadlock: all fibers are grabbed");
        }
    }

    return fiber;
}

size_t TaskScheduler::localWorkerIndex()
{
    for(unsigned int i = 0; i < nWorkers_; i ++)
    {
        if(std::this_thread::get_id() == workers_[i].get_id())
        {
            // Found this worker thread
            return i;
        }
    }

    // This thread is not a worker thread
    return INVALID_INDEX;
}

void TaskScheduler::workerLoop(TaskScheduler* scheduler)
{
    // Wait for all worker threads to be ready
    // TODO Make this into a condition variable instead
    while(!scheduler->ready_)
    {
    }

    // Grab a a fiber and make it as the initial "scheduler fiber" for the local worker.
    // After switching, the scheduler fiber will continue to recurse into itself
    // until `scheduler->running_` is set to `false`, after which the fiber will
    // terminate - and the worker thread with it
    auto workerIndex = scheduler->localWorkerIndex();
    assert((workerIndex != INVALID_INDEX) && "workerLoop() not running inside of a worker");
    auto& workerData = scheduler->workerData_[workerIndex];

    Fiber* startFiber = scheduler->lockingGrabFiber();
    workerData.curFiber = startFiber;
    workerData.doneFiber = nullptr;

    Fiber localFiber;
    workerData.finalFiber = &localFiber;
    localFiber.switchTo(*startFiber);

    // When we reach here, the worker loop is done because `finalFiber` was resumed.
    // The worker thread will terminate here.
}

void TaskScheduler::fiberFunc(void* data)
{
    auto scheduler = reinterpret_cast<TaskScheduler*>(data);

    auto workerIndex = scheduler->localWorkerIndex();
    assert((workerIndex != INVALID_INDEX) && "fiberFunc() not running inside of a worker");
    auto& workerData = scheduler->workerData_[workerIndex];

    // Wake up the first fiber that is done waiting
    for(auto it = workerData.waitingFibers.begin(); it != workerData.waitingFibers.end(); it ++)
    {
        if(it->var->load() == it->target)
        {
            // Fiber is done waiting, resume it
            Fiber* localFiber = workerData.curFiber;
            Fiber* waitingFiber = it->fiber;

            workerData.waitingFibers.erase(it);
            localFiber->switchTo(*waitingFiber);
        }
    }

    // After that try running a task
    TaskSlot taskSlot;
    if(scheduler->tasks_.try_dequeue(taskSlot))
    {
        // Actually run the task
        // Note that this could invoke `scheduler->waitFor()` and this fiber could
        // stop running at some point to be resumed later!
        taskSlot.task.func(scheduler, taskSlot.task.data);

        if(taskSlot.var)
        {
            // Then atomically decrement its var when done, if any
            std::atomic_fetch_sub(taskSlot.var, TaskVarValue(1));
        }
    }
    else
    {
        // No more tasks. Lock (sleep) until any new task is scheduled or `running_`
        // is set to false to lower the CPU consumption.
        std::unique_lock<std::mutex> sleepLock(scheduler->sleepingMutex_);
        while(scheduler->running_.load()
              && scheduler->tasks_.size_approx() == 0)
        {
            scheduler->sleepingCond_.wait(sleepLock);
        }
    }

    if(!scheduler->running_)
    {
        // The scheduler is done running. Make this fiber switch to a "dead end"
        // one: the worker thread will die with it
        // **DO NOT SIMPLY RETURN HERE!**; FTL's `boost::context` code calls
        // `exit(0)` if you don't explicitly exit from a fiber!
        workerData.curFiber->switchTo(*workerData.finalFiber);
    }

    // Else we need to recurse back into `fiberFunc()`.
    // You could just mark `curFiber` as done, grab another fiber from the pool
    // and switch to it but that would require an `AtomicPool::grab()` per
    // iteration, which is an expensive operation. It's better to instead try to
    // recycle `doneFiber` (after all it was to be put in the fiber pool again!);
    // you can just reset `doneFiber` to point to `fiberFunc()` and mark it as the
    // new `curFiber`, then switch to it in a ping-pong fashion.
    // If we do not in fact have a `doneFiber` (but this happens rarely) then we
    // can just grab a fresh one from the pool and switch to that.

    Fiber* localFiber = workerData.curFiber;
    Fiber* targetFiber = nullptr;
    if(workerData.doneFiber)
    {
        // Reset the done fiber and recycle it
        targetFiber = workerData.doneFiber;
        *targetFiber = Fiber(fiberFunc, targetFiber->stack(), targetFiber->stackSize(), scheduler);
    }
    else
    {
        // Grab a new fiber from the pool
        // No need to reset it since fibers in the pool are always ready to go
        // (i.e. set to point to `fiberFunc`)
        targetFiber = scheduler->fibers_.grab();
    }

    workerData.curFiber = targetFiber;
    workerData.doneFiber = localFiber;
    localFiber->switchTo(*targetFiber);
}

}
