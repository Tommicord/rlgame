#pragma once

#if defined(_WIN32) || defined(_WIN64)
#define R_MICROBIT_PLATFORM_WINDOWS 1
#elif defined(__linux__)
#define R_MICROBIT_PLATFORM_LINUX 1
#elif defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_OS_MAC
#define R_MICROBIT_PLATFORM_MACOS 1
#endif
#elif defined(__ANDROID__)
#define R_MICROBIT_PLATFORM_ANDROID 1
#endif

#if defined(R_DEVMODE)
#define R_MICROBIT_DEBUG
#endif

#if defined(R_MICROBIT_DEBUG)
#include <assert.h>
#define R_MICROBIT_ASSERT(condition) assert (condition)
#else
#define R_MICROBIT_ASSERT(condition) ((void)0)
#endif

#if defined(_WIN32)
#ifdef R_MICROBIT_BUILDING_DLL
#define R_MICROBIT_API __declspec (dllexport)
#else
#define R_MICROBIT_API __declspec (dllimport)
#endif
#else
#define R_MICROBIT_API
#endif