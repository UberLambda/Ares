#pragma once

#include <assert.h>
#include <utility>
#include <atomic>
#include <mutex>
#include <type_traits>

namespace Ares
{

/// A reference-counted container for a `T`.
/// Similiar to a thread-safe + lockless `std::shared_ptr`.
template <typename T>
class Ref
{
    template <typename U, typename... TArgs>
    friend Ref<U> makeRef(TArgs&&... tArgs);

    template <typename U>
    friend Ref<U> intoRef(U* t);


    /// A `T` allocated on the heap.
    struct THeapBox
    {
        T* t;

        /// Takes ownership of `t`.
        THeapBox(T* t)
            : t(t)
        {
        }

        /// Constructs a new `t` in-place.
        template <typename... TArgs>
        THeapBox(TArgs&&... args)
            : t(new T(std::forward<TArgs>(args)...))
        {
        }

        ~THeapBox()
        {
            delete t; // t.~T() will be run here
        }

        inline T* get()
        {
            return t;
        }

        inline const T* get() const
        {
            return t;
        }
    };

    /// A `T` allocated on the stack (or, actually, on the heap as part of `Data`
    /// if inside of `data_`)
    struct TStackBox
    {
        T t;

        /// Takes ownership of `t`.
        TStackBox(T* t)
            : t(std::move(t))
        {
            delete t;
        }

        /// Constructs a new `t` in-place.
        template <typename... TArgs>
        TStackBox(TArgs&&... args)
            : t(std::forward<TArgs>(args)...)
        {
        }

        ~TStackBox()
        {
            // t.~T() will be run here
        }

        inline T* get()
        {
            return &t;
        }

        inline const T* get() const
        {
            return &t;
        }
    };


    /// A box acting as a `T`; allocated on the stack if possible (i.e. if
    /// a `T` can be allocated because it is not abstract), otherwise allocated
    /// on the heap.
    using TBox = typename std::conditional<std::is_abstract<T>::value, THeapBox, TStackBox>::type;

    // (see `~Ref()`)
    struct Data
    {
        TBox t;
        std::atomic<unsigned int> refCount;

        ~Data() = default; // (will invoke `t.~T()`)
    };
    std::atomic<Data*> data_;


    /// See `get()`
    inline T* get_() const
    {
        Data* data = data_.load();
        if(data)
        {
            return data->t.get();
        }
        else
        {
            return nullptr;
        }
    }

public:
    /// Constructs a new null `Ref`.
    /// Threadsafe and lockless.
    Ref()
        : data_(nullptr)
    {
    }

    /// Constructs a new `Ref` by copying the given one.
    /// If the given ref is not null, the internal reference count to the resource
    /// is incremented by one and this ref will point to that shared resource.
    /// However, in some cases (a ref being copied while its `~Ref()` destructor
    /// is running) this operation may fail and leave this ref null.
    Ref(const Ref& toCopy)
        : data_(nullptr)
    {
        (void)operator=(toCopy);
    }

    /// See `Ref(const Ref&)`.
    Ref& operator=(const Ref& toCopy)
    {
        this->~Ref();

        // Copy over toCopy's data, if any; if data is present, increment its
        // reference count.
        data_ = toCopy.data_.load(); // (atomic load `toCopy.data_`, followed by atomic store `data_`)

        if(data_) // (atomic load `data_`)
        {
            // FIXME `data_` could potentially have been set to `null` by another thread here!
            data_.load()->refCount ++; // (atomic load `data_` + atomic fetch/add `data_->refCount`)
        }

        return *this;
    }

    /// Constructs a new `Ref` by moving the given one (which is set to null).
    /// If the given ref is not null the internal reference count to the resource
    /// is kept unchanged but this `Ref` will point to the given ref's shared resource
    /// in its place.
    /// However, in some cases (a ref being moved while its `~Ref()` destructor
    /// is running) this operation may fail and leave this ref null.
    /// Threadsafe and lockless.
    Ref(Ref&& toMove)
        : data_(nullptr)
    {
        (void)operator=(std::move(toMove));
    }

