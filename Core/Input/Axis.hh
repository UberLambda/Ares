#pragma once

#include <Core/Base/KeyString.hh>

namespace Ares
{

/// A virtual axis, i.e. a floating point value that changes according to input.
using Axis = float;

/// A name to identify a particular input `Axis`.
using AxisName = KeyString<16>;

}
