#include "TaskScheduler.hh"

#include <atomic>

namespace Ares
{

TaskScheduler::TaskScheduler(unsigned int nWorkers, unsigned int nFibers, size_t fiberStackSize)
    : nWorkers_(nWorkers)
{
    // FIXME IMPLEMENT: Allocate `nFiber` fibers and fiber stacks

    // Spin up all workers
    running_ = true;
    for(unsigned int j = 0; j < nWorkers; j ++)
    {
        workers_.emplace_back(workerLoop, this);
    }
}

TaskScheduler::~TaskScheduler()
{
    // Spin down all workers
    running_ = false;
    for(auto& worker : workers_)
    {
        worker.join();
    }
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

void TaskScheduler::workerLoop(TaskScheduler* scheduler)
{
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
