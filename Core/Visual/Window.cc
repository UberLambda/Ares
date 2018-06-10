#include "Window.hh"

#include <assert.h>
#include <utility>
#include "../Base/Utils.hh"
#include "../Base/NumTypes.hh"

// =============================================================================
// == GLFW 3.x Window implementation: for Windows, POSIX, Mac OS              ==
// =============================================================================
#include "GLFW.hh"

// FIXME IMPLEMENT Added joystick plug/unplug callback
//                 On joystick unplug, reset all joystick axes to `0`. This is
//                 to prevent the last value saved in "Joy.AxisN" to "stick" and
//                 change the output sum

namespace Ares
{

struct Window::Impl
{
    static void scrollCallbackGLFW(GLFWwindow* window, double dx, double dy)
    {
        auto impl = reinterpret_cast<Window::Impl*>(glfwGetWindowUserPointer(window));
        impl->mouseScrollX = dx;
        impl->mouseScrollY = dy;
    }

    GLFWwindow* window;
    std::string windowTitle;
    double mouseScrollX, mouseScrollY;
};


Window::Window()
    : impl_(nullptr)
{
}

Window::Window(Api api, VideoMode videoMode, const std::string& title)
{
    if(!GLFW::instance())
    {
        // Error initializing GLFW
        return;
    }

    impl_ = new Impl();

    switch(api)
    {
    case GL33:
        // OpenGL 3.3
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    break;

    default:
        // Vulkan
        if(!glfwVulkanSupported())
        {
            // GLFW: Vulkan not supported
            return;
        }
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // No [E]GL[ES] context required
    }


    impl_->window = glfwCreateWindow(800, 600, "Ares", nullptr, nullptr);
    if(!impl_->window)
    {
        // Error initializing window
        delete impl_; impl_ = nullptr;
        return;
    }

    // Register callbacks; window user data will be `impl_`
    glfwSetWindowUserPointer(impl_->window, impl_);
    glfwSetScrollCallback(impl_->window, Impl::scrollCallbackGLFW);

    // Enable sticky keys so that no key events are lost
    // FIXME Remove this maybe?
    glfwSetInputMode(impl_->window, GLFW_STICKY_KEYS, true);

    // FIXME IMPLEMENT initialize Vulkan here

    this->changeVideoMode(videoMode);
    this->title(title);
}

Window::~Window()
{
    delete impl_; impl_ = nullptr;
}

Window::operator bool() const
{
    return impl_ != nullptr;
}


Window::Window(Window&& toMove)
{
    (void)operator=(std::move(toMove));
}

Window& Window::operator=(Window&& toMove)
{
    // Move data over
    impl_ = toMove.impl_;

    // Invalidate moved instance
    toMove.impl_ = nullptr;

    return *this;
}


#define ARES_windowAssertOk() assert(operator bool() && "Uninitialized Window")

void Window::pollEvents()
{
    ARES_windowAssertOk();
    glfwPollEvents();

    // Update axis map
    updateGLFWAxisMap(impl_->window, axisMap_, impl_->mouseScrollX, impl_->mouseScrollY);
    // Reset mouse scroll to 0 since scrolling has already been registered by `updateGLFWAxisMap`
    impl_->mouseScrollX = impl_->mouseScrollY = 0;
}

Resolution Window::resolution() const
{
    ARES_windowAssertOk();
    int width, height;
    glfwGetWindowSize(impl_->window, &width, &height);
    return {unsigned(width), unsigned(height)};
}

bool Window::quitRequested() const
{
    ARES_windowAssertOk();
    return glfwWindowShouldClose(impl_->window);
}


const std::string& Window::title() const
{
    ARES_windowAssertOk();
    return impl_->windowTitle;
}

Window& Window::title(const std::string& newTitle)
{
    assert(operator bool() && "Uninitialized Window");
    impl_->windowTitle = newTitle;
    glfwSetWindowTitle(impl_->window, impl_->windowTitle.c_str());
    return *this;
}


Window& Window::changeVideoMode(VideoMode target)
{
    ARES_windowAssertOk();

    // Apply the closest possible video mode
    switch(target.fullscreenMode)
    {
    case VideoMode::Windowed:
        // Windowed mode: no restrictions
        glfwSetWindowMonitor(impl_->window, nullptr,
                             0, 0, target.resolution.width, target.resolution.height,
                             target.refreshRate);
        glfwSetWindowAttrib(impl_->window, GLFW_DECORATED, true);
        glfwSetWindowAttrib(impl_->window, GLFW_RESIZABLE, true);
        glfwSetWindowAttrib(impl_->window, GLFW_FLOATING, true);
    break;

    case VideoMode::Fullscreen:
        // "True" fullscreen mode: put window on primary monitor
        // TODO Would this be better if it fullscreened on the monitor where the
        //      window currently is, and not on the primary monitor by default?
        glfwSetWindowMonitor(impl_->window, glfwGetPrimaryMonitor(),
                             0, 0, target.resolution.width, target.resolution.height,
                             target.refreshRate);
        glfwSetWindowAttrib(impl_->window, GLFW_DECORATED, false);
        glfwSetWindowAttrib(impl_->window, GLFW_RESIZABLE, false);
        glfwSetWindowAttrib(impl_->window, GLFW_FLOATING, false);
    break;

    case VideoMode::WindowedFullscreen:
        // Windowed fullscreen: stretch window across all monitors
        // width = sum of all monitors' widths, height = minimum monitor height

        Resolution res = {0, unsigned(-1)}; // Set height to maximum possible now so that
                                            // it will be overwritten later by `min()` in
                                            // the monitor iteration loop
        int nMonitors = 0;
        GLFWmonitor** monitors = glfwGetMonitors(&nMonitors);
        for(GLFWmonitor** it = monitors; it != monitors + nMonitors; it ++)
        {
            const GLFWvidmode* monitorMode = glfwGetVideoMode(*it);
            res.width += monitorMode->width;
            res.height = min<size_t>(monitorMode->height, res.height);
        }

        glfwSetWindowMonitor(impl_->window, nullptr,
                             0, 0, res.width, res.height,
                             target.refreshRate);
        glfwSetWindowAttrib(impl_->window, GLFW_DECORATED, false);
        glfwSetWindowAttrib(impl_->window, GLFW_RESIZABLE, false);
        glfwSetWindowAttrib(impl_->window, GLFW_FLOATING, true);
    break;

    // default: unimplemented
    }

    return *this;
}


bool Window::queryRequiredVulkanExts(const char**& exts, unsigned int& count) const
{
    ARES_windowAssertOk();
    U32 queriedCount = 0;
    auto queriedExts = glfwGetRequiredInstanceExtensions(&queriedCount);
    exts = queriedExts;
    count = queriedCount;
    return exts != nullptr;
}

VkResult Window::createVulkanSurface(VkSurfaceKHR& surface, VkInstance instance,
                                     const VkAllocationCallbacks* allocator)
{
    ARES_windowAssertOk();
    return glfwCreateWindowSurface(instance, impl_->window, allocator, &surface);
}

void Window::beginFrame()
{
    ARES_windowAssertOk();
    // FIXME Make context current if OpenGL
}

void Window::endFrame()
{
    ARES_windowAssertOk();
    glfwSwapBuffers(impl_->window);
}

}
