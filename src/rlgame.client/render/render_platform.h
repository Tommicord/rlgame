#pragma once

#if defined(_WIN32)
#ifdef R_RENDER_BUILDING_DLL
#define R_RENDER_API __declspec (dllexport)
#else
#define R_RENDER_API __declspec (dllimport)
#endif
#else
#define R_RENDER_API
#endif

#if defined(R_DEVMODE)
#define R_RENDER_DEBUG
#endif

#if defined(R_RENDER_DEBUG)
#include <assert.h>
#define R_RENDER_ASSERT(condition) assert (condition)
#else
#define R_RENDER_ASSERT(condition) ((void)0)
#endif
