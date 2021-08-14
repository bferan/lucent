cmake_minimum_required(VERSION 3.17)
project(tinygltf)

add_library(tinygltf STATIC)
target_include_directories(tinygltf PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(tinygltf PRIVATE
        json.hpp
        tiny_gltf.cc
        mikktspace.c
        mikktspace.h
        weldmesh.c
        weldmesh.h
        tiny_gltf.h)
target_link_libraries(tinygltf PRIVATE stb)