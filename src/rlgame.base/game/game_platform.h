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

#if defined(_WIN32)
#include <windows.h>
#define R_GAME_ATOMIC_INT32 volatile LONG
#define R_GAME_ATOMIC_UINT32 volatile ULONG
#define R_GAME_ATOMIC_UINT64 volatile ULONG64
#define R_GAME_ATOMIC_PTR volatile PVOID

#define R_GAME_ATOMIC_INCREMENT(p) InterlockedIncrement ((LONG*)(p))
#define R_GAME_ATOMIC_DECREMENT(p) InterlockedDecrement ((LONG*)(p))
#define R_GAME_ATOMIC_COMPARE_EXCHANGE(p, expected, desired) \
        InterlockedCompareExchange ((LONG*)(p), (LONG)(desired), (LONG)(expected))
#define R_GAME_ATOMIC_EXCHANGE(p, value) InterlockedExchange ((LONG*)(p), (LONG)(value))
#define R_GAME_ATOMIC_LOAD(p) (*(p))
#define R_GAME_ATOMIC_STORE(p, value) (*(p) = (value))
#define R_GAME_ATOMIC_ADD(p, value) InterlockedAdd ((LONG*)(p), (LONG)(value))

#elif defined(__linux__) || defined(__APPLE__)
#include <stdatomic.h>
#define R_GAME_ATOMIC_INT32 volatile atomic_int
#define R_GAME_ATOMIC_UINT32 volatile atomic_uint
#define R_GAME_ATOMIC_UINT64 volatile atomic_uint_least64_t
#define R_GAME_ATOMIC_PTR volatile atomic_void_p

#define R_GAME_ATOMIC_INCREMENT(p) atomic_fetch_add_explicit ((atomic_int*)(p), 1, memory_order_relaxed) + 1
#define R_GAME_ATOMIC_DECREMENT(p) atomic_fetch_sub_explicit ((atomic_int*)(p), 1, memory_order_relaxed) - 1
#define R_GAME_ATOMIC_COMPARE_EXCHANGE(p, expected, desired) \
        atomic_compare_exchange_strong_explicit ((atomic_int*)(p), (int*)(expected), (int)(desired), memory_order_acq_rel, memory_order_relaxed)
#define R_GAME_ATOMIC_EXCHANGE(p, value) atomic_exchange_explicit ((atomic_int*)(p), (int)(value), memory_order_acq_rel)
#define R_GAME_ATOMIC_LOAD(p) atomic_load_explicit ((atomic_int*)(p), memory_order_acquire)
#define R_GAME_ATOMIC_STORE(p, value) atomic_store_explicit ((atomic_int*)(p), (int)(value), memory_order_release)
#define R_GAME_ATOMIC_ADD(p, value) atomic_fetch_add_explicit ((atomic_int*)(p), (int)(value), memory_order_relaxed)

#endif

#if defined(_WIN32)
#define R_GAME_THREAD_HANDLE HANDLE
#define R_GAME_THREAD_ID DWORD
#define R_GAME_MUTEX CRITICAL_SECTION
#define R_GAME_CONDITION_VARIABLE CONDITION_VARIABLE

#define R_GAME_MUTEX_INIT(p) InitializeCriticalSection ((CRITICAL_SECTION*)(p))
#define R_GAME_MUTEX_DESTROY(p) DeleteCriticalSection ((CRITICAL_SECTION*)(p))
#define R_GAME_MUTEX_LOCK(p) EnterCriticalSection ((CRITICAL_SECTION*)(p))
#define R_GAME_MUTEX_UNLOCK(p) LeaveCriticalSection ((CRITICAL_SECTION*)(p))
#define R_GAME_COND_INIT(p) InitializeConditionVariable ((CONDITION_VARIABLE*)(p))
#define R_GAME_COND_DESTROY(p) ((void)0)
#define R_GAME_COND_WAIT(pCond, pMutex) SleepConditionVariableCS ((CONDITION_VARIABLE*)(pCond), (CRITICAL_SECTION*)(pMutex), INFINITE)
#define R_GAME_COND_SIGNAL(p) WakeConditionVariable ((CONDITION_VARIABLE*)(p))
#define R_GAME_COND_BROADCAST(p) WakeAllConditionVariable ((CONDITION_VARIABLE*)(p))

#elif defined(__linux__) || defined(__APPLE__)
#include <pthread.h>
#define R_GAME_THREAD_HANDLE pthread_t
#define R_GAME_THREAD_ID pthread_t
#define R_GAME_MUTEX pthread_mutex_t
#define R_GAME_CONDITION_VARIABLE pthread_cond_t

#define R_GAME_MUTEX_INIT(p) pthread_mutex_init ((pthread_mutex_t*)(p), NULL)
#define R_GAME_MUTEX_DESTROY(p) pthread_mutex_destroy ((pthread_mutex_t*)(p))
#define R_GAME_MUTEX_LOCK(p) pthread_mutex_lock ((pthread_mutex_t*)(p))
#define R_GAME_MUTEX_UNLOCK(p) pthread_mutex_unlock ((pthread_mutex_t*)(p))
#define R_GAME_COND_INIT(p) pthread_cond_init ((pthread_cond_t*)(p), NULL)
#define R_GAME_COND_DESTROY(p) pthread_cond_destroy ((pthread_cond_t*)(p))
#define R_GAME_COND_WAIT(pCond, pMutex) pthread_cond_wait ((pthread_cond_t*)(pCond), (pthread_mutex_t*)(pMutex))
#define R_GAME_COND_SIGNAL(p) pthread_cond_signal ((pthread_cond_t*)(p))
#define R_GAME_COND_BROADCAST(p) pthread_cond_broadcast ((pthread_cond_t*)(p))

#endif

#define R_GAME_RENDERER_MAX_FRAMES_IN_FLIGHT 3
#define R_GAME_RENDERER_MAX_LAYERS 16
#define R_GAME_RENDERER_MAX_RESOURCES 1024
#define R_GAME_RENDERER_MAX_COMMAND_BUFFERS_PER_FRAME 8
#define R_GAME_RENDERER_MAX_WORKER_THREADS 4
#define R_GAME_RENDERER_STATE_STOPPED 0
#define R_GAME_RENDERER_STATE_RUNNING 1
#define R_GAME_RENDERER_STATE_PAUSED 2
#define R_GAME_RENDERER_STATE_ERROR 3
#define R_GAME_RENDERER_LAYER_FLAG_ENABLED 0x01
#define R_GAME_RENDERER_LAYER_FLAG_TRANSPARENT 0x02
#define R_GAME_RENDERER_LAYER_FLAG_POST_PROCESS 0x04

#if defined(R_GAME_DEBUG)
#define R_GAME_VALIDATE_PARAM(Data) \
        do { \
                if (Data) { \
                        return -1; \
                } \
        } while (0)

#define R_GAME_VALIDATE_SUBSYSTEM(Data) \
        R_GAME_VALIDATE_PARAM (Data)

#define R_GAME_VALIDATE_SUBSYSTEM_INITIALIZED(Data) \
        do { \
                R_GAME_VALIDATE_SUBSYSTEM (Data); \
                if (!(Data)->isInitialized) { \
                        R_CSTL_LOG_ERROR ("Renderer subsystem not initialized"); \
                        return -1; \
                } \
        } while (0)
#else
#define R_GAME_VALIDATE_PARAM(Data) ((void)0)
#define R_GAME_VALIDATE_SUBSYSTEM(Data) ((void)0)
#define R_GAME_VALIDATE_SUBSYSTEM_INITIALIZED(Data) ((void)0)
#endif
