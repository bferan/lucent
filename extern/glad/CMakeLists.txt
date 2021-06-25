cmake_minimum_required(VERSION 3.17)
project(glad)

add_library(glad INTERFACE)
target_include_directories(glad INTERFACE include)
target_sources(glad INTERFACE src/glad.c)