#pragma once

#if defined(R_DEVMODE)
#define R_BASE_DEBUG
#endif

#if defined(_WIN32)
#if defined(R_ENTRY_BUILDING_DLL)
#define R_ENTRY_API __declspec(dllexport)
#else
#define R_ENTRY_API __declspec(dllimport)
#endif
#else
#define R_ENTRY_API
#endif