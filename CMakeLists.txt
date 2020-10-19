# Project
cmake_minimum_required(VERSION 3.0.0)
project(Termui VERSION 0)

# C++ compiler settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
string(APPEND CMAKE_CXX_FLAGS " -Wall -Wextra -Werror -pipe")
string(APPEND CMAKE_CXX_FLAGS_RELEASE " -flto -s")

# main executable
add_executable(termui-demo
    src/termui.cpp
    src/termui_internal.cpp
    demo/main.cpp
)
target_include_directories(termui-demo PRIVATE src)