#include "vulkan_if.h"
#include "GLFW/glfw3.h"
#include "log.h"

#include <string.h>
#include <stdlib.h>
#include <limits.h>

static VkInstance instance;
static VkDebugUtilsMessengerEXT debug_messenger;



#ifdef NDEBUG
    static const bool enable_validation_layers = false;
#else
    static const bool enable_validation_layers = true;
#endif


typedef struct  queue_family_indices_t {
    uint32_t graphics_family;
}queue_family_indices_t;

// function declaration
static void setup_messanger_create_info();
static void setup_debug_messenger();
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback();
static bool is_validation_layer_available(const char *validation_layer);
static void setup_debug_messenger(VkDebugUtilsMessengerCreateInfoEXT *messanger_info);
static void destroy_debug_messanger();
static void setup_messanger_create_info(VkDebugUtilsMessengerCreateInfoEXT *messanger_info) ;
static bool create_vulkan_instance();
static bool pick_physical_device(VkPhysicalDevice *device);
static queue_family_indices_t find_queue_families(VkPhysicalDevice device);





bool init_vulkan(){
    if (create_vulkan_instance()) {
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
    default:
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

static void setup_debug_messenger(VkDebugUtilsMessengerCreateInfoEXT *messanger_info) {
    if (!enable_validation_layers) {
        return;
    }

    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
     if (func != NULL) {
        func(instance, messanger_info, NULL, &debug_messenger);
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

static void setup_messanger_create_info(VkDebugUtilsMessengerCreateInfoEXT *messanger_info) {
    //VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    messanger_info->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    //messanger_info->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | 
    messanger_info->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    messanger_info->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    messanger_info->pfnUserCallback = debug_callback;
    messanger_info->pUserData = NULL; // Optional
}

static bool create_vulkan_instance() {
    VkDebugUtilsMessengerCreateInfoEXT messanger_create_info = {};
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Minecraft";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;
    
    uint32_t count=0;
    uint32_t aux_count = 0;

    const char** glfwExtensions;
    //const char** extensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&count);
    aux_count = (enable_validation_layers) ? count + 1: count;

    const char* extensions[count]; 
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
    const char *apple_extra_extensions[count];
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
    create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    create_info.ppEnabledExtensionNames = apple_extra_extensions;
#else
    create_info.ppEnabledExtensionNames = extensions;
#endif
    
    // create the validation layer 
    const char *layer[] = {"VK_LAYER_KHRONOS_validation"};
    
    if (enable_validation_layers && is_validation_layer_available("VK_LAYER_KHRONOS_validation")){
        INFO("Validation layer [%s] available", "VK_LAYER_KHRONOS_validation");
        setup_messanger_create_info(&messanger_create_info);
        create_info.enabledLayerCount = (uint32_t) 1;
        create_info.ppEnabledLayerNames = layer;
        create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)  &messanger_create_info;
    } else {
        create_info.enabledLayerCount = 0;
        create_info.ppEnabledLayerNames = NULL;
    }

    create_info.enabledExtensionCount = count;
    
    VkResult result = vkCreateInstance(&create_info, NULL, &instance);

    if (result != VK_SUCCESS) {
        FATAL("Failed to create instance: %d", result);
        return false;
    }
    
    // now setup the debug messenger
    setup_debug_messenger( &messanger_create_info);

    // now pick a GPU. This object will be implicitly destroyed whith VkInstance
    static VkPhysicalDevice physical_device = VK_NULL_HANDLE; 
    if (!pick_physical_device(&physical_device)) {
        FATAL("Failed to pick a GPU");
        return false;
    }

    INFO("Vulkan instance created");
    return true;
}

static bool pick_physical_device(VkPhysicalDevice *device){
    // pick a graphic card
    uint32_t device_count;
    
    vkEnumeratePhysicalDevices(instance, &device_count, NULL);
    if (device_count == 0){
        FATAL("No graphic card available");
        return false;
    }

    // create an array to contain all graphic cards available
    VkPhysicalDevice device_list[device_count];
    vkEnumeratePhysicalDevices(instance, &device_count, device_list);

    // https://gamedev.stackexchange.com/questions/124738/how-to-select-the-most-powerful-vkdevice
    bool found_discrete_GPU = false;

    // check for discrete GPU
    for (int i=0; i<device_count; i++){
        VkPhysicalDeviceProperties properties ={};
        vkGetPhysicalDeviceProperties(device_list[0], &properties);
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            found_discrete_GPU = true;
            INFO("Found and discrete GPU");
            INFO(" Driver name: %s", properties.deviceName);
            device = &device_list[i];
            break;
        } 
    }

    // if not dicrete then look for an integrate GPU
    if(!found_discrete_GPU){
        for (int i=0; i<device_count; i++){
            VkPhysicalDeviceProperties properties ={};
            vkGetPhysicalDeviceProperties(device_list[0], &properties);
            if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
                INFO("Found integrate GPU");
                INFO(" Driver name: %s", properties.deviceName);
                device = &device_list[i];
                break;
            }
        } 
    }       

    // fail if not dicrete or integrated GPU is found
    if (device == VK_NULL_HANDLE) {
        FATAL("Failed to find a suitable GPU");
        return false;
    }  

    queue_family_indices_t indices =find_queue_families(*device);

    if (indices.graphics_family == UINT32_MAX) {
        FATAL("Phisical device do not support graphichs queue");
        return false;
    }
    return true;
}

static queue_family_indices_t find_queue_families(VkPhysicalDevice device){
    // find grapichs queue family
    queue_family_indices_t indices = {};
    indices.graphics_family = UINT32_MAX;

    uint32_t queue_family_count;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);
    VkQueueFamilyProperties queue_family[queue_family_count];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_family);
    
    for (int i=0; i<queue_family_count; i++){
        if(queue_family[i].queueFlags & VK_QUEUE_GRAPHICS_BIT){
            indices.graphics_family = i;
        }
    }

    return indices;
}