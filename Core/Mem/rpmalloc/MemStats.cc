#include "../MemStats.hh"
#include "Globals.hh"

#include <rpmalloc/rpmalloc.h>

namespace Ares
{

std::atomic<unsigned int> gNAllocs{0}; // Declared in "Globals.hh"

MemStats gMemStats; // Declared in "Globals.hh"


const MemStats& memStats()
{
    // Update stats
    struct rpmalloc_global_statistics_t rpmStats;
    rpmalloc_global_statistics(&rpmStats);

    gMemStats.nAllocs = gNAllocs.load();
    gMemStats.mapped = rpmStats.mapped;
    gMemStats.totalMapped = rpmStats.mapped_total;
    gMemStats.totalUnmapped = rpmStats.unmapped_total;

    // Return updated stats
    return gMemStats;
}

const char* memBackendName()
{
    return "rpmalloc";
}

}
