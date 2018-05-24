#pragma once

#include "Task.hh"
#include "TaskVar.hh"

namespace Ares
{

/// A scheduler of `Task`s.
/// Schedulers distribute `m` tasks over `n` OS threads so that some of them can
/// be run concurrently.
class TaskScheduler
{
public:
    /// Schedules the given task for [later] execution. If `var` is not null,
    /// increments `var` by one beforehand (see: `waitFor()`).
    void schedule(Task task, TaskVar* var=nullptr);

    /// Waits for the value inside `var` to reach zero. If there is to wait,
    /// the task running on the local thread is suspended and other ones are
    /// executed while waiting (so that CPU cycles are not wasted busy-waiting).
    void waitFor(TaskVar& var);
};

}
