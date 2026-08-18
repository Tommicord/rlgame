#pragma once

#if defined(_WIN32) || defined(_WIN64)
#define R_CVULKAN_PLATFORM_WINDOWS 1
#elif defined(__linux__)
#define R_CVULKAN_PLATFORM_LINUX 1
#elif defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_OS_MAC
#define R_CVULKAN_PLATFORM_MACOS 1
#endif
#elif defined(__ANDROID__)
#define R_CVULKAN_PLATFORM_ANDROID 1
#endif

#if defined(R_DEVMODE)
#define R_CVULKAN_DEBUG
#endif

#if defined(R_CVULKAN_DEBUG)
#include <assert.h>
#define R_CVULKAN_ASSERT(condition)        assert (condition)
#define R_CVULKAN_ALWAYS_ASSERT(condition) assert (condition)
#else
#define R_CVULKAN_ASSERT(condition)        ((void)0)
#define R_CVULKAN_ALWAYS_ASSERT(condition) ((void)0)
#endif

#if defined(R_CVULKAN_COMPILER_MSVC)
#define R_CVULKAN_API_ATTR __forceinline
#elif defined(R_CVULKAN_COMPILER_GCC) || defined(R_CVULKAN_COMPILER_CLANG)
#define R_CVULKAN_API_ATTR __attribute__ ((always_inline)) inline
#else
#define R_CVULKAN_API_ATTR inline
#endif

#if defined(_WIN32)
#ifdef R_CVULKAN_BUILDING_DLL
#define R_CVULKAN_API __declspec (dllexport)
#else
#define R_CVULKAN_API __declspec (dllimport)
#endif
#else
#define R_CVULKAN_API
#endif

#if defined(R_CVULKAN_DEBUG)
#include <stdbool.h>
#define R_CVULKAN_DEBUG_FIELD bool isInitialized;
#define R_CVULKAN_IS_INITIALIZED_RETURN(pObj) ((pObj)->isInitialized)
#else
#define R_CVULKAN_DEBUG_FIELD
#define R_CVULKAN_IS_INITIALIZED_RETURN(pObj) (1)
#endif