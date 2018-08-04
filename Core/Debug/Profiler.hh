#pragma once

#include <atomic>
#include <thread>
#include <string>
#include <concurrentqueue.h>
#include <Ares/BuildConfig.h>
#include <Core/Api.h>
#include <Core/Base/Platform.h>
#include <Core/Base/ErrString.hh>
#include <Core/Base/NumTypes.hh>

namespace Ares
{

/// A simple intrusive profiler based on `TimeProbe`s.
class ARES_API Profiler
{
    friend class TimeProbe;

    struct Clock
    {
        /// Returns the current time in ticks since an unspecified epoch.
        inline static U64 now()
        {
            auto timepoint = std::chrono::high_resolution_clock::now();
            return timepoint.time_since_epoch().count();
        }

        /// Returns the number of nanoseconds per `Clock` tick.
        inline static constexpr U64 nsPerTick()
        {
            using Period = std::chrono::high_resolution_clock::period;
            return (U64(Period::num) * 1000000000UL) / U64(Period::den);
        }
    };

    using ThreadId = uintptr_t;

    /// Gets an unique `ThreadId` for the local thread.
    inline ThreadId localThreadId()
    {
#ifdef ARES_PLATFORM_IS_POSIX
        // PThreads: convert a `pthread_t*` to an integer id
        return uintptr_t(pthread_self());
#else
        // Win32 threads: use the Win32 `DWORD` integer id
        return uintptr_t(GetCurrentThreadId());
#endif
    }

    /// A profiling time event as reported by a `TimeProbe`.
    struct TimeEvent
    {
        const char* name;
        ThreadId thread;
        U64 startTime, endTime; // as reported by `Clock::now()`
    };

    moodycamel::ConcurrentQueue<TimeEvent> timeEvents_;

    /// Records the given time event in the events list, waiting for it to be
    /// processed by the next `flush()` call.
    /// Threadsafe and lockless.
    inline void record(TimeEvent&& event)
    {
        timeEvents_.enqueue(event);
    }

public:
    /// Initializes a new profiler ready to accept events from `TimeProbe`s.
    /// See `connect()`.
    Profiler();

    /// Halts the profiler, disconnecting it from any profiling client and making
    /// it unable to accept any more `TimeProbe` data.
    ~Profiler();

    Profiler(const Profiler& toCopy) = delete;
    Profiler& operator=(const Profiler& toCopy) = delete;

    Profiler(Profiler&& toMove) = delete;
    Profiler& operator=(Profiler&& toMove) = delete;


    /// Attempts to bind the profiler to `address` so that `flush()`ed data can
    /// be retrieved by profiling clients that connect to that address.
    ErrString connect(const std::string& address);


    /// Flushes all events reported by `TimeProbe`s inbetween the lastest `flush()`
    /// call and this one, sending them over to profiling clients for inspection.
    /// Does nothing `#ifndef ARES_ENABLE_PROFILER`.
    void flush();
};

}
