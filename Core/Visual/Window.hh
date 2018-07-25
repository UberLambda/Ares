#pragma once

#include <string>
#include <vulkan/vulkan.h>
#include <Core/Api.h>
#include <Core/Visual/VideoMode.hh>
#include <Core/Input/AxisMap.hh>

namespace Ares
{


/// A rendering and input context.
class Window
{
public:
    /// The graphics API to use.
    enum Api
    {
        GL33, ///< OpenGL 3.3 (core)
        VK11, ///< Vulkan 1.1
    };

private:
    struct Impl;
    Impl* impl_;
    Api api_;

    AxisMap axisMap_;

    Window(const Window& toCopy) = delete;
    Window& operator=(const Window& toCopy) = delete;

public:
    /// Creates a new Window object, but does not initialize it.
    Window();

    /// Initializes a new Window using the given Api with a default video mode
    /// and title and then changes video mode and title accordingly.
    /// Check `operator bool()` to see if initialization succeeded.
    /// **WARNING** if GLFW: Needs to be called from the main thread!
    /// **NOTE** if GLFW: This will initialize flextGL.
    Window(Api api, VideoMode videoMode, const std::string& title);

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


    /// Attempts to query the Vulkan instance extensions required to make the
    /// Window work with Vulkan. Returns `true` and sets `exts` and `count` on
    /// success or `false` on error.
    /// **ASSERTS**: `operator bool()`
    /// **WARNING**: Do not `free()` the returned pointer; also, it is guaranteed
    ///              to be valid only up to the next call of this function!
    bool queryRequiredVulkanExts(const char**& exts, unsigned int& count) const;

    /// Attempts to create a Vulkan surface for the window. Returns a non-zero
    /// `VkResult` on error.
    /// **ASSERTS**: `operator bool()`
    VkResult createVulkanSurface(VkSurfaceKHR& surface, VkInstance instance,
                                 const VkAllocationCallbacks* allocator=nullptr);

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
