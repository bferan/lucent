cmake_minimum_required(VERSION 3.17)
project(stb)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})