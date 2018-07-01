#pragma once

#include "../Base/TypeMap.hh"
#include "EventQueue.hh"

namespace Ares
{

/// A collection of `EventQueue`s of types.
class EventMatrix
{
    TypeMap matrix_;

public:
    EventMatrix() = default;
    ~EventMatrix() = default;


    /// Gets a pointer to the event queue for `T` events in the matrix, or null
    /// if no such queue is present.
    template <typename T>
    EventQueue<T>* getQueue() const
    {
        return matrix_.get<EventQueue<T>>();
    }

    /// Attempts to add a new queue for events of type `T` to the matrix.
    /// Returns `false` and does nothing if such a queue already existed.
    template <typename T>
    bool addQueue()
    {
        return matrix_.add<EventQueue<T>>();
    }


    /// Clears every event queue in the event matrix.
    void clearAllQueues()
    {
        for(std::pair<typeid(T), void*>& queuePair : matrix_)
        {
            auto queue = reinterpret_cast<EventQueueBase*>(queuePair.second);
            queue->clear();
        }
    }
};

}
