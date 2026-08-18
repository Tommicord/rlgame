cmake_minimum_required(VERSION 3.22.1)
project("rlgame_android")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

add_library(rlgame_lib STATIC IMPORTED)

if(ANDROID_ABI STREQUAL "arm64-v8a")
    set_target_properties(rlgame_lib PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/../jniLibs/arm64-v8a/librlgame_lib.a)
elseif(ANDROID_ABI STREQUAL "armeabi-v7a")
    set_target_properties(rlgame_lib PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/../jniLibs/armeabi-v7a/librlgame_lib.a)
elseif(ANDROID_ABI STREQUAL "x86_64")
    set_target_properties(rlgame_lib PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/../jniLibs/x86_64/librlgame_lib.a)
elseif(ANDROID_ABI STREQUAL "x86")
    set_target_properties(rlgame_lib PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/../jniLibs/x86/librlgame_lib.a)
endif()

add_library(rlgame_android SHARED
    android_native_bridge.c
)

target_link_libraries(rlgame_android
    rlgame_lib
    android
    log
    EGL
    GLESv3
)

target_include_directories(rlgame_android PRIVATE
    ${CMAKE_SOURCE_DIR}/../../../../src/rlgame.base
    ${CMAKE_SOURCE_DIR}/../../../../src/rlgame.base/cstl
)
