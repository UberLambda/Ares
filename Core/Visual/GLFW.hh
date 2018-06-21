#pragma once

#include "Axis.hh"
#include <vulkan/vulkan.h>
#include <flextGL.h>
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

/// Updates core axes in `axisMap` according to the inputs read from the given
/// GLFW window as of the latest `glfwPollEvents()` call. `mouseScroll{X,Y}` have
/// to be supplied manually since there is (atleast at the moment) no way to poll
/// for their values - a callback has to be setup instead.
void updateGLFWAxisMap(GLFWwindow* window, AxisMap& axisMap, double mouseScrollX, double mouseScrollY);

}
