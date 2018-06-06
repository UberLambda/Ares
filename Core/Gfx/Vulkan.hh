#pragma once

#include <vulkan/vulkan.h>

namespace Ares
{
namespace Vulkan
{

/// Loads a Vulkan instance's function pointer for the function `name`.
/// Returns null on failure.
#define ARES_vkFunc(instance, name) ((PFN_##name)vkGetInstanceProcAddr(instance, #name))

/// Checks that every validation layer named `*it` is supported, for `it` in
/// `layers..layers+nLayers`; returns `true` if they all are or `false` otherwise.
bool hasValidationLayers(const char** layers, unsigned int nLayers);

/// Attempts to create a Vulkan 1.1 instance for Ares given its required validation
/// layers and instance extensions. Returns a non-`VK_SUCCESS` `VkResult` on failure.
VkResult createInstance(VkInstance& outInstance,
                        const char** requiredExts, unsigned int nRequiredExts,
                        const char** requiredLayers, unsigned int nRequiredLayers,
                        const VkAllocationCallbacks* allocator=nullptr);


VkResult createDebugCallback(VkDebugReportCallbackEXT& outCallback,
                             VkInstance instance,
                             PFN_vkDebugReportCallbackEXT callbackFunc, VkDebugReportFlagsEXT flags,
                             const VkAllocationCallbacks* allocator=nullptr);

}
}
