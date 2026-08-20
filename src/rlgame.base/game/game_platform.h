#pragma once

#if defined(R_DEVMODE)
#define R_GAME_CVULKAN_DEBUG
#endif

#if defined(_WIN32)
#ifdef R_GAME_CVULKAN_BUILDING_DLL
#define GAME_API __declspec (dllexport)
#else
#define GAME_API __declspec (dllimport)
#endif
#else
#define GAME_API
#endif

#if defined(R_CVULKAN_DEBUG)
#include <assert.h>
#define R_GAME_CVULKAN_ASSERT(condition)        assert (condition)
#define R_GAME_CVULKAN_ALWAYS_ASSERT(condition) assert (condition)
#else
#define R_GAME_CVULKAN_ASSERT(condition)        ((void)0)
#define R_GAME_CVULKAN_ALWAYS_ASSERT(condition) ((void)0)
#endif

#if defined(R_GAME_DEBUG)
#include <stdbool.h>
#define R_GAME_CVULKAN_DEBUG_FIELD uint8_t isInitialized : 1;
#else
#define R_GAME_CVULKAN_DEBUG_FIELD
#endif
