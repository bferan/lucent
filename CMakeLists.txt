cmake_minimum_required(VERSION 3.17)
project(lucent)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

add_library(lucent STATIC)
add_subdirectory(src)

# Glad
add_subdirectory(extern/glad)
target_link_libraries(lucent PRIVATE glad)

# Vulkan
find_package(Vulkan)
target_link_libraries(lucent PRIVATE Vulkan::Vulkan)

# VulkanMemoryAllocator
add_subdirectory(extern/VulkanMemoryAllocator)
target_link_libraries(lucent PRIVATE VulkanMemoryAllocator)
target_link_libraries(VulkanMemoryAllocator PRIVATE Vulkan::Vulkan)

#glslang
add_subdirectory(extern/glslang)
target_link_libraries(lucent PRIVATE glslang)

# SPIRV
add_subdirectory(extern/SPIRV-Headers)
target_link_libraries(lucent PRIVATE SPIRV-Headers)

add_subdirectory(extern/SPIRV-Tools)
target_link_libraries(lucent PRIVATE SPIRV-Tools)

# Shaderc
set(SHADERC_SKIP_TESTS ON)
add_subdirectory(extern/shaderc)
target_link_libraries(lucent PRIVATE shaderc)

# glfw
add_subdirectory(extern/glfw)
target_link_libraries(lucent PRIVATE glfw)

# renderdoc
add_subdirectory(extern/renderdoc)
target_link_libraries(lucent PRIVATE renderdoc)

# Assimp
add_subdirectory(extern/assimp)
target_link_libraries(lucent PRIVATE assimp)

# stb
add_subdirectory(extern/stb)
target_link_libraries(lucent PRIVATE stb)