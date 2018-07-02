#pragma once

#include <stddef.h>
#include <concurrentqueue.h>
#include <atomic>
#include <thread>
#include <list>
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
///
/// Based on the GDC talk "Parallelizing the Naughty Dog engine using fibers"
/// and inspired by the implementation of task_scheduler in FiberTaskingLib
class TaskScheduler
{
    // A returned index that means "invalid".
    static constexpr const size_t INVALID_INDEX = -1;

    // The amount of attempts to grab a free fiber (`lockingGrabFiber()`) after
    // which a deadlock is very likely
    static constexpr const size_t GRAB_DEADLOCK_THRES = 100;


    unsigned int nWorkers_, nFibers_;

    struct TaskSlot
    {
        Task task;
        TaskVar* var = nullptr;
    };
    moodycamel::ConcurrentQueue<TaskSlot> tasks_;

    AtomicPool<Fiber> fibers_;
    FiberStackStore fiberStacks_;

    std::atomic<bool> ready_; // TODO Replace this with a condition_variable
    std::atomic<bool> running_;
    std::thread* workers_;
    struct WaitingFiberSlot
    {
        Fiber* fiber;
        TaskVar* var;
        TaskVarValue target;
    };
    struct WorkerData
    {
        Fiber* curFiber; ///< The fiber that is currently running on this worker.
        Fiber* doneFiber; ///< A fiber to free because it is done.
        Fiber* finalFiber; ///< A "dead end" fiber to switch to when the worker thread is done.
        std::list<WaitingFiberSlot> waitingFibers; ///< The fibers waiting for a var on this worker.
    };
    WorkerData* workerData_;


    /// Keeps attempting to grab a fiber until it succeeds, then returns it.
    /// **ASSERTS** `false` if the number of attempts grabbing a fiber exceeeds `GRAB_DEADLOCK_THRES`
    Fiber* lockingGrabFiber();

    /// Returns the index of the local worker thread, or `INVALID_INDEX` if the
    /// local thread is not a worker thread.
    size_t localWorkerIndex();

    /// The function that each fiber in the scheduler will run: grabs a task,
    /// executes it, switches to itself if the scheduler is still `running_`.
    static void fiberFunc(void* data);

    /// The loop that each worker thread will run.
    static void workerLoop(TaskScheduler* scheduler);

public:
    /// Returns the optimal amount of worker threads for the host machine.
    /// This usually returns `(number of physical threads) - 1` since the main
    /// thread most of the time needs to do different things than run tasks (ex.:
    /// polling system event queues, reading files...)
    static unsigned int optimalNWorkers();

    /// Initializes a task scheduler that will spin `nWorkers` worker threads,
    /// sharing a pool of `nFibers` fibers each with `fiberStackSize` bytes of
    /// stack.
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


    /// Returns the number of worker threads for this scheduler.
    inline unsigned int nWorkers() const
    {
        return nWorkers_;
    }

    /// Returns the number of fibers in the fiber pool of this scheduler.
    inline unsigned int nFibers() const
    {
        return nFibers_;
    }

    /// Returns the size in bytes of the stack of each fiber in the fiber pool of this
    /// scheduler.
    inline unsigned int fiberStackSize() const
    {
        return fiberStacks_.stackSize();
    }
};

}
