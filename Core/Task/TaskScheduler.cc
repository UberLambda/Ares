#include "TaskScheduler.hh"

#include <atomic>

namespace Ares
{

TaskScheduler::TaskScheduler(unsigned int nWorkers, unsigned int nFibers, size_t fiberStackSize)
    : nWorkers_(nWorkers),
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
    }

    // Unlock workers and start spinning
    ready_ = true;
}

TaskScheduler::~TaskScheduler()
{
    // Spin down all workers
    running_ = false;
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
}

void TaskScheduler::waitFor(TaskVar& var, TaskVarValue target)
{
    if(var.load() == target)
    {
        // var is already =target, nothing to wait for
        return;
    }

    // FIXME IMPLEMENT: Instead of spinlocking, put local fiber in waiting list
    //                  and start a new one
    while(var.load() != target)
    {
    }
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
    // until `scheduler->running_` is set to `false` - after which the thread
    // will terminate.
    auto workerIndex = scheduler->localWorkerIndex();
    assert((workerIndex != INVALID_INDEX) && "workerLoop() not running inside of a worker");
    auto& workerData = scheduler->workerData_[workerIndex];

    workerData.prevFiber = nullptr;
    workerData.curFiber = scheduler->fibers_.grab();

    Fiber thisFiber;
    thisFiber.switchTo(*workerData.curFiber);

    assert(false && "This should never be reached");
}

void TaskScheduler::fiberFunc(void* data)
{
    auto scheduler = reinterpret_cast<TaskScheduler*>(data);

    auto workerIndex = scheduler->localWorkerIndex();
    assert((workerIndex != INVALID_INDEX) && "fiberFunc() not running inside of a worker");
    auto& workerData = scheduler->workerData_[workerIndex];

    if(workerData.prevFiber)
    {
        // Reset the previous fiber, then free it back into the pool since it's
        // not needed anymore
        *workerData.prevFiber = std::move(Fiber(fiberFunc,
                                                workerData.prevFiber->stack(), workerData.prevFiber->stackSize(),
                                                scheduler));
        scheduler->fibers_.free(workerData.prevFiber);

        workerData.prevFiber = nullptr;
    }

    // Try running a task
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

    if(scheduler->running_)
    {
        // Recurse
        Fiber* thisFiber = workerData.curFiber;
        workerData.prevFiber = thisFiber;
        workerData.curFiber = scheduler->fibers_.grab();
        thisFiber->switchTo(*workerData.curFiber); // This will make the current fiber be freed on the next iteration

        assert(false && "This should never be reached");
    }
}

}
