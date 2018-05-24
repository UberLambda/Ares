#pragma once

#include <stddef.h>
#include <concurrentqueue.h>
#include <vector>
#include <thread>
#include "Task.hh"
#include "TaskVar.hh"
#include "Fiber.hh"
#include "FiberStackStore.hh"
#include "../Base/AtomicPool.hh"
#include "../Base/NumTypes.hh"

namespace Ares
{

/// A scheduler of `Task`s.
/// Schedulers distribute `m` tasks over `n` OS threads so that some of them can
/// be run concurrently.
class TaskScheduler
{

    unsigned int nWorkers_, nFibers_;

    struct TaskSlot
    {
        Task task;
        TaskVar* var = nullptr;
    };
    moodycamel::ConcurrentQueue<TaskSlot> tasks_;

    volatile bool running_;
    std::vector<std::thread> workers_;


    /// The loop that each worker thread will run.
    static void workerLoop(TaskScheduler* scheduler);

public:
    TaskScheduler(unsigned int nWorkers,
                  unsigned int nFibers=200, size_t fiberStackSize=128*1024);
    ~TaskScheduler();

    /// Schedules the given tasks for [later] execution. If `var` is not null,
    /// increments `var` by `n` beforehand (see: `waitFor()`).
    void schedule(const Task* tasks, size_t n, TaskVar* var=nullptr);

    /// Schedules the given task for [later] execution. If `var` is not null,
    /// increments `var` by one beforehand (see: `waitFor()`).
    void schedule(Task task, TaskVar* var=nullptr);

    /// Waits for the value inside `var` to reach `target`. If there is to wait,
    /// the task running on the local thread is suspended and other ones are
    /// executed while waiting (so that CPU cycles are not wasted busy-waiting).
    void waitFor(TaskVar& var, TaskVarValue target=0);
};

}
