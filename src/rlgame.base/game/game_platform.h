#pragma once

#if defined(_WIN32)
#ifdef R_GAME_CVULKAN_BUILDING_DLL
#define GAME_API __declspec (dllexport)
#else
#define GAME_API __declspec (dllimport)
#endif
#else
#define GAME_API
#endif

#if defined(R_DEVMODE)
#define R_GAME_DEBUG
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
#define R_GAME_DEBUG_FIELD uint8_t isInitialized : 1;
#else
#define R_GAME_DEBUG_FIELD
#endif

#define R_GAME_RENDERER_MAX_FRAMES_IN_FLIGHT 3
#define R_GAME_RENDERER_MAX_LAYERS 16
#define R_GAME_RENDERER_MAX_RESOURCES 1024
#define R_GAME_RENDERER_MAX_COMMAND_BUFFERS_PER_FRAME 8
#define R_GAME_RENDERER_STATE_STOPPED 0
#define R_GAME_RENDERER_STATE_RUNNING 1
#define R_GAME_RENDERER_STATE_PAUSED 2
#define R_GAME_RENDERER_STATE_ERROR 3
#define R_GAME_RENDERER_LAYER_FLAG_ENABLED 0x01
#define R_GAME_RENDERER_LAYER_FLAG_TRANSPARENT 0x02
#define R_GAME_RENDERER_LAYER_FLAG_POST_PROCESS 0x04
#define R_GAME_RENDERER_RESOURCE_TYPE_BUFFER 0
#define R_GAME_RENDERER_RESOURCE_TYPE_IMAGE 1
#define R_GAME_RENDERER_RESOURCE_TYPE_DESCRIPTOR_SET 2
#define R_GAME_RENDERER_RESOURCE_TYPE_PIPELINE 3
