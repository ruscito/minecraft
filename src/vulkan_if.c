#include "vulkan_if.h"
#include "log.h"

#include <string.h>
#include <stdlib.h>
#include <limits.h>

struct  queue_family_indices {
    uint32_t graphics_family;
    uint32_t present_family;
};

static VkInstance instance;
static VkDevice logical_device = VK_NULL_HANDLE; // the logical device
static VkPhysicalDevice physical_device = VK_NULL_HANDLE; 
static VkDebugUtilsMessengerEXT debug_messenger;
static VkDebugUtilsMessengerCreateInfoEXT messanger_create_info = {};
static VkQueue graphics_queue; // handler to the graphics queue
static VkQueue present_queue; // handler to the graphics queue
static GLFWwindow *wnd;
static VkSurfaceKHR surface;
static struct queue_family_indices queue_indices; 

#if defined(__APPLE__)
// https://stackoverflow.com/questions/68127785/how-to-fix-vk-khr-portability-subset-error-on-mac-m1-while-following-vulkan-tuto
    static const char* device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, "VK_KHR_portability_subset"};
    static uint32_t devie_extensions_count = 2;
#else
    static const char* device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    static uint32_t devie_extensions_count = 1;
#endif


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
static void destroy_debug_messanger();
static void setup_messanger_create_info() ;
static bool create_vulkan_instance();
static bool pick_physical_device();
static void find_queue_families();
static bool create_logical_device();
static bool create_surface();
static bool check_device_extension_support();


bool init_vulkan(GLFWwindow *window){
    wnd = window;

    if (!create_vulkan_instance()) return false;
    setup_debug_messenger( &messanger_create_info);
    if (!create_surface()) return false;
    if (!pick_physical_device())  return false; // pick a GPU. This object will be implicitly destroyed whith VkInstance
    if (!create_logical_device()) return false;

    return true;
}

void destroy_vulkan() {
    vkDestroyDevice(logical_device, NULL);
    destroy_debug_messanger();
    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyInstance(instance, NULL);
    INFO("Vulkan destroyed")
}

static bool create_surface() {
    if (glfwCreateWindowSurface(instance, wnd, NULL, &surface) != VK_SUCCESS) {
        FATAL("Failed to pick a GPU");
        return false;
    }

    return true;
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

static void setup_messanger_create_info() {
    messanger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    //messanger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | 
    //messanger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
    messanger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    messanger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    messanger_create_info.pfnUserCallback = debug_callback;
    messanger_create_info.pUserData = NULL; // Optional
}

static bool pick_physical_device(){
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
            INFO("Found discrete GPU");
            INFO("  Driver name: %s", properties.deviceName);
            physical_device = device_list[i];
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
                INFO("  Driver name: %s", properties.deviceName);
                physical_device = device_list[i];
                break;
            }
        } 
    }       

    // fail if not discrete or integrated GPU is found
    if (physical_device == VK_NULL_HANDLE) {
        FATAL("Failed to find a suitable GPU");
        return false;
    }  

    // find queue families
    find_queue_families();

    if (queue_indices.graphics_family == UINT32_MAX ) {
        FATAL("Phisical device selected do not support graphichs queue");
        return false;
    }

    if (queue_indices.present_family == UINT32_MAX ) {
        FATAL("Phisical device selected do not support presentation");
        return false;
    }

    if(!check_device_extension_support()) {
        FATAL("Phisical device do not support required extensions");
        return false;
    }

    return true;
}

static void find_queue_families(){
    // find grapichs queue family
    queue_indices.graphics_family = UINT32_MAX;
    queue_indices.present_family = UINT32_MAX;

    uint32_t queue_family_count;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, NULL);
    VkQueueFamilyProperties queue_family[queue_family_count];
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_family);

    INFO("Queue Families Supported")
    for (int i=0; i<queue_family_count; i++){
        INFO("  Queue Family Property:%d, can create up to :%d queue and supports:",i,  queue_family[i].queueCount);
        if (queue_family[i].queueFlags & VK_QUEUE_GRAPHICS_BIT ) INFO("  ->%s", "Graphics operations");
        if (queue_family[i].queueFlags & VK_QUEUE_COMPUTE_BIT  ) INFO("  ->%s", "Compute operations");
        if (queue_family[i].queueFlags & VK_QUEUE_TRANSFER_BIT  ) INFO("  ->%s", "Transfer operations");
        if (queue_family[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT  ) INFO("  ->%s", "Sparse memory mangment operations");
        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support);
        if (present_support) INFO("  ->%s", "Presentation");
    }
    
    for (int i=0; i<queue_family_count; i++){
        if(queue_family[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && queue_indices.graphics_family == UINT32_MAX){
            queue_indices.graphics_family = i;
            continue;
        }

        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support);
        if(present_support){
            queue_indices.present_family =i;
        }

        if (queue_indices.graphics_family != UINT32_MAX && queue_indices.present_family != UINT32_MAX) {
            INFO("Queue family selected for graphics operation :%d", queue_indices.graphics_family);
            INFO("Queue family selected for presentaion support:%d", queue_indices.present_family);
            break;
        }
    }
}

