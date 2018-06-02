#pragma once

#include <string>
#include "VideoMode.hh"
#include "Axis.hh"

namespace Ares
{

/// A rendering and input context.
class Window
{
    struct Impl;
    Impl* impl_;

    AxisMap axisMap_;

    Window(const Window& toCopy) = delete;
    Window& operator=(const Window& toCopy) = delete;

public:
    /// Creates a new Window object, but does not initialize it.
    Window();

    /// Initializes a new Window with a default video mode and title and then changes
    /// video mode and title accordingly.
    /// Check `operator bool()` to see if initialization succeeded.
    /// **WARNING** if GLFW: Needs to be called from the main thread!
    Window(VideoMode videoMode, const std::string& title);

    Window(Window&& toMove);
    Window& operator=(Window&& toMove);

    /// Destroys the Window (if initialized)
    /// **WARNING** if GLFW: Needs to be called from the main thread!
    ~Window();

    /// Returns `true` if the Window is currently initialized (and not
    /// destroyed/not moved) or `false `otherwise.
    operator bool() const;


    /// Polls for any new user/OS events concerning the Window.
    /// **ASSERTS**: `operator bool()`
    /// **WARNING** if GLFW: Needs to be called from the main thread!
    void pollEvents();

    /// Returns `true` if something has asked the Window to quit sometime before
    /// the latest `pollEvents()` call.
    /// **ASSERTS**: `operator bool()`
    bool quitRequested() const;

    /// Returns the resolution of the Window's drawable area as of the latest
    /// `pollEvents()` call.
    /// **ASSERTS**: `operator bool()`
    Resolution resolution() const;

    /// Returns a reference to the window's axis map, which is updated as of the
    /// latest `pollEvents()` call.
    /// **ASSERTS**: `operator bool()`
    inline const AxisMap& axisMap() const
    {
        return axisMap_;
    }


    /// Tries to change the Window's video mode to `target`. This may or may not
    /// succeed fully; check resolution, refresh rate, etc. afterwards to see if
    /// it actually succeeded.
    /// **ASSERTS**: `operator bool()`
    /// **WARNING** if GLFW: Needs to be called from the main thread!
    Window& changeVideoMode(VideoMode target);
    // FIXME IMPLEMENT A way to check `target.refreshRate`
    // FIXME IMPLEMENT An interface to list native video modes supported by the system

    /// Gets or sets the Window's title.
    /// **ASSERTS**: `operator bool()`
    /// **WARNING** if GLFW: Setter needs to be called from the main thread!
    const std::string& title() const;
    Window& title(const std::string& newTitle);


    /// Makes the Window prepare for the rendering of a new frame.
    /// **ASSERTS**: `operator bool()`
    /// **WARNING** if GLFW: Needs to be called from the main thread!
    void beginFrame();

    /// Makes the Window mark the rendering of the current frame as complete,
    /// swapping buffers.
    /// **ASSERTS**: `operator bool()`
    /// **WARNING** if GLFW: Needs to be called from the main thread!
    void endFrame();
};

}