#include "vulkan.h"
#include "GLFW/glfw3.h"
#include "log.h"

#include <string.h>
#include <stdlib.h>

static VkInstance instance;

bool create_vulkan_instance() {
    VkApplicationInfo appInfo={};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    uint32_t count=0;
    const char** glfwExtensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&count);

#if defined(__APPLE__)
    count ++;
    const char **extensions = malloc(count * sizeof(*extensions));
    if (extensions==NULL) {
        FATAL("malloc allocation fail");
        return false;
    }
    for(int i=0; i<count-1; i++) {
        extensions[i] = glfwExtensions[i];
    }
    extensions[count-1] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
    INFO("Required extensions:")
    for(int i=0; i<count; i++){
        INFO("  %s", extensions[i]);
    }
#else
    INFO("Required extensions:")
    for(int i=0; i<count; i++){
        INFO("  %s", glfwExtensions[i]);
    }
#endif

    VkInstanceCreateInfo createInfo={};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
#if defined(__APPLE__)
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    createInfo.ppEnabledExtensionNames = extensions;
#else
    createInfo.ppEnabledExtensionNames = glfwExtensions;
#endif
    createInfo.enabledExtensionCount = count;
    createInfo.enabledLayerCount = 0;

    VkResult result = vkCreateInstance(&createInfo, NULL, &instance);
#if defined(__APPLE__)
    free(extensions);
#endif
    if (result != VK_SUCCESS) {
        FATAL("Failed to create instance: %d", result);
        return false;
    }
    return true;
}

bool init_vulkan(){
    return create_vulkan_instance();
}

void destroy_vulkan() {
    vkDestroyInstance(instance, NULL);
    INFO("Vulkan destroyed")
}