#include "vulkan.h"
#include "GLFW/glfw3.h"
#include "log.h"

#include <string.h>
#include <stdlib.h>

static VkInstance instance;
static VkDebugUtilsMessengerEXT debug_messenger;
static VkDebugUtilsMessengerCreateInfoEXT messanger_create_info = {};

#ifdef NDEBUG
    static const bool enable_validation_layers = false;
#else
    static const bool enable_validation_layers = true;
#endif

// function declaration
static void setup_messanger_create_info();
static void setup_debug_messenger();
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback();
static bool is_validation_layer_available(const char *validation_layer);
static void setup_debug_messenger();
static void destroy_debug_messanger();
static void setup_messanger_create_info() ;
static bool create_vulkan_instance();


bool init_vulkan(){
    if (create_vulkan_instance()) {
        setup_debug_messenger();
        return true;
    }
    return false;
}

void destroy_vulkan() {
    destroy_debug_messanger();
    vkDestroyInstance(instance, NULL);
    INFO("Vulkan destroyed")
}


static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data) {
    
    char *type_info = "[UNKNOWN]";
    switch (message_type)
    {
    case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
        type_info = "[GENERAL]";
        break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
        type_info = "[SPECIFICATION]";
        break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
        type_info = "[PERFORAMNCE]";
        break;
    }

    switch (message_severity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        TRACE("Vulkan %s %s", type_info, callback_data->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        INFO("Vulkan %s %s", type_info, callback_data->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        WARNING("Vulkan %s %s", type_info, callback_data->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        FATAL("Vulkan %s %s", type_info, callback_data->pMessage);
        break;
    }


    // according to the totorial  (message callback section) always return false
    // https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Validation_layers   
    return VK_FALSE;
}

static bool is_validation_layer_available(const char *validation_layer) {
    uint32_t count = 0;

    vkEnumerateInstanceLayerProperties(&count, NULL);
    VkLayerProperties *available_layers = ( VkLayerProperties *) malloc(count * sizeof(*available_layers));
    VkResult result = vkEnumerateInstanceLayerProperties(&count, available_layers);

    for (int i=0; i<count; i++) {
        if (strcmp(validation_layer, available_layers[i].layerName)==0) {
            free(available_layers);
            return true;
        }
    }

    free(available_layers);
    return false;
}

static void setup_debug_messenger() {
    if (!enable_validation_layers) {
        return;
    }

    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
     if (func != NULL) {
        func(instance, &messanger_create_info, NULL, &debug_messenger);
    } else {
        ERROR("[setup_debug_manager] Extension %s not present");
        return;
    }   
    INFO("Debug messanger created")
}

static void destroy_debug_messanger() {
    if (!enable_validation_layers) {
        return;
    }
    PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
     if (func != NULL) {
        func(instance, debug_messenger, NULL);
    } 
    INFO("Debug messanger destroyed")
}

static void setup_messanger_create_info() {
    //VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    messanger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    //messanger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | 
    messanger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    messanger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    messanger_create_info.pfnUserCallback = debug_callback;
    messanger_create_info.pUserData = NULL; // Optional
}

static bool create_vulkan_instance() {
    VkApplicationInfo app_info={};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Minecraft";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;
    
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

    VkInstanceCreateInfo create_info={};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
#if defined(__APPLE__)
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    createInfo.ppEnabledExtensionNames = apple_extra_extensions;
#else
    create_info.ppEnabledExtensionNames = extensions;
#endif
    
    // create the validation layer 
    const char **layer = malloc(sizeof(*layer));
    layer[0] ="VK_LAYER_KHRONOS_validation";
    
    if (enable_validation_layers && is_validation_layer_available("VK_LAYER_KHRONOS_validation")){
        INFO("Validation layer [%s] available", "VK_LAYER_KHRONOS_validation");
        setup_messanger_create_info();
        create_info.enabledLayerCount = (uint32_t) 1;
        create_info.ppEnabledLayerNames = layer;
        create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)  &messanger_create_info;
    } else {
        create_info.enabledLayerCount = 0;
        create_info.ppEnabledLayerNames = NULL;
    }

    create_info.enabledExtensionCount = count;
    
    VkResult result = vkCreateInstance(&create_info, NULL, &instance);
#if defined(__APPLE__)
    free(apple_extra_extensions);
#endif
    free(extensions);
    free(layer);
    if (result != VK_SUCCESS) {
        FATAL("Failed to create instance: %d", result);
        return false;
    }
    INFO("Vulkan instance created");
    return true;
}