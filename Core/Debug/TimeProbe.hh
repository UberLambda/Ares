#pragma once

#include <thread>
#include <chrono>
#include <string>
#include <utility>
#include <Ares/BuildConfig.h>
#include <Core/Api.h>
#include <Core/Debug/Profiler.hh>

namespace Ares
{

/// A time probe used to profile the time taken by the code in a scope to execute.
///
/// Does nothing `#ifndef ARES_ENABLE_PROFILING`.
class ARES_API TimeProbe
{
private:
    Profiler& profiler_;
    Profiler::TimeEvent event_;

public:
    /// Creates a profiler time probe named `name` for this scope.
    /// **WARNING** `name` should be a pointer to a static string constant;
    ///             the string is not copied, but merely passed over to `Profiler`!
    TimeProbe(Profiler& profiler, const char* name)
        : profiler_(profiler)
    {
#ifdef ARES_ENABLE_PROFILER
        event_.name = name;
        event_.thread = profiler_.localThreadId();
        event_.startTime = Profiler::Clock::now();
#endif
    }

    ~TimeProbe()
    {
#ifdef ARES_ENABLE_PROFILER
        event_.endTime = Profiler::Clock::now();
        profiler_.record(std::move(event_));
#endif
    }
};

}
