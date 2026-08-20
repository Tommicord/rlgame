#pragma once

#if defined(R_DEVMODE)
#define R_GAME_CVULKAN_DEBUG
#endif

#if defined(R_CVULKAN_COMPILER_MSVC)
#define R_GAME_CVULKAN_API_ATTR __forceinline
#elif defined(R_CVULKAN_COMPILER_GCC) || defined(R_CVULKAN_COMPILER_CLANG)
#define R_GAME_CVULKAN_API_ATTR __attribute__ ((always_inline)) inline
#else
#define R_GAME_CVULKAN_API_ATTR inline
#endif

#if defined(_WIN32)
#ifdef R_GAME_CVULKAN_BUILDING_DLL
#define R_GAME_CVULKAN_API __declspec (dllexport)
#else
#define R_GAME_CVULKAN_API __declspec (dllimport)
#endif
#else
#define R_GAME_CVULKAN_API
#endif

#if defined(R_GAME_CVULKAN_DEBUG)
#include <stdbool.h>
#include <assert.h>
#define R_GAME_CVULKAN_DEBUG_FIELD       bool isInitialized;
#define R_GAME_CVULKAN_ASSERT(condition) assert (condition)
#else
#define R_GAME_CVULKAN_DEBUG_FIELD
#define R_GAME_CVULKAN_ASSERT(condition) ((void)0)
#endif
