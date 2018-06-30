#pragma once

#include <unordered_map>
#include <typeindex>
#include <utility>
#include <memory>

namespace Ares
{

/// A map of `typeid(T) -> T`s, i.e. mapping a type id to a value of that type.
/// NOTE: Requires RTTI.
class TypeMap
{
    struct SlotBase
    {
        virtual ~SlotBase() = default; // (will invoke `~Slot<T>()`)
    };

    template <typename T>
    struct Slot : public SlotBase
    {
        T value;


        Slot(T&& valueToMove)
            : value(std::move(valueToMove))
        {
        }

        template <typename... TArgs>
        Slot(TArgs&&... tArgs)
            : value(std::forward<TArgs>(tArgs)...)
        {
        }

        ~Slot() override = default; // (will invoke `value.~T()`)
    };

    using SlotMap = std::unordered_map<std::type_index, std::unique_ptr<SlotBase>>;
    SlotMap map_; ///< Maps typeid(T) -> smart pointer holding a `Slot<T>`


    /// See `get()`'s documentation.
    template <typename T>
    T* getValue() const
    {
        std::type_index tType = typeid(T);
        auto it = map_.find(tType);
        if(it != map_.end())
        {
            SlotBase* slotBase = it->second.get();
            auto slot = reinterpret_cast<Slot<T>*>(slotBase);
            return &slot->value; // (note: always has a fixed address since `slot`
                                 //  itself has a fixed address - that was `new`'d
                                 //  somewhere in `unique_ptr`)
        }
        else
        {
            return nullptr;
        }
    }

public:
    TypeMap() = default;
    ~TypeMap() = default;

    /// Returns a pointer for the `T` value stored in the type map, if any, or
    /// null if no `T` is currently stored in the map.
    /// The returned pointer is valid **and will not change** until the map is
    /// destroyed or `erase<T>()` is invoked. This allows you to cache it
    /// somewhere to avoid a map lookup every time.
    template <typename T>
    inline T* get()
    {
        return getValue<T>();
    }
    template <typename T>
    inline const T* get() const
    {
        return getValue<T>();
    }


    /// Attempts to construct a new `T` value in the map by constructing a new
    /// `T` in place given its constructor's arguments.
    /// Returns `true` if the `T` was actually added to the map or `false` if no
    /// action was taken because a `T` was already in the map.
    ///
    /// Note that `tArgs` may be empty (to invoke `T`'s default constructor),
    /// hold a single `T&&` (to invoke `T`'s move constructor) or hold a single
    /// `const T&` (to invoke `T`'s copy constructor).
    template <typename T, typename... TArgs>
    bool add(TArgs&&... tArgs)
    {
        if(getValue<T>() == nullptr)
        {
            std::type_index tType = typeid(T);
            auto tSlot = new Slot<T>(std::forward<TArgs>(tArgs)...);
            map_[tType].reset(static_cast<SlotBase*>(tSlot)); // (takes ownership of `tSlot`)
            return true;
        }
        else
        {
            return false;
        }
    }

    /// Erases the `T` value from the map, if any.
    template <typename T>
    void erase()
    {
        std::type_index tType = typeid(T);
        map_.erase(tType);
    }
};


}
