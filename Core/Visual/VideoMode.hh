#pragma once

#include <Core/Api.h>
#include <Core/Visual/Resolution.hh>

namespace Ares
{

/// A video mode for rendering.
struct ARES_API VideoMode
{
    /// The fullscreen mode for a certain video mode.
    enum FullscreenMode
    {
        Windowed, ///< Windowed with client decorations.
        Fullscreen, ///< "True" fullscreen.
        WindowedFullscreen, ///< Borderless windowed "fake" fullscreen.

    } fullscreenMode = Windowed; ///< The fullscreen mode for the window.

    /// The drawable area's resolution.
    Resolution resolution = {800, 600};

    /// The refresh rate in Hertz. 0 means "don't care".
    unsigned int refreshRate = 0;
};

}
