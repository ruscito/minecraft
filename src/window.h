// window is also responsable for initalize vulkan

#pragma once

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "defines.h"




struct Button {
    bool pressed;
};

struct Position {
    double x;
    double y;
};

struct Mouse {
    struct Button button[GLFW_MOUSE_BUTTON_LAST];
    struct Position position;
    struct Position scroll;
    bool is_inside;
};

struct Keyboard {
    struct Button key[GLFW_KEY_LAST];
};

enum window_status {
    UNDEFINED,
    CREATED
}; 

struct Window {
    GLFWwindow *handle;
    int width;
    int height;
    const char *title;
    enum window_status  status;

    struct Keyboard keyboard;
    struct Mouse mouse;
};

extern struct Window window;


bool window_create(int width, int height, const char *title);
void window_loop();
void window_destroy();