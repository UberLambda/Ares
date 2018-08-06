#include "Profiler.hh"

#include <tinyformat.h>

namespace Ares
{

Profiler::Profiler()
    : timeEvents_(), timeEventsConsumer_(timeEvents_)
{
}

Profiler::~Profiler()
{
}

size_t Profiler::flush(std::vector<TimeEvent>& events)
{
#ifdef ARES_ENABLE_PROFILER
    size_t oldSize = events.size();
    size_t n = timeEvents_.size_approx();

    events.resize(oldSize + n);

    TimeEvent* it = &events[oldSize]; // The first event to write in `events`
    return timeEvents_.try_dequeue_bulk(timeEventsConsumer_, it, n);

#else
    return 0;
#endif
}

}
