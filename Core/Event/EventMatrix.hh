#pragma once

#include <unordered_map>
#include <memory>
#include <utility>
#include <typeinfo>
#include <Core/Api.h>
#include <Core/Base/TypeMap.hh>
#include <Core/Base/KeyString.hh>
#include <Core/Base/MultiDelegate.hh>

namespace Ares
{

/// A collection of event callbacks indexed by a name string.
class ARES_API EventMatrix
{
public:
    /// The type of the key identifying a particular event.
    using Key = KeyString<32>;

private:
    struct EventSlotBase
    {
        const std::type_info& delegateTypeId;

        EventSlotBase(const std::type_info& delegateTypeId)
            : delegateTypeId(delegateTypeId)
        {
        }
        virtual ~EventSlotBase() = default;
    };

    template <typename... Args>
    struct EventSlot : public EventSlotBase
    {
        using MultiDelegate = Ares::MultiDelegate<void(Args...)>;

        EventSlot()
            : EventSlotBase(typeid(MultiDelegate))
        {
        }
        ~EventSlot() override = default;

        MultiDelegate delegate;
    };

    using EventSlotHolder = std::unique_ptr<EventSlotBase>;

    std::unordered_map<Key, EventSlotHolder> matrix_;

public:
    EventMatrix() = default;
    ~EventMatrix() = default;

    template <typename... Args>
    inline MultiDelegate<void(Args...)>* get(const Key& key)
    {
        auto slotIt = matrix_.find(key);
        if(slotIt == matrix_.end())
        {
            EventSlotHolder slotHolder(new EventSlot<Args...>());
            slotIt = matrix_.insert({key, std::move(slotHolder)}).first;
        }

        EventSlotBase* slotBase = slotIt->second.get();
        if(slotBase->delegateTypeId == typeid(MultiDelegate<void(Args...)>))
        {
            auto slot = reinterpret_cast<EventSlot<Args...>*>(slotBase);
            return &slot->delegate;
        }
        else
        {
            return nullptr;
        }
    }
};

}