    /// See `Ref(Ref&&)`
    Ref& operator=(Ref&& toMove)
    {
        this->~Ref();

        data_ = toMove.data_.exchange(nullptr); // (atomic exchange `toMove.data_` + atomic store `data_`)

        return *this;
    }

    /// For non-null refs, decreases the `T`'s reference count by 1 and, if the
    /// reference count dropped to 0, frees the inner `T`.
    /// Threadsafe and lockless.
    /// **WARNING**: Assumes that `~Ref` is only run once per ref (as should alrady
    /// be the case), and expecially that multiple `~Ref()` calls for the same
    /// object are not run in parallel!
    ~Ref()
    {
        if(!data_) // (atomic load `data_`)
        {
            // Nothing to do, `data_` is null
            return;
        }

        unsigned int refCount = (-- data_.load()->refCount); // (atomic load `data_` + atomic fetch/sub `data_->refCount`)
        if(refCount == 0)
        {
            // Reference count dropped to 0. Since `refCount` is atomic, this if
            // branch will only run in a single `~Ref()` invocation per object;
            // that invocation will be responsible for deleting `data_` (hence also its
            // `data_->t`).
            Data* data = data_.exchange(nullptr);
            if(data && data->refCount.load() == 0)
            {
                delete data;
            }
            // Else `data` is null because `Ref(Ref&&)` replaced `data_` or
            // `data->refCount` is not zero because `Ref(const Ref&)` incremented
            // it; in both cases we need to abort the deletion of `data_` (hence also `data_->t`)
            // since they are still required by the newly-instantiated ref!
        }
    }


    /// Returns `true` if the `Ref` is currently pointing to a `T`, or `false`
    /// if it is a null `Ref`.
    /// Threadsafe and lockless.
    inline operator bool() const
    {
        return data_.load() != nullptr;
    }

    /// Returns the current reference count of the internal `T`, or 0 if this is
    /// a null ref.
    /// Threadsafe and lockless.
    inline unsigned int refCount() const
    {
        if(data_.load() != nullptr)
        {
            // FIXME `data_` could potentially have been set to `null` by another thread here!
            return data_.load()->refCount.load();
        }
        else
        {
            return 0;
        }
    }


    /// Obtains a pointer to the inner `T` value, or null if this ref is null.
    /// Threadsafe and lockless.
    inline T* get()
    {
        return get_();
    }
    inline const T* get() const
    {
        return get_();
    }

    /// See `get()`.
    inline T* operator->()
    {
        return get_();
    }
    inline const T* operator->() const
    {
        return get_();
    }

    /// Obtains a reference to the inner `T` value.
    /// Threadsafe and lockless.
    /// **WARNING**: Undefined behaviour (possibly segmentation fault) if the ref is null!
    inline T& operator*()
    {
        return *get_();
    }
    inline const T& operator*() const
    {
        return *get_();
    }
};


/// Constructs a new `Ref` that will own a `T` constructed by passing `tArgs`
/// to `T`'s constructor.
/// Threadsafe and lockless.
template <typename T, typename... TArgs>
inline Ref<T> makeRef(TArgs&&... tArgs)
{
    using Ref = Ref<T>;
    using RefData = typename Ref::Data;

    Ref ref;
    ref.data_ = new RefData{{std::forward<TArgs>(tArgs)...}, {1}};
    return std::move(ref);
}

/// Transfers ownership of `t` to a new `Ref` with reference count 1.
/// `t` must have been allocated with `operator new`.
/// **WARNING**: This could potentially move the `T` at `t` to a new memory location,
///              making the pointer invalid! Do not reuse the supplied `t` pointer;
///              use `get()` on the returned ref instead.
/// Threadsafe and lockless.
template <typename T>
inline Ref<T> intoRef(T* t)
{
    // Abstract type: replace the `T*` in the `ref.data_: THeapBox`
    using Ref = Ref<T>;
    using RefData = typename Ref::Data;

    Ref ref;
    ref.data_ = new RefData{{t}, {1}};
    return std::move(ref);
}

}
