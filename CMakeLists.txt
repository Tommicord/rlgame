cmake_minimum_required(VERSION 4.2.0)

project(rl LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED)

add_subdirectory(Deps/glfw)

add_executable(
    rlgame
    rl/Base/Game.cpp
    rl/Base/Game.h
)
target_include_directories(
    rlgame
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(
    rlgame
    PRIVATE
    Vulkan::Vulkan
    glfw
)