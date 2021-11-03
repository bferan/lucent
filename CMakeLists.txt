cmake_minimum_required(VERSION 3.17)
project(lucent)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

add_library(lucent STATIC)
add_subdirectory(src)

# Demos
add_subdirectory(demos)

# Glad
add_subdirectory(extern/glad)
target_link_libraries(lucent PRIVATE glad)

# Vulkan
find_package(Vulkan)
target_link_libraries(lucent PUBLIC Vulkan::Vulkan)

# VulkanMemoryAllocator
add_subdirectory(extern/VulkanMemoryAllocator)
target_link_libraries(lucent PRIVATE VulkanMemoryAllocator)
target_link_libraries(VulkanMemoryAllocator PRIVATE Vulkan::Vulkan)

# glslang
add_subdirectory(extern/glslang)
target_link_libraries(lucent PRIVATE glslang)
target_link_libraries(lucent PRIVATE SPIRV)

# SPIRV
add_subdirectory(extern/SPIRV-Headers)
target_link_libraries(lucent PRIVATE SPIRV-Headers)

add_subdirectory(extern/SPIRV-Tools)
target_link_libraries(lucent PRIVATE SPIRV-Tools)

# glfw
add_subdirectory(extern/glfw)
target_link_libraries(lucent PRIVATE glfw)

# renderdoc
add_subdirectory(extern/renderdoc)
target_link_libraries(lucent PRIVATE renderdoc)

# stb
add_subdirectory(extern/stb)
target_link_libraries(lucent PRIVATE stb)

# tinygltf
add_subdirectory(extern/tinygltf)
target_link_libraries(lucent PRIVATE tinygltf)

# FMT
add_subdirectory(extern/fmt)
target_link_libraries(lucent PUBLIC fmt)