static bool create_logical_device(){
    // creating queue

    uint32_t unique_queue_families[] = {queue_indices.graphics_family, queue_indices.present_family};
    float queue_priority =  1.0f;

   
    VkDeviceQueueCreateInfo queue_create_info [2] = {};
 
    queue_create_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info[0].queueFamilyIndex = unique_queue_families[0];
    queue_create_info[0].queueCount = 1;
    queue_create_info[0].pQueuePriorities = &queue_priority;

 
    queue_create_info[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info[1].queueFamilyIndex = unique_queue_families[1];
    queue_create_info[1].queueCount = 1;
    queue_create_info[1].pQueuePriorities = &queue_priority;
  
    VkPhysicalDeviceFeatures device_features = {};

    VkDeviceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.pQueueCreateInfos = queue_create_info;
    create_info.queueCreateInfoCount = 2;
    create_info.pEnabledFeatures = &device_features;

https://stackoverflow.com/questions/68127785/how-to-fix-vk-khr-portability-subset-error-on-mac-m1-while-following-vulkan-tuto    
#if defined(__APPLE__)
    create_info.enabledExtensionCount = 2;
    create_info.ppEnabledExtensionNames = device_extensions;
#else
    create_info.enabledExtensionCount = 1;
    create_info.ppEnabledExtensionNames = device_extensions;
#endif


    if (enable_validation_layers) {
        // create the validation layer 
        const char *layer[] = {"VK_LAYER_KHRONOS_validation"};
        create_info.enabledLayerCount = (uint32_t) 1;
        create_info.ppEnabledLayerNames = layer;
    } else {
        create_info.ppEnabledLayerNames = NULL;
        create_info.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physical_device, &create_info, NULL, &logical_device) != VK_SUCCESS) {
        FATAL("Failed to create a logical device");
        return false;
    }

    vkGetDeviceQueue(logical_device, queue_indices.graphics_family, 0, &graphics_queue);
    vkGetDeviceQueue(logical_device, queue_indices.present_family, 0, &present_queue);
    return true;
}

static bool check_device_extension_support() {
    uint32_t extensions_count = 0;
    bool all_extensions_supported = false;

    vkEnumerateDeviceExtensionProperties(physical_device, NULL, &extensions_count, NULL);
    VkExtensionProperties available_extension[extensions_count];
    vkEnumerateDeviceExtensionProperties(physical_device, NULL, &extensions_count, available_extension);
    for (int j=0; j<devie_extensions_count; j++) {   
        all_extensions_supported = false;
        for (int i=0; i<extensions_count; i++) {
            if (strcmp(device_extensions[j], available_extension[i].extensionName)==0) {
                all_extensions_supported = true;
                break;
            }
        }
    }

    return all_extensions_supported;
}

static bool create_vulkan_instance() {
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
    //count ++;
    const char *apple_extra_extensions[count+2];
    for(int i=0; i<count; i++) {
        apple_extra_extensions[i] = extensions[i];
    }
    apple_extra_extensions[count] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
    // https://stackoverflow.com/questions/68127785/how-to-fix-vk-khr-portability-subset-error-on-mac-m1-while-following-vulkan-tuto
    apple_extra_extensions[count+1] = "VK_KHR_get_physical_device_properties2";
    INFO("Required extensions:")
    count +=2;
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

    if (result != VK_SUCCESS) {
        FATAL("Failed to create instance: %d", result);
        return false;
    }

    INFO("Vulkan instance created");
    return true;
}

