#pragma once

#include <stddef.h>
#include <Core/Base/NumTypes.hh>
#include <Core/Task/TaskScheduler.hh>

namespace Ares
{

/// The arguments required for spawning a reader task. See `ioReaderTask()`.
struct IOReadArgs
{
    /// The path to the file to write.
    /// **WARNING**: Will trigger an assert if null!
    const char* path = nullptr;

    /// The task function that the "done task" will execute. This function will be
    /// passed a pointer to an `IOReadResult` as data argument.
    /// **IMPORTANT**: Will trigger an assert if null!
    TaskFunc doneTaskFunc = nullptr;

    /// Some data to be passed to the "done task"; it will be accessible from its
    /// `IOReadResult`'s `args` field.
    void* doneTaskData = nullptr;
};

/// The result that the "done task" of a reader task will receive. See `ioReaderTask()`.
struct IOReadResult
{
    /// The args the reader task was spawned with.
    const IOReadArgs* args;

    /// `true` if the file could be read, `false` otherwise.
    bool successful;

    /// A `malloc()`'d memory region contained the data read from the file.
    /// Null if the read was unsuccessful.
    ///
    /// The memory is normally automatically `free()`d after the "done task" is run;
    /// to prevent this from happening, set `data` to null and move it around freely
    /// - but note that you will have to `free()` it up yourself afterwards!
    U8* data;

    /// The size in bytes of the data read.
    /// Zero if the read was unsuccessful.
    size_t dataSize;
};

/// Creates a new reader task that, when run by a scheduler, will read a file and
/// then spawn and wait for a "done task" with a `IOReadResult` argument.
/// **WARNING**: The `args` pointer must stay valid throughout the whole lifetime
///              of the spawned task - including the "done task"!!
///              If you do not `waitFor()` the reader task before returning from
///              the function that will schedule it, consider allocating the
///              `IOReadArgs` on the heap.
///
/// **ASSERTS**: `args != nullptr`, `args.path`, `args.doneTaskFunc != nullptr`
Task ioReaderTask(const IOReadArgs* args);


/// The arguments required for spawning a reader task. See `ioReaderTask()`.
struct IOWriteArgs
{
    /// The path to the file to write.
    /// **WARNING**: Will trigger an assert if null!
    const char* path = nullptr;

    /// The data to write to the file. If null, the file will be created/truncated
    /// but no data will be written to it.
    const U8* data = nullptr; //< If data is null, the file will be created but no data written

    /// The size in bytes of the data to write. If zero, the file will be created/truncated
    /// but no data will be written to it.
    size_t dataSize = 0;

    /// The task function that the "done task" will execute; if no done task is
    /// to execute, set it to null. This function - if present - will be passed
    /// a pointer to an `IOWriteResult` as data argument.
    TaskFunc doneTaskFunc = nullptr;

    /// Some data to be passed to the "done task"; it will be accessible from its
    /// `IOReadResult`'s `args` field.
    void* doneTaskData = nullptr;
};

/// The result that the "done task" of a writer task will receive - if any "done task"
/// is to be run anyways. See `ioWriterTask()`.
struct IOWriteResult
{
    /// The args the writer task was spawned with.
    const IOWriteArgs* args;

    /// `true` if the file could be written to, `false` otherwise.
    bool successful;
};

/// Creates a new writer task that, when run by a scheduler, will [over]write a
/// file and - if a `args.doneTaskFunc` is supplied - then spawn and wait for a "done task"
/// with a `IOWriteResult` argument.
/// **WARNING**: The `args` pointer must stay valid throughout the whole lifetime
///              of the spawned task - including the "done task" if any!!
///              If you do not `waitFor()` the writer task before returning from
///              the function that will schedule it, consider allocating the
///              `IOWriteArgs` on the heap.
///
/// **ASSERTS**: `args != nullptr`, `args.path`
Task ioWriterTask(const IOWriteArgs* args);

}
