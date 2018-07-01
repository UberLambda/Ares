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

        /// Returns a typeless pointer to the slotted value
        virtual void* get() = 0;
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


        void* get() override
        {
            return reinterpret_cast<void*>(&value);
        }
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
    friend class const_iterator;
    class const_iterator;

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


    /// A readonly iterator over pairs in the `EventQueue<T>`.
    /// Pointers to `T`s are retrieved as `void*`.
    class const_iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = std::pair<std::type_index, void*>;
        using reference = const value_type&;
        using pointer = const value_type*;

    private:
        friend class TypeMap;

        using InnerIterator = TypeMap::SlotMap::const_iterator;
        InnerIterator it_;
        value_type itPair_;

        const_iterator(const InnerIterator it)
            : it_(it), itPair_(typeid(int), nullptr)
        {
            // (`itPair_` undefined here)
        }

    public:
        const_iterator(const const_iterator& toCopy)
            : const_iterator(toCopy.it_)
        {
            // (`itPair_` does not need to be copied)
        }

        inline const_iterator& operator=(const const_iterator& toCopy)
        {
            it_ = toCopy.it_;
            // (`itPair_` does not need to be copied)

            return *this;
        }


        inline pointer operator->()
        {
            // Update `itPair_` and return a pointer to it
            itPair_.first = it_->first;
            auto slotPtr = reinterpret_cast<SlotBase*>(it_->second.get());
            itPair_.second = slotPtr->get();

            return &itPair_;
        }

        inline reference operator*()
        {
            return *operator->();
        }


        inline bool operator==(const const_iterator& other) const
        {
            return it_ == other.it_;
        }

        inline bool operator!=(const const_iterator& other) const
        {
            return !operator==(other);
        }


        const_iterator& operator++() // preincrement
        {
            it_ ++;
            return *this;
        }

        inline const_iterator operator++(int) // postincrement
        {
            const_iterator old = *this;
            (void)operator++();
            return old;
        }
    };

    inline const_iterator begin() const
    {
        return const_iterator(map_.cbegin());
    }

    inline const_iterator cbegin() const
    {
        return begin();
    }

    inline const_iterator end() const
    {
        return const_iterator(map_.cend());
    }

    inline const_iterator cend() const
    {
        return end();
    }
};


}
