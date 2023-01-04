cmake_minimum_required(VERSION 3.16)

project(minecraft
    VERSION 0.0.1
    LANGUAGES C)

set(CMAKE_C_STANDARD 99)

# Set project directories
set(PRJ_INCLUDES 
    src/)
    #lib/glfw/include)

set(PRJ_SOURCES
    src/vulkan.c
    src/log.c
    src/window.c
    src/main.c)

# Set bin directory
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "/bin")


# add any external library
# https://www.glfw.org/docs/latest/build_guide.html#build_link_cmake_source
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(lib/glfw)
add_subdirectory(lib/cglm)


# Vulkan 
find_package(Vulkan REQUIRED)


# Put all together
add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE ${PRJ_SOURCES})
target_include_directories(${PROJECT_NAME} 
    PRIVATE ${PRJ_INCLUDES}
    PRIVATE "${Vulkan_INCLUDE_DIRS}")

target_link_libraries(${PROJECT_NAME} 
    PRIVATE ${Vulkan_LIBRARY}
    PRIVATE cglm
    PRIVATE glfw)


# Setting the path to the resources
target_compile_definitions(${PROJECT_NAME} PUBLIC RESOURCES_PATH="${CMAKE_CURRENT_DIR}/rsc")



