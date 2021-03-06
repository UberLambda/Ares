#pragma once

#include <stddef.h>
#include <ostream>
#include <vector>
#include <concurrentqueue.h>
#include <tinyformat.h>
#include <Core/Api.h>
#include <Core/Base/AtomicPool.hh>
#include <Core/Base/MemStreambuf.hh>

namespace Ares
{

/// The level of a `LogMessage`.
enum LogLevel
{
    /// Events that are only marginally (or sometimes not at all) useful for
    /// debugging purposes.
    Trace,

    /// Events that are generally only useful for debugging purposes.
    Debug,

    /// Events that normally occur during the flow of the program but that are
    /// useful to be logged as reference points in time.
    Info,

    /// Events that do not impact the user but that can have hidden
    /// repercussions (ex. performance problems).
    Warning,

    /// Events that visibly impact the user but that are usually recoverable
    /// from.
    Error,

    /// Events that are usually unrecoverable from and could possibly even
    /// crash the application.
    Fatal,
};

/// A message to be logged in a `Log`.
struct ARES_API LogMessage
{
    /// The maximum size in bytes of the contents of the message.
    /// This includes the null terminator.
    static constexpr const size_t MAX_CONTENT_SIZE = 256;


    /// The level of the message to be logged.
    LogLevel level;

    /// The source file the message originated from.
    const char* sourceFile;

    /// The line of the source file the message originated from.
    unsigned int sourceLine;

    /// The content of the message iself.
    char content[MAX_CONTENT_SIZE];
};

/// A sink to which `LogMessage`s will go to when a `Log` is `flush()`ed.
/// The sink will be passed the message that is to be output (but it may as well
/// ignore it if wishes to do so) and some data that the user passed when
/// `Log::addSink()` was called.
using LogSink = void(*)(const LogMessage* message, void* data);

/// A stream of `LogMessage`s.
class ARES_API Log
{
    AtomicPool<LogMessage> messagePool_;
    moodycamel::ConcurrentQueue<LogMessage*> messagesToFlush_;

    struct SinkSlot
    {
        LogSink sink;
        void* data;
    };
    std::vector<SinkSlot> sinks_;

public:
    /// Intializes a new log given the number of messages in its message pool;
    /// only `messagePoolSize` messages can be logged inbetween `flush()` calls.
    Log(size_t messagePoolSize=1024);
    ~Log();

    /// Returns the size of the log's internal message pool size.
    inline size_t messagePoolSize() const
    {
        return messagePool_.n();
    }

    /// Commits an unformatted message to the log. See the documentation of
    /// `LogMessage` for a description of the parameters. Note that message contents
    /// are copied over, but truncated to `LogMessage::MAX_CONTENT_SIZE` bytes in size!
    /// The message will not be logged immediately; it will be enqueued internally
    /// on the next `flush()` call.
    /// This operation is threadsafe and atomic.
    /// **ASSERTS**: That a message can be logged (number of messages available
    /// in pool must be atleast one)
    inline void log(LogLevel level, const char* sourceFile, unsigned int sourceLine,
                    const char* content)
    {
        // Grab message from pool [atomic operation]
        LogMessage* message = messagePool_.grab();
        assert(message && "No more available messages!");
        // FIXME UNDECIDED Maybe just drop some messages instead of crashing the
        //                 program if the pool is full?

        // Fill message
        message->level = level;
        message->sourceFile = sourceFile;
        message->sourceLine = sourceLine;

        strncpy(message->content, content, LogMessage::MAX_CONTENT_SIZE);
        message->content[LogMessage::MAX_CONTENT_SIZE - 1] = '\0'; // Make sure content is null-terminated

        // Put message into flushing queue [atomic operation]
        messagesToFlush_.enqueue(message);
    }

    /// Commits a printf-style formatted message to the log. See the documentation of
    /// `LogMessage` for a description of the parameters. Note that formatted
    /// message contents are truncated to `LogMessage::MAX_CONTENT_SIZE` bytes
    /// in size!
    /// The message will not be logged immediately; it will be enqueued internally
    /// on the next `flush()` call.
    /// This operation is threadsafe and atomic.
    /// **ASSERTS**: That a message can be logged (number of messages available
    /// in pool must be atleast one)
    template <typename... FmtArgs>
    inline void log(LogLevel level, const char* sourceFile, unsigned int sourceLine,
             const char* contentFmt, FmtArgs... contentFmtArgs)
    {
        // Grab message from pool [atomic operation]
        LogMessage* message = messagePool_.grab();
        assert(message && "No more available messages!");
        // FIXME UNDECIDED Maybe just drop some messages instead of crashing the
        //                 program if the pool is full?

        // Reset message contents and fill message, formatting `contentFmt` to
        // `message->content`
        message->level = level;
        message->sourceFile = sourceFile;
        message->sourceLine = sourceLine;
        memset(message->content, '\0', LogMessage::MAX_CONTENT_SIZE);

        MemStreambuf messageBuf(message->content, LogMessage::MAX_CONTENT_SIZE);
        std::ostream messageStream(&messageBuf);
        tinyformat::format(messageStream,
                           contentFmt, std::forward<FmtArgs>(contentFmtArgs)...);

        // Put message into flushing queue [atomic operation]
        messagesToFlush_.enqueue(message);
    }


    /// Adds a sink to which messages will be output on `flush()`. The optional
    /// given `data` will be passed to the sink each time that it is invoked.
    /// **WARNING**: The same sink can be added multiple times to the same `Log`
    ///              and this will result in it being called multiple times!
    void addSink(LogSink sink, void* data=nullptr);

    /// Removes a sink that was `addSink()`ed into the log previously. Does nothing
    /// if the sink was in fact never added.
    /// **WARNING**: If the same sink is `addSink()`ed multiple times into the
    ///              Log all copies will be removed by a single `removeSink()` call!
    void removeSink(LogSink sink);


    /// Flushes messages enqueued before this `flush()` call, actually sending
    /// them to the log streams. If count is `size_t(-1)`, messages are flushed
    /// until there are no more in the queue; **in some situations, this goes on
    /// forever** as messages are enqueued by other thread as this thread is
    /// flushing them! To prevent an infinite loop, if `count` is any other number
    /// at most `count` messages are flushed, leaving the rest in the queue to be
    /// flushed later.
    ///
    /// This is *NOT* guaranteed to be threadsafe, but in any case while messages
    /// can be `log()`ged by any thread concurrently, they should be periodically
    /// `flush()`ed by a single "logger I/O thread".
    void flush(size_t count=-1);
};


// Default value, to be replaced via CMake for each project using Log.hh; See `ARES_FILE_`
#ifndef ARES_SRCDIR_PATH_LENGTH
#   define ARES_SRCDIR_PATH_LENGTH 0
#endif


/// A shortened __FILE__, obtained by skipping `ARES_SRCDIR_PATH_LENGTH` characters from it
/// See: https://stackoverflow.com/a/40947954 and Debug/Log/Log.hh and Core/CMakeLists.txt
#define ARES_FILE_ (&__FILE__[ARES_SRCDIR_PATH_LENGTH])

/// A convenience macro that `log()`s a [un]formatted message coming from the
/// current file and line to a `Log`.
/// Example: `ARES_log(myLog, Info, "Could not find %s", "shrubberies")` in
///           Main.cc at line 9 would expand to `myLog.log(Info, "Main.cc", 9,
///           "Could not find %s", "shrubberies")`.
#define ARES_log(log_, level, ...) (log_).log((level), ARES_FILE_, __LINE__, __VA_ARGS__)

}
