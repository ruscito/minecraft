#include "vulkan_if.h"
#include "log.h"
#include "window.h"
#include "pipeline.h"

#include <string.h>
#include <stdlib.h>
#include <limits.h>

struct  queue_family_indices {
    uint32_t graphics_family;
    uint32_t present_family;
};



VkDevice logical_device = VK_NULL_HANDLE; // the logical device
swap_chain_t swap_chain;

static VkInstance instance;
static VkPhysicalDevice physical_device = VK_NULL_HANDLE; 
static VkDebugUtilsMessengerEXT debug_messenger;
static VkDebugUtilsMessengerCreateInfoEXT messanger_create_info = {};
static VkQueue graphics_queue; // handler to the graphics queue
static VkQueue present_queue; // handler to the graphics queue
static GLFWwindow *wnd;
static VkSurfaceKHR surface;
static struct queue_family_indices queue_indices; 

VkFramebuffer *swap_chain_framebuffers;
VkCommandPool command_pool;
VkCommandBuffer command_buffer;

VkSemaphore imageAvailableSemaphore;
VkSemaphore renderFinishedSemaphore;
VkFence inFlightFence;


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


struct swap_chain_support_details {
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR *formats;
    uint32_t formats_count;
    VkPresentModeKHR *present_modes;
    uint32_t present_modes_count;
} ;


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
static struct swap_chain_support_details query_swap_chain_support();
static VkSurfaceFormatKHR choose_swap_surface_format(const VkSurfaceFormatKHR  *available_formats, uint32_t count);
static VkPresentModeKHR choose_swap_present_mode(const VkPresentModeKHR *availabl_present_modes, uint32_t count); 
static VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR capabilities);
static bool create_swap_chain();
static void destroy_swap_chain();
static uint32_t clamp(uint32_t val, uint32_t min, uint32_t max);
static bool create_image_views();
static void destroy_image_views();
static bool create_framebuffers();
static void destroy_framebuffers();
static bool create_command_pool();
static void destroy_command_pool();
static bool create_command_buffer();
static void destroy_command_buffer();
static bool record_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index);
static bool create_sync_objects();
static void destroy_sync_objects();



bool init_vulkan(GLFWwindow *window){
    wnd = window;

    if (!create_vulkan_instance()) return false;
    setup_debug_messenger( &messanger_create_info);
    if (!create_surface()) return false;
    if (!pick_physical_device())  return false; // pick a GPU. This object will be implicitly destroyed whith VkInstance
    if (!create_logical_device()) return false;
    if (!create_swap_chain()) return false;
    if (!create_image_views()) return false;
    if (!create_render_passes()) return false;
    if (!create_pipeline()) return false;
    if (!create_framebuffers()) return false;
    if (!create_command_pool()) return false;
    if (!create_command_buffer()) return false;
    if (!create_sync_objects()) return false;

    return true;
}

void destroy_vulkan() {
    destroy_sync_objects();
    destroy_command_pool();
    destroy_framebuffers();
    destroy_pipeline();
    destroy_render_passes();
    destroy_image_views();
    destroy_swap_chain();
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
        // TODO:
        // maybe find a better way to close in case of a fatal vulkan error
        FATAL("Vulkan %s %s", type_info, callback_data->pMessage);
        glfwTerminate();
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

    struct swap_chain_support_details swap_chain_support = query_swap_chain_support();
    if(swap_chain_support.formats == NULL) {
        FATAL("swapchain do not support image format");
        return false;       
    }

    if(swap_chain_support.present_modes == NULL) {
        FATAL("swapchain do not support presentation mode");
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
            //continue; in case we want to select different family for the presenter 
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
    VkPhysicalDeviceFeatures device_features = {};
    VkDeviceCreateInfo create_info = {};
    VkDeviceQueueCreateInfo *queue_create_infos;
    float queue_priority =  1.0f;
    uint32_t create_info_count = 0;

    if (queue_indices.graphics_family != queue_indices.present_family) {
        uint32_t queue_families[] = {queue_indices.graphics_family, queue_indices.present_family};
        queue_create_infos = malloc (2 * sizeof(VkDeviceQueueCreateInfo));
        queue_create_infos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_infos[0].queueFamilyIndex = queue_families[0];
        queue_create_infos[0].queueCount = 1;
        queue_create_infos[0].pQueuePriorities = &queue_priority;

    
        queue_create_infos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_infos[1].queueFamilyIndex = queue_families[1];
        queue_create_infos[1].queueCount = 1;
        queue_create_infos[1].pQueuePriorities = &queue_priority;

        create_info.pQueueCreateInfos = queue_create_infos;
        create_info.queueCreateInfoCount = 2;
    } else  {
         queue_create_infos = malloc (sizeof(VkDeviceQueueCreateInfo));
    
        queue_create_infos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_infos[0].queueFamilyIndex = queue_indices.graphics_family;
        queue_create_infos[0].queueCount = 1;
        queue_create_infos[0].pQueuePriorities = &queue_priority;
        create_info.pQueueCreateInfos = queue_create_infos; 
        create_info.queueCreateInfoCount = 1; 
    }
  
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    //create_info.pQueueCreateInfos = queue_create_info; 
    //create_info.queueCreateInfoCount = 2;
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

static uint32_t clamp(uint32_t val, uint32_t min, uint32_t max) {
    const uint32_t m = val < min ? min : val;
    return m> max ? max : m;
}

static VkSurfaceFormatKHR choose_swap_surface_format(const VkSurfaceFormatKHR  *available_formats, uint32_t count){
    for (int i=0; i<count; i++) {
        if (available_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && 
            available_formats[i].colorSpace== VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return available_formats[i];
            } 
    }
    return available_formats[0];
}

static VkPresentModeKHR choose_swap_present_mode(const VkPresentModeKHR *available_present_modes, uint32_t count){
    for (int i=0; i<count; i++){
        if (available_present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            return available_present_modes[i];
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR capabilities) {
    if (capabilities.currentExtent.width !=  UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window.handle, &width, &height);

        VkExtent2D actual_extend = {};
        actual_extend.width  = clamp(actual_extend.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actual_extend.height  = clamp(actual_extend.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        return actual_extend;
    }
}

static struct swap_chain_support_details query_swap_chain_support() {
    // Swap chain support is sufficient for this tutorial if there is at least one supported 
    // image format and one supported presentation mode given the window surface we have.
    struct swap_chain_support_details details;
    details.formats = NULL;
    details.present_modes = NULL;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &details.capabilities);

    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &details.formats_count, NULL);
    if (details.formats_count > 0) {
        details.formats = malloc(details.formats_count * sizeof(*details.formats));
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &details.formats_count, details.formats);
    }


    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &details.present_modes_count, NULL);
    if (details.present_modes_count > 0) {
        details.present_modes = malloc(details.present_modes_count * sizeof(*details.present_modes));
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &details.present_modes_count, details.present_modes);
    }

    return details;
}


static bool create_swap_chain() {
    struct swap_chain_support_details details = query_swap_chain_support();

    VkSurfaceFormatKHR surface_format = choose_swap_surface_format(details.formats, details.formats_count);
    VkPresentModeKHR present_mode = choose_swap_present_mode(details.present_modes, details.present_modes_count);
    VkExtent2D extent = choose_swap_extent(details.capabilities);


    uint32_t image_count = details.capabilities.minImageCount + 1;
    if (details.capabilities.maxImageCount > 0 && image_count > details.capabilities.maxImageCount) {
        image_count = details.capabilities.maxImageCount;
    }

    // now the usual struct
    VkSwapchainCreateInfoKHR create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = surface;
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (queue_indices.graphics_family != queue_indices.present_family) {
        uint32_t queue_families[] = {queue_indices.graphics_family, queue_indices.present_family};
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_families;
    } else  {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0; // Optional
        create_info.pQueueFamilyIndices = NULL; // Optional
    }

    create_info.preTransform = details.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;
    if (vkCreateSwapchainKHR(logical_device, &create_info, NULL, &swap_chain.handle) != VK_SUCCESS) {
        FATAL("Failed to create a swap chain");
        return false;
    }

    swap_chain.image_format = surface_format.format;
    swap_chain.extent = extent;

    // retriving swap chain images
    vkGetSwapchainImagesKHR(logical_device, swap_chain.handle, &swap_chain.images_count, NULL);
    swap_chain.images = malloc(swap_chain.images_count * sizeof *swap_chain.images);
    vkGetSwapchainImagesKHR(logical_device, swap_chain.handle, &swap_chain.images_count, swap_chain.images);    
    return true;
}

static void destroy_swap_chain() {
    free(swap_chain.images);
    vkDestroySwapchainKHR(logical_device, swap_chain.handle, NULL);
}

static bool create_image_views(){
    swap_chain.image_views = malloc(swap_chain.images_count * sizeof *swap_chain.image_views);
    for (int i=0; i<swap_chain.images_count; i++) {
        VkImageViewCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = swap_chain.images[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = swap_chain.image_format;
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        if (vkCreateImageView(logical_device, &create_info, NULL, &swap_chain.image_views[i]) != VK_SUCCESS) {
            FATAL("Fail to create image views");
            return false;
        }
    }
    return true;   
}

static void destroy_image_views() {
    if (swap_chain.image_views == NULL) return;
    for (int i=0; i<swap_chain.images_count; i++) {
        vkDestroyImageView(logical_device, swap_chain.image_views[i], NULL);
    }
}

static bool create_framebuffers() {
    swap_chain_framebuffers = malloc(swap_chain.images_count * sizeof(VkFramebuffer));
    if (swap_chain_framebuffers  == NULL) {
        FATAL("Failed to allocate memoty for the framebuffers");
        return false;
    }
    
    for (int i=0; i<swap_chain.images_count; i++) {
        VkImageView attachments[] = {
            swap_chain.image_views[i]
        };

        VkFramebufferCreateInfo framebuffer_info = {};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = render_pass;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = attachments;
        framebuffer_info.width = swap_chain.extent.width;
        framebuffer_info.height = swap_chain.extent.height;
        framebuffer_info.layers = 1;

        if (vkCreateFramebuffer(logical_device, &framebuffer_info, NULL, &swap_chain_framebuffers[i]) != VK_SUCCESS) {
            FATAL("Failed to create framebuffers");
            return false;
        }    
    }
    return true;
}

static void destroy_framebuffers() {
    for (int i=0; i<swap_chain.images_count; i++) {
        vkDestroyFramebuffer(logical_device, swap_chain_framebuffers[i], NULL);
    }
}

static bool create_command_pool(){
    VkCommandPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = queue_indices.graphics_family;
    if (vkCreateCommandPool(logical_device, &pool_info, NULL, &command_pool) != VK_SUCCESS) {
        FATAL("Failed to create command pool");
        return false;
    }
    return true;
}

static void destroy_command_pool(){
    vkDestroyCommandPool(logical_device, command_pool, NULL);
}

static bool create_command_buffer(){
    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(logical_device, &alloc_info, &command_buffer) != VK_SUCCESS) {
        FATAL("Failed to create command buffer");
        return false;
    }
    return true;
}

static bool record_command_buffer(VkCommandBuffer cmd_buffer, uint32_t image_index){
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(cmd_buffer, &beginInfo) != VK_SUCCESS) {
        FATAL("Failed to begin recording command buffer!");
        return false;
    }

    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = render_pass;
    renderPassInfo.framebuffer = swap_chain_framebuffers[image_index];

    renderPassInfo.renderArea.offset.x = 0;
    renderPassInfo.renderArea.offset.y = 0;
    renderPassInfo.renderArea.extent = swap_chain.extent;

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(cmd_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) swap_chain.extent.width;
        viewport.height = (float) swap_chain.extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd_buffer, 0, 1, &viewport);

        VkRect2D scissor = {};
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        scissor.extent =  swap_chain.extent;
        vkCmdSetScissor(cmd_buffer, 0, 1, &scissor);    

        vkCmdDraw(cmd_buffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(cmd_buffer);

    if (vkEndCommandBuffer(cmd_buffer ) != VK_SUCCESS) {
        FATAL("Failed to record command buffer!");
        return false;
    }

    return true;
}

static bool create_sync_objects(){
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateSemaphore(logical_device, &semaphoreInfo, NULL, &imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(logical_device, &semaphoreInfo, NULL, &renderFinishedSemaphore) != VK_SUCCESS ||
        vkCreateFence(logical_device, &fenceInfo, NULL, &inFlightFence) != VK_SUCCESS) {
        FATAL("Failed to create semaphores!");
        return false;
    }
    return true;
}

static void destroy_sync_objects(){
    vkDestroySemaphore(logical_device, imageAvailableSemaphore, NULL);
    vkDestroySemaphore(logical_device, renderFinishedSemaphore, NULL);
    vkDestroyFence(logical_device, inFlightFence, NULL);
}

void draw_frame() {
    vkWaitForFences(logical_device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(logical_device, 1, &inFlightFence);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(logical_device, swap_chain.handle, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    vkResetCommandBuffer(command_buffer, 0);
  
    record_command_buffer(command_buffer, imageIndex);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[1] = {imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[1] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &command_buffer;

    VkSemaphore signalSemaphores[1] = {renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(graphics_queue, 1, &submitInfo, inFlightFence) != VK_SUCCESS) {
        FATAL("Failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[1] = {swap_chain.handle};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(present_queue, &presentInfo);
}