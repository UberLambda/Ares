#include "Vulkan.hh"

#include <assert.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include "../Base/Utils.hh"
#include "../Base/NumTypes.hh"

namespace Ares
{
namespace Vulkan
{

bool hasValidationLayers(const char** layers, unsigned int nLayers)
{
    assert(layers && "Layers not supplied");

    U32 nSupportedLayers = 0;
    vkEnumerateInstanceLayerProperties(&nSupportedLayers, nullptr);
    if(nSupportedLayers == 0)
    {
        // Failed querying supported layer count, assume none of `layers` are supported.
        return false;
    }

    std::vector<VkLayerProperties> supportedLayers(nSupportedLayers);
    if(vkEnumerateInstanceLayerProperties(&nSupportedLayers, &supportedLayers[0]) != VK_SUCCESS)
    {
        // Failed querying supported layers, assume none of `layers` are supported.
        return false;
    }

    for(const char** it = layers; it != layers + nLayers; it ++)
    {
        auto layerMatch = [it](const VkLayerProperties& supportedLayer)
        {
            return strcmp(*it, supportedLayer.layerName) == 0;
        };

        if(!any(supportedLayers.begin(), supportedLayers.end(), layerMatch))
        {
            // Layer unsupported: *it
            return false;
        }
    }

    // All layers supported
    return true;
}

VkResult createInstance(VkInstance& outInstance,
                        const char** requiredExts, unsigned int nRequiredExts,
                        const char** requiredLayers, unsigned int nRequiredLayers,
                        const VkAllocationCallbacks* allocator)
{
    VkApplicationInfo appInfo =
    {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Ares app", // FIXME Use proper application name
        .applicationVersion = VK_MAKE_VERSION(0, 0, 1), // FIXME Use appropriate app version number
        .pEngineName = "Ares",
        .engineVersion = VK_MAKE_VERSION(0, 0, 1), // FIXME Use appropriate engine version number
        .apiVersion = VK_API_VERSION_1_1,
    };

    VkInstanceCreateInfo createInfo =
    {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = nRequiredLayers,
        .ppEnabledLayerNames = requiredLayers,
        .enabledExtensionCount = nRequiredExts,
        .ppEnabledExtensionNames = requiredExts,
    };

    return vkCreateInstance(&createInfo, allocator, &outInstance);
}

VkResult createDebugCallback(VkDebugReportCallbackEXT& outCallback,
                             VkInstance instance,
                             PFN_vkDebugReportCallbackEXT callbackFunc, VkDebugReportFlagsEXT flags, void* userData,
                             const VkAllocationCallbacks* allocator)
{
    VkDebugReportCallbackCreateInfoEXT createInfo =
    {
        .sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
        .flags = flags,
        .pfnCallback = callbackFunc,
        .pUserData = userData,
    };

    auto creatorFunc = ARES_vkFunc(instance, vkCreateDebugReportCallbackEXT);
    if(creatorFunc)
    {
        return creatorFunc(instance, &createInfo, allocator, &outCallback);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

VkResult listPhysicalDevices(std::vector<VkPhysicalDevice>& devices,
                             VkInstance instance)

{
    // Query device count and devices
    U32 nDevices = 0;
    VkResult res = vkEnumeratePhysicalDevices(instance, &nDevices, nullptr);
    if (res != VK_SUCCESS)
    {
        // Error while getting physical device count
        return res;
    }

    devices.resize(nDevices);
    res = vkEnumeratePhysicalDevices(instance, &nDevices, &devices[0]);
    if (res != VK_SUCCESS)
    {
        // Error while getting physical devices
        return res;
    }

    return VK_SUCCESS;
}

}
}
