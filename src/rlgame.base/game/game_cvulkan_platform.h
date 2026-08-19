#pragma once

#if defined(_WIN32)
#define GAME_CVULKAN_API __declspec (dllexport)
#elif defined(__APPLE__)
#define GAME_CVULKAN_API __attribute__ ((visibility ("default")))
#else
#define GAME_CVULKAN_API __attribute__ ((visibility ("default")))
#endif
