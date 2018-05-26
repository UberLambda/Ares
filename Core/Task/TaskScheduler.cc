#include "TaskScheduler.hh"

#include <atomic>

namespace Ares
{

TaskScheduler::TaskScheduler(unsigned int nWorkers, unsigned int nFibers, size_t fiberStackSize)
    : nWorkers_(nWorkers)
{
    // FIXME IMPLEMENT: Allocate `nFiber` fibers and fiber stacks

    workers_ = new std::thread[nWorkers_];
    workersData_ = new WorkerData[nWorkers_];

    // Initialize all workers
    for(unsigned int i = 0; i < nWorkers_; i ++)
    {
        workers_[i] = std::thread(workerLoop, this);
    }

    // Unlock workers and start spinning
    running_ = true;
    ready_.notify_one();
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
    delete[] workersData_; workersData_ = nullptr;
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
    for(size_t i = 0; i < nWorkers_; i ++)
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
    std::unique_lock<std::mutex> readyLock(scheduler->readyMutex_);
    scheduler->ready_.wait(readyLock);

    // Main loop
    while(scheduler->running_)
    {
        TaskSlot taskSlot;
        if(scheduler->tasks_.try_dequeue(taskSlot))
        {
            // Popped a task from the queue; run it...
            // FIXME IMPLEMENT: Assign task to the local fiber and run it from there
            taskSlot.task.func(scheduler, taskSlot.task.data);

            // ...then decrement its var when done if any
            if(taskSlot.var)
            {
                // Decrement var by 1 atomically
                std::atomic_fetch_sub(taskSlot.var, TaskVarValue(1));
            }
        }
    }
}

}
