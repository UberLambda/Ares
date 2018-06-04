#include "FileIO.hh"

#include <stdlib.h>
#include <assert.h>
#include <fstream>

namespace Ares
{

static void readerFunc(TaskScheduler* scheduler, void* data)
{
    auto args = reinterpret_cast<const IOReadArgs*>(data);

    IOReadResult result = // (initially set as "unsuccessful")
    {
        .args = args,
        .successful = false,
        .data = nullptr,
        .dataSize = 0,
    };

    std::ifstream stream(args->path, std::ios::in | std::ios::binary);
    if(stream)
    {
        // Stream is valid, read away
        stream.seekg(0, std::ios::end);
        result.dataSize = stream.tellg();
        stream.seekg(0, std::ios::beg);

        result.data = (U8*)malloc(result.dataSize);
        stream.read(reinterpret_cast<char*>(result.data), result.dataSize);

        result.successful = true;
    }
    // Else result will stay unchanged, marking a failure in reading the file

    // Spawn and wait for `doneTask`
    TaskVar waitVar{0};
    Task doneTask = {args->doneTaskFunc, &result};
    scheduler->schedule(doneTask, &waitVar);
    scheduler->waitFor(waitVar);

    // Free result
    // NOTE: The user can swap out `result.data` for a null pointer for having the
    //       data moved elsewhere - `free()` will do nothing in this case
    free(result.data);
}

Task ioReaderTask(const IOReadArgs* args)
{
    assert(args && "Missing args");
    assert(args->path && "Missing path");
    assert(args->doneTaskFunc && "Missing done task func");

    return {readerFunc, (void*)args};
}


static void writerFunc(TaskScheduler* scheduler, void* data)
{
    auto args = reinterpret_cast<const IOWriteArgs*>(data);

    IOWriteResult result = // (initially set as "unsuccessful")
    {
        .args = args,
        .successful = false,
    };

    std::ofstream stream(args->path, std::ios::out | std::ios::binary);
    if(stream)
    {
        // Stream is valid, write away

        if(args->data && args->dataSize)
        {
            // Actually need to write some data instead of just `touch`ing the file
            stream.write(reinterpret_cast<const char*>(args->data), args->dataSize);
        }

        result.successful = true;
    }
    // Else result will stay unchanged, marking a failure in reading the file

    // Spawn and wait for a done task if any was requested to be scheduled
    if(args->doneTaskFunc)
    {
        TaskVar waitVar{0};
        Task doneTask = {args->doneTaskFunc, &result};
        scheduler->schedule(doneTask, &waitVar);
        scheduler->waitFor(waitVar);
    }
}

Task ioWriterTask(const IOWriteArgs* args)
{
    assert(args && "Missing args");
    assert(args->path && "Missing path");

    return {writerFunc, (void*)args};
}

}
