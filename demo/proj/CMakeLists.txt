cmake_minimum_required(VERSION 3.1)
project("shader_editor")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CUSTOM_WARNINGS "-Wall -Wextra -Wno-unused-parameter -Wno-comment -Wno-unused-function -Wold-style-cast -Wsign-conversion -Wno-system-headers")
set(CMAKE_INSTALL_RPATH "$ORIGIN")
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

include(${CMAKE_SOURCE_DIR}/../.cmake/util.cmake)

add_module(stb)
add_module(glm)
add_module(cereal)
add_module(utfcpp)
add_module(lz4)
add_module(gl3w)
add_module(glfw)
add_module(opengl)
add_module(std_thread)

copy_resources(${CMAKE_SOURCE_DIR}/resources)

#set_debug()
set_release()

set(GL_GUI "glgui")
build_library(${GL_GUI} ${CMAKE_SOURCE_DIR}/base_classes ${CMAKE_SOURCE_DIR}/machine_learning ${CMAKE_SOURCE_DIR}/gui)
link_directories(${CMAKE_BINARY_DIR}/lib)

build_exec(${CMAKE_SOURCE_DIR}/tester ${GL_GUI})
