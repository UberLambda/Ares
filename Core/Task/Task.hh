#pragma once

namespace Ares
{

class TaskScheduler;

/// A function to be run when a `Task` is run.
using TaskFunc = void(*)(TaskScheduler* scheduler, void* data);

/// An atomic task to execute.
struct Task
{
    TaskFunc func = nullptr; ///< The function to be run when the task is run.
    void* data = nullptr; ///< Some data to pass to `func`.
};

}
