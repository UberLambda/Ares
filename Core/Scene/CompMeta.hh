#pragma once

#include "CompTypes.hh"

namespace Ares
{

/// The field of a component for reflection. See `CompMeta`.
struct CompField
{
    KeyString<16> name; ///< The name of the field.
    KeyString<16> type; ///< The datatype of the field.
    const char* descr=nullptr; ///< The description of the field. May be null.
};

/// A component's metadata for reflection.
///
/// Also: 1 medic, 1 soldier, 1 heavy, 1 demoman, 2 scouts
struct CompMeta
{
    /// The maximum number of fields allowed in a component.
    static constexpr const unsigned int MAX_FIELDS = 16;

    KeyString<16> name; ///< The name of the component class.
    const char* descr; ///< The description of the component class.
    CompField fields[MAX_FIELDS]; ///< The fields of the component class, in order of declaration.
    unsigned int nFields; ///< The number of `CompField`s in `fields`.
};

/// A struct to hold `Comp`'s component class metadata.
template <typename Comp>
struct CompMetaGetter
{
    /// Gets the component metadata associated to the component class `Comp`.
    static const CompMeta& get();
};

}
