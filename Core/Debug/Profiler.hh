#pragma once

#include <atomic>
#include <thread>
#include <string>
#include <ostream>
#include <vector>
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
public:
    friend class TimeProbe;

    /// An unique id for a thread.
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

    /// The high precision clock used for profiling.
    struct ARES_API Clock
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

    /// A profiling event as reported by a `TimeProbe`.
    struct ARES_API TimeEvent
    {
        /// The name of the event.
        const char* name;

        /// The id of the thread from which the event originated.
        ThreadId thread;

        /// The probed start and end time, as reported by `Profiler::Clock::now()`.
        U64 startTime, endTime;
    };

private:
    moodycamel::ConcurrentQueue<TimeEvent> timeEvents_;
    moodycamel::ConsumerToken timeEventsConsumer_; ///< Used by `flush()` only

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


    /// Appends all events reported by `TimeProbe`s inbetween the lastest `flush()`
    /// call and this one to `events`. Returns the number of appended events.
    ///
    /// Always returns 0 `#ifndef ARES_ENABLE_PROFILER`.
    size_t flush(std::vector<TimeEvent>& events);
};

inline std::ostream& operator<<(std::ostream& stream, const Profiler::TimeEvent& event)
{
    stream << event.name << '@' << event.thread << ':'
           << event.startTime << ',' << event.endTime
           << '\n';
    return stream;
}

}
