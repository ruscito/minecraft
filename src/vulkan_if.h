#pragma once

#include "defines.h"
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include <vulkan/vulkan.h>


bool init_vulkan(GLFWwindow *window);

void destroy_vulkan();

