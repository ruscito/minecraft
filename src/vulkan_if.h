#pragma once

#include "defines.h"
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include <vulkan/vulkan.h>

typedef struct swap_chain {
    VkSwapchainKHR handle;      // handle to the swap chain not sure we realy need this
    uint32_t images_count;      // number of images in the swap chain
    VkImage *images;            // the array of images
    VkImageView *image_views;   // the view into the image
    VkFormat image_format;  
    VkExtent2D extent;      
} swap_chain_t;


bool init_vulkan(GLFWwindow *window);

void destroy_vulkan();

extern VkDevice logical_device;
extern swap_chain_t swap_chain;