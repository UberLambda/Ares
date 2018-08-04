#include "Profiler.hh"

#include <tinyformat.h>

namespace Ares
{

Profiler::Profiler()
{
}

Profiler::~Profiler()
{
}

ErrString Profiler::connect(const std::string& address)
{
   // FIXME IMPLEMENT Initialize a WebSocket server bound to `address` that will
   //       send data to clients on `flush()`
   return "Unimplemented";
}


void Profiler::flush()
{
#ifdef ARES_ENABLE_PROFILER
    // Write each header to socket
    TimeEvent event;
    while(timeEvents_.try_dequeue(event))
    {
        // FIXME IMPLEMENT Send the event to profiling clients via WebSocket
    }
#endif
}

}
