cmake_minimum_required(VERSION 3.17)
project(renderdoc)

add_library(renderdoc STATIC)
target_include_directories(renderdoc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(renderdoc PRIVATE renderdoc_app.cpp)