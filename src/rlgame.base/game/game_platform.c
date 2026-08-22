#include "rlgame.base/game/game_platform.h"

R_GAME_API const char* 
R_GameErrorToString (enum R_GameError error)
{
        switch (error)
        {
                case R_GAME_OK: 
                        return "Success";
                case R_GAME_ERROR_FAILED: 
                        return "General failure";
                case R_GAME_ERROR_OUT_OF_MEMORY: 
                        return "Out of memory";
                case R_GAME_ERROR_INVALID_ARGUMENT: 
                        return "Invalid argument";
                case R_GAME_ERROR_NULL_POINTER: 
                        return "Null pointer";
                case R_GAME_ERROR_NOT_INITIALIZED: 
                        return "Not initialized";
                case R_GAME_ERROR_ALREADY_INITIALIZED: 
                        return "Already initialized";
                case R_GAME_ERROR_INVALID_STATE: 
                        return "Invalid state";
                case R_GAME_ERROR_RESOURCE_NOT_FOUND: 
                        return "Resource not found";
                case R_GAME_ERROR_RESOURCE_ALREADY_EXISTS: 
                        return "Resource already exists";
                case R_GAME_ERROR_MAX_RESOURCES_REACHED: 
                        return "Max resources reached";
                case R_GAME_ERROR_INVALID_HANDLE: 
                        return "Invalid handle";
                case R_GAME_ERROR_LAYER_NOT_FOUND: 
                        return "Layer not found";
                case R_GAME_ERROR_THREAD_CREATE_FAILED: 
                        return "Thread creation failed";
                case R_GAME_ERROR_INDEX_OUT_OF_BOUNDS: 
                        return "Index out of bounds";
                case R_GAME_ERROR_ARRAY_OPERATION_FAILED: 
                        return "Array operation failed";
                case R_GAME_ERROR_RENDERER_NOT_SET: 
                        return "Renderer not set";
                case R_GAME_ERROR_FRAMEBUFFER_NOT_READY: 
                        return "Framebuffer not ready";
                case R_GAME_ERROR_COMMAND_BUFFER_FAILED: 
                        return "Command buffer failed";
                case R_GAME_ERROR_SUBSYSTEM_NOT_FOUND: 
                        return "Subsystem not found";
                case R_GAME_ERROR_VALIDATION_FAILED: 
                        return "Validation failed";
                case R_GAME_ERROR_INITIALIZATION_FAILED: 
                        return "Initialization failed";
                default: 
                        return "Unknown error";
        }
}