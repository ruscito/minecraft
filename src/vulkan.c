#include "vulkan.h"
#include "GLFW/glfw3.h"
#include "log.h"

#include <string.h>
#include <stdlib.h>

static VkInstance instance;

#ifdef RELEASE
    static const bool enable_validation_layers = false;
#else
    static const bool enable_validation_layers = true;
#endif


bool create_vulkan_instance() {
    VkApplicationInfo appInfo={};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    uint32_t count=0;
    uint32_t aux_count = 0;

    const char** glfwExtensions;
    const char** extensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&count);
    aux_count = (enable_validation_layers) ? count + 1: count;

    extensions = malloc(count * sizeof(*extensions));
    if (extensions == NULL){
        FATAL("malloc allocation faile");
        return false;
    }
    
    for(int i=0; i<count; i++) {
        extensions[i] = glfwExtensions[i];
    }

    // add only if validation callback is needed
    if (enable_validation_layers) {
        extensions[count] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        count = aux_count;
    }

#if defined(__APPLE__)
    count ++;
    const char **apple_extra_extensions = malloc(count * sizeof(*apple_extra_extensions));
    if (apple_extra_extensions==NULL) {
        FATAL("malloc allocation fail");
        return false;
    }
    for(int i=0; i<count-1; i++) {
        apple_extra_extensions[i] = extensions[i];
    }
    apple_extra_extensions[count-1] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
    INFO("Required extensions:")
    for(int i=0; i<count; i++){
        INFO("  %s", apple_extra_extensions[i]);
    }
#else
    INFO("Required extensions:")
    for(int i=0; i<count; i++){
        INFO("  %s", extensions[i]);
    }
#endif

    VkInstanceCreateInfo createInfo={};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
#if defined(__APPLE__)
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    createInfo.ppEnabledExtensionNames = apple_extra_extensions;
#else
    createInfo.ppEnabledExtensionNames = extensions;
#endif
    createInfo.enabledExtensionCount = count;
    createInfo.enabledLayerCount = 0;

    VkResult result = vkCreateInstance(&createInfo, NULL, &instance);
#if defined(__APPLE__)
    free(apple_extra_extensions);
#endif
    free(extensions);

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