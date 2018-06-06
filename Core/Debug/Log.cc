#include "Log.hh"

#include <algorithm>

namespace Ares
{

Log::Log(size_t messagePoolSize)
    : messagePool_(messagePoolSize)
{
}

Log::~Log()
{
}


void Log::addSink(LogSink sink, void* data)
{
    sinks_.emplace_back(SinkSlot{sink, data});
}

void Log::removeSink(LogSink sink)
{
    auto matchFn = [sink](const SinkSlot& slot) {
        return slot.sink == sink;
    };
    sinks_.erase(std::remove_if(sinks_.begin(), sinks_.end(), matchFn));
}


void Log::flush()
{
    LogMessage* message;
    while(messagesToFlush_.try_dequeue(message))
    {

        // Sink message to all sinks
        for(const SinkSlot& it : sinks_)
        {
            it.sink(message, it.data);
        }

        messagePool_.free(message); // Now the message can be recycled
    }
}

}
