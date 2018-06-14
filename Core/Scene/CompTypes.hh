#pragma once

#include "../Base/NumTypes.hh"
#include "../Base/LinTypes.hh"
#include "../Base/KeyString.hh"

namespace Ares
{

/// Alias of `bool`, used for `Comp` fields.
using Bool = bool;

/// Alias of `char`, used for `Comp` fields.
using Char = char;

/// Alias of `KeyString<4>`, used for `Comp` fields.
using Str4 = KeyString<4>;

/// Alias of `KeyString<8>`, used for `Comp` fields.
using Str8 = KeyString<8>;

/// Alias of `KeyString<16>`, used for `Comp` fields.
using Str16 = KeyString<16>;

/// Alias of `KeyString<32>`, used for `Comp` fields.
using Str32 = KeyString<32>;

}

