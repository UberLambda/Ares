#include "Window.hh"

#include <assert.h>
#include <utility>
#include "../Base/Utils.hh"

// =============================================================================
// == GLFW 3.x Window implementation: for Windows, POSIX, Mac OS              ==
// =============================================================================

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace Ares
{

/// An helper singleton used to initialize and halt GLFW automagically.
class GLFW
{
    bool ok_;

    GLFW()
    {
        ok_ = glfwInit();
    }

    GLFW(const GLFW& toCopy) = delete;
    GLFW& operator=(const GLFW& toCopy) = delete;
    GLFW(GLFW&& toMove) = delete;
    GLFW& operator=(GLFW&& toMove) = delete;

public:
    static GLFW& instance()
    {
        static GLFW instance;
        return instance;
    }

    ~GLFW()
    {
        if(ok_)
        {
            glfwTerminate();
        }
        ok_ = false;
    }

    inline operator bool() const
    {
        return ok_;
    }
};


struct Window::Impl
{
    GLFWwindow* window;
    std::string windowTitle;
};

Window::Window()
    : impl_(nullptr)
{
}

Window::Window(VideoMode videoMode, const std::string& title)
{
    if(!GLFW::instance())
    {
        // Error initializing GLFW
        return;
    }

    impl_ = new Impl();
    impl_->window = glfwCreateWindow(800, 600, "Ares", nullptr, nullptr);
    if(!impl_->window)
    {
        // Error initializing window
        delete impl_; impl_ = nullptr;
        return;
    }

    // FIXME Implement initialize Vulkan here

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


void Window::beginFrame()
{
    assert(operator bool() && "Uninitialized Window");
    // FIXME Make context current if OpenGL
}

void Window::endFrame()
{
    assert(operator bool() && "Uninitialized Window");
    glfwSwapBuffers(impl_->window);
}

}
