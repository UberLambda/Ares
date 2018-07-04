#include "../MemStats.hh"
#include "Globals.hh"

namespace Ares
{

std::atomic<unsigned int> gNAllocs{0}; // Declared in "Globals.hh"

MemStats gMemStats; // Declared in "Globals.hh"


const MemStats& memStats()
{
    // Update stats
    gMemStats.nAllocs = gNAllocs.load();
    // TODO IMPLEMENT Every other MemStats field for stdlib mem backend is unimplemented

    // Return updated stats
    return gMemStats;
}

const char* memBackendName()
{
    return "stdlib";
}

}
