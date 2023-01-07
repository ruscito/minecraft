#include "window.h"
#include "defines.h"
#include "log.h"
#include "vulkan.h"


// the global window
struct Window window;


static void error_callback(int error, const char *description) {
    ERROR("GLFW: %s [%n]", description, error);
}

static void key_callback(GLFWwindow *_window, int key, int scancode, int action, int mods){
    // if (key >0 && action == GLFW_PRESS) {
    //     window.keyboard.key[key].pressed = true;
    // } else {
    //     window.keyboard.key[key].pressed = false;
    // }
    window.keyboard.key[key].pressed = action == GLFW_PRESS;


}

static void mouse_button_callback(GLFWwindow* _window, int button, int action, int mods)
{
    // if (button > 0 && action == GLFW_PRESS) {
    //     window.mouse.button[button].pressed = true;
    // } else {
    //     window.mouse.button[button].pressed = false;
    // }
    window.mouse.button[button].pressed  = action == GLFW_PRESS;
}

static void cursor_position_callback(GLFWwindow* _window, double xpos, double ypos) {
    window.mouse.position.x = xpos;
    window.mouse.position.y = ypos;
}

static void scroll_callback(GLFWwindow* _window, double xoffset, double yoffset)
{
    window.mouse.scroll.x = xoffset;
    window.mouse.scroll.y = yoffset;
}

static void cursor_enter_callback(GLFWwindow* _window, int entered) {
    window.mouse.is_inside = entered;
}


bool window_create(int width, int height, const char *title) {
     if (!glfwInit()) {
        FATAL("Failed to initialize GLFW");
        return FAIL;
    } 

    window.status = CREATED;   
    


    // Create the window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window.handle = glfwCreateWindow(width, height, title, NULL, NULL);
    if (! window.handle) {
        FATAL("Failed to create GLFW window");
        return FAIL;
    }

    glfwSetErrorCallback(error_callback);
    glfwSetKeyCallback(window.handle, key_callback);
    glfwSetCursorPosCallback(window.handle, cursor_position_callback);
    
    glfwSetMouseButtonCallback(window.handle, mouse_button_callback);
    glfwSetScrollCallback(window.handle, scroll_callback);
    glfwSetCursorEnterCallback(window.handle, cursor_enter_callback);
    
    if (!init_vulkan()) {
        return false;
    }

    return true;
}

void window_loop() {
 
    while (!glfwWindowShouldClose(window.handle))
    {
        glfwPollEvents();
        glfwSetWindowShouldClose(window.handle, window.keyboard.key[GLFW_KEY_Q].pressed);

        if (window.keyboard.key[GLFW_KEY_ESCAPE].pressed && glfwGetInputMode(window.handle, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
            glfwSetInputMode(window.handle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }

        if (window.mouse.button[GLFW_MOUSE_BUTTON_LEFT].pressed && glfwGetInputMode(window.handle, GLFW_CURSOR) == GLFW_CURSOR_NORMAL) {
             glfwSetInputMode(window.handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } 
    }
}

void window_destroy() {
    destroy_vulkan();

    if (window.handle){
        glfwDestroyWindow(window.handle);
        INFO("GLFW window destroyed");   
    }

    if (window.status != UNDEFINED) {
        glfwTerminate();
        INFO("GLFW terminated");   
    }   
}
