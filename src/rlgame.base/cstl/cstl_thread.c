#include "rlgame.base/cstl/cstl_thread.h"

#define R_CSTL_INLINE
#include "rlgame.base/cstl/cstl_platform.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"

#include <stdbool.h>

#ifndef R_CSTL_THREAD_DEBUG
#define R_CSTL_THREAD_DEBUG 0
#endif

#if defined(R_CSTL_PLATFORM_WINDOWS)
#include <windows.h>
#elif defined(R_CSTL_PLATFORM_LINUX)
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/syscall.h>
#else
#error "Unsupported platform for cstl_thread"
#endif

#if defined(R_CSTL_PLATFORM_WINDOWS)

struct r_cstl_thread
{
        HANDLE handle;
        DWORD  threadId;
};

typedef DWORD (WINAPI* Win32ThreadFunc) (LPVOID lpParam);

static DWORD WINAPI
Win32ThreadEntry (LPVOID lpParam)
{
    struct
    {
            r_cstl_thread_func pFunc;
            void*             pData;
    }* pParams = lpParam;

    r_cstl_thread_func pFunc = pParams->pFunc;
    void*             pData = pParams->pData;

    r_cstl_heap_free (pParams);

    pFunc (pData);
    return 0;
}

struct r_cstl_thread*
r_cstl_new_thread (r_cstl_thread_func pFunc, void* pData)
{
#if R_CSTL_THREAD_DEBUG
    if (pFunc == NULL)
    {
        return NULL;
    }
#endif
    struct
    {
            r_cstl_thread_func pFunc;
            void*             pData;
    }* pParams = r_cstl_heap_alloc (sizeof (*pParams));

#if R_CSTL_THREAD_DEBUG
    if (pParams == NULL)
    {
        return NULL;
    }
#endif

    pParams->pFunc = pFunc;
    pParams->pData = pData;

    struct r_cstl_thread* pThread = r_cstl_heap_alloc (sizeof (struct r_cstl_thread));
#if R_CSTL_THREAD_DEBUG
    if (pThread == NULL)
    {
        r_cstl_heap_free (pParams);
        return NULL;
    }
    r_cstl_heap_register_allocation (
        pThread,
        pThread,
        sizeof (struct r_cstl_thread),
        R_CSTL_HEAP_NAME (r_cstl_thread));
#endif

    pThread->handle = CreateThread (NULL, 0, Win32ThreadEntry, pParams, 0, &pThread->threadId);

#if R_CSTL_THREAD_DEBUG
    if (pThread->handle == NULL)
    {
        r_cstl_heap_unregister_allocation (pThread, pThread);
        r_cstl_heap_free (pParams);
        r_cstl_heap_free (pThread);
        return NULL;
    }
#endif

    return pThread;
}

int
r_cstl_thread_join (struct r_cstl_thread* pThread)
{
#if R_CSTL_THREAD_DEBUG
    if (pThread == NULL)
    {
        return R_CSTL_ERROR_INVALID_ARGUMENT;
    }
#endif

    DWORD waitResult = WaitForSingleObject (pThread->handle, INFINITE);

#if R_CSTL_THREAD_DEBUG
    if (waitResult != WAIT_OBJECT_0)
    {
        CloseHandle (pThread->handle);
        r_cstl_heap_unregister_allocation (pThread, pThread);
        r_cstl_heap_free (pThread);
        return R_CSTL_ERROR_THREAD_JOIN_FAILED;
    }
#endif

    CloseHandle (pThread->handle);
#if R_CSTL_THREAD_DEBUG
    r_cstl_heap_unregister_allocation (pThread, pThread);
#endif
    r_cstl_heap_free (pThread);

    return R_CSTL_OK;
}

uint64_t
r_cstl_thread_get_current_id (void)
{
    return (uint64_t)GetCurrentThreadId ();
}

void
r_cstl_thread_yield (void)
{
    SwitchToThread ();
}

void
r_cstl_thread_sleep (uint32_t milliseconds)
{
    Sleep (milliseconds);
}

#elif defined(R_CSTL_PLATFORM_LINUX)

struct r_cstl_thread
{
        pthread_t thread;
        int       joined;
};

struct ThreadParams
{
        r_cstl_thread_func pFunc;
        void*             pData;
};

static void*
PthreadThreadEntry (void* arg)
{
    struct ThreadParams* pParams = arg;
    r_cstl_thread_func    pFunc = pParams->pFunc;
    void*                pData = pParams->pData;

    r_cstl_heap_free (pParams);

    pFunc (pData);
    return NULL;
}

struct r_cstl_thread*
r_cstl_new_thread (r_cstl_thread_func pFunc, void* pData)
{
#if R_CSTL_THREAD_DEBUG
    if (pFunc == NULL)
    {
        return NULL;
    }
#endif

    struct ThreadParams* pParams = r_cstl_heap_alloc (sizeof (struct ThreadParams));
#if R_CSTL_THREAD_DEBUG
    if (pParams == NULL)
    {
        return NULL;
    }
#endif

    pParams->pFunc = pFunc;
    pParams->pData = pData;

    struct r_cstl_thread* pThread = r_cstl_heap_alloc (sizeof (struct r_cstl_thread));
#if R_CSTL_THREAD_DEBUG
    if (pThread == NULL)
    {
        r_cstl_heap_free (pParams);
        return NULL;
    }
    r_cstl_heap_register_allocation (
        pThread,
        pThread,
        sizeof (struct r_cstl_thread),
        R_CSTL_HEAP_NAME (r_cstl_thread));
#endif

    pThread->joined = 0;

    int result = pthread_create (&pThread->thread, NULL, PthreadThreadEntry, pParams);
#if R_CSTL_THREAD_DEBUG
    if (result != 0)
    {
        r_cstl_heap_unregister_allocation (pThread, pThread);
        r_cstl_heap_free (pParams);
        r_cstl_heap_free (pThread);
        return NULL;
    }
#endif

    return pThread;
}

int
r_cstl_thread_join (struct r_cstl_thread* pThread)
{
#if R_CSTL_THREAD_DEBUG
    if (pThread == NULL)
    {
        return R_CSTL_ERROR_INVALID_ARGUMENT;
    }

    if (pThread->joined)
    {
#if R_CSTL_THREAD_DEBUG
        r_cstl_heap_unregister_allocation (pThread, pThread);
#endif
        r_cstl_heap_free (pThread);
        return R_CSTL_ERROR_INVALID_ARGUMENT;
    }
#endif

    int result = pthread_join (pThread->thread, NULL);
#if R_CSTL_THREAD_DEBUG
    if (result != 0)
    {
        r_cstl_heap_unregister_allocation (pThread, pThread);
        r_cstl_heap_free (pThread);
        return R_CSTL_ERROR_THREAD_JOIN_FAILED;
    }
#endif

#if R_CSTL_THREAD_DEBUG
    r_cstl_heap_unregister_allocation (pThread, pThread);
#endif
    r_cstl_heap_free (pThread);
    return R_CSTL_OK;
}

uint64_t
r_cstl_thread_get_current_id (void)
{
    return (uint64_t)syscall (SYS_gettid);
}

void
r_cstl_thread_yield (void)
{
    sched_yield ();
}

void
r_cstl_thread_sleep (uint32_t milliseconds)
{
    usleep (milliseconds * 1000);
}

#endif

#if defined(R_CSTL_PLATFORM_WINDOWS)

struct r_cstl_mutex
{
        CRITICAL_SECTION cs;
};

struct r_cstl_mutex*
r_cstl_new_mutex (void)
{
    struct r_cstl_mutex* pMutex = r_cstl_heap_alloc (sizeof (struct r_cstl_mutex));
#if R_CSTL_THREAD_DEBUG
    if (pMutex == NULL)
    {
        return NULL;
    }
    r_cstl_heap_register_allocation (
        pMutex,
        pMutex,
        sizeof (struct r_cstl_mutex),
        R_CSTL_HEAP_NAME (r_cstl_mutex));
#endif
    InitializeCriticalSection (&pMutex->cs);
    return pMutex;
}

void
r_cstl_mutex_destroy (struct r_cstl_mutex* pMutex)
{
    if (pMutex != NULL)
    {
#if R_CSTL_THREAD_DEBUG
        DeleteCriticalSection (&pMutex->cs);
        r_cstl_heap_unregister_allocation (pMutex, pMutex);
#endif
        r_cstl_heap_free (pMutex);
    }
}

int
r_cstl_mutex_lock (struct r_cstl_mutex* pMutex)
{
#if R_CSTL_THREAD_DEBUG
    if (pMutex == NULL)
    {
        return R_CSTL_ERROR_INVALID_ARGUMENT;
    }
#endif

    EnterCriticalSection (&pMutex->cs);
    return R_CSTL_OK;
}

int
r_cstl_mutex_try_lock (struct r_cstl_mutex* pMutex, int* pOutLocked)
{
#if R_CSTL_THREAD_DEBUG
    if (pMutex == NULL || pOutLocked == NULL)
    {
        return R_CSTL_ERROR_INVALID_ARGUMENT;
    }
#endif
    BOOL result = TryEnterCriticalSection (&pMutex->cs);
    *pOutLocked = result ? true : false;
    return R_CSTL_OK;
}

int
r_cstl_mutex_unlock (struct r_cstl_mutex* pMutex)
{
#if R_CSTL_THREAD_DEBUG
    if (pMutex == NULL)
    {
        return R_CSTL_ERROR_INVALID_ARGUMENT;
    }
#endif
    LeaveCriticalSection (&pMutex->cs);
    return R_CSTL_OK;
}

#elif defined(R_CSTL_PLATFORM_LINUX)

struct r_cstl_mutex
{
        pthread_mutex_t mutex;
};

struct r_cstl_mutex*
r_cstl_new_mutex (void)
{
    struct r_cstl_mutex* pMutex = r_cstl_heap_alloc (sizeof (struct r_cstl_mutex));
#if R_CSTL_THREAD_DEBUG
    if (pMutex == NULL)
    {
        return NULL;
    }
    r_cstl_heap_register_allocation (
        pMutex,
        pMutex,
        sizeof (struct r_cstl_mutex),
        R_CSTL_HEAP_NAME (r_cstl_mutex));
#endif
    int result = pthread_mutex_init (&pMutex->mutex, NULL);
#if R_CSTL_THREAD_DEBUG
    if (result != 0)
    {
        r_cstl_heap_unregister_allocation (pMutex, pMutex);
        r_cstl_heap_free (pMutex);
        return NULL;
    }
#endif
    return pMutex;
}

void
r_cstl_mutex_destroy (struct r_cstl_mutex* pMutex)
{
    if (pMutex != NULL)
    {
#if R_CSTL_THREAD_DEBUG
        pthread_mutex_destroy (&pMutex->mutex);
        r_cstl_heap_unregister_allocation (pMutex, pMutex);
#endif
        r_cstl_heap_free (pMutex);
    }
}

int
r_cstl_mutex_lock (struct r_cstl_mutex* pMutex)
{
#if R_CSTL_THREAD_DEBUG
    if (pMutex == NULL)
    {
        return R_CSTL_ERROR_INVALID_ARGUMENT;
    }
#endif

    int result = pthread_mutex_lock (&pMutex->mutex);
#if R_CSTL_THREAD_DEBUG
    if (result != 0)
    {
        return R_CSTL_ERROR_MUTEX_LOCK_FAILED;
    }
#endif

    return R_CSTL_OK;
}

int
r_cstl_mutex_try_lock (struct r_cstl_mutex* pMutex, int* pOutLocked)
{
#if R_CSTL_THREAD_DEBUG
    if (pMutex == NULL || pOutLocked == NULL)
    {
        return R_CSTL_ERROR_INVALID_ARGUMENT;
    }
#endif

    int result = pthread_mutex_trylock (&pMutex->mutex);
    if (result == 0)
    {
        *pOutLocked = 1;
    }
    else if (result == EBUSY)
    {
        *pOutLocked = 0;
    }
#if R_CSTL_THREAD_DEBUG
    else
    {
        return R_CSTL_ERROR_MUTEX_LOCK_FAILED;
    }
#endif

    return R_CSTL_OK;
}

int
r_cstl_mutex_unlock (struct r_cstl_mutex* pMutex)
{
#if R_CSTL_THREAD_DEBUG
    if (pMutex == NULL)
    {
        return R_CSTL_ERROR_INVALID_ARGUMENT;
    }
#endif

    int result = pthread_mutex_unlock (&pMutex->mutex);
#if R_CSTL_THREAD_DEBUG
    if (result != 0)
    {
        return R_CSTL_ERROR_MUTEX_UNLOCK_FAILED;
    }
#endif

    return R_CSTL_OK;
}

#endif

#if defined(R_CSTL_PLATFORM_WINDOWS)

struct r_cstl_condition
{
        CONDITION_VARIABLE cv;
};

struct r_cstl_condition*
r_cstl_condition_create (void)
{
    struct r_cstl_condition* pCondition = r_cstl_heap_alloc (sizeof (struct r_cstl_condition));
#if R_CSTL_THREAD_DEBUG
    if (pCondition == NULL)
    {
        return NULL;
    }
    r_cstl_heap_register_allocation (
        pCondition,
        pCondition,
        sizeof (struct r_cstl_condition),
        R_CSTL_HEAP_NAME (r_cstl_condition));
#endif
    InitializeConditionVariable (&pCondition->cv);
    return pCondition;
}

void
r_cstl_condition_destroy (struct r_cstl_condition* pCondition)
{
    if (pCondition != NULL)
    {
#if R_CSTL_THREAD_DEBUG
        r_cstl_heap_unregister_allocation (pCondition, pCondition);
#endif
        r_cstl_heap_free (pCondition);
    }
}

int
r_cstl_condition_wait (struct r_cstl_condition* pCondition, struct r_cstl_mutex* pMutex)
{
#if R_CSTL_THREAD_DEBUG
    if (pCondition == NULL)
    {
        return R_CSTL_ERROR_INVALID_ARGUMENT;
    }

    if (pMutex == NULL)
    {
        return R_CSTL_ERROR_INVALID_ARGUMENT;
    }
#endif
    BOOL result = SleepConditionVariableCS (&pCondition->cv, &pMutex->cs, INFINITE);
#if R_CSTL_THREAD_DEBUG
    if (!result)
    {
        return R_CSTL_ERROR_CONDITION_WAIT_FAILED;
    }
#endif
    return R_CSTL_OK;
}

int
r_cstl_condition_signal (struct r_cstl_condition* pCondition)
{
#if R_CSTL_THREAD_DEBUG
    if (pCondition == NULL)
    {
        return R_CSTL_ERROR_INVALID_ARGUMENT;
    }
#endif
    WakeConditionVariable (&pCondition->cv);
    return R_CSTL_OK;
}

int
r_cstl_condition_broadcast (struct r_cstl_condition* pCondition)
{
#if R_CSTL_THREAD_DEBUG
    if (pCondition == NULL)
    {
        return R_CSTL_ERROR_INVALID_ARGUMENT;
    }
#endif
    WakeAllConditionVariable (&pCondition->cv);
    return R_CSTL_OK;
}

#elif defined(R_CSTL_PLATFORM_LINUX)

struct r_cstl_condition
{
        pthread_cond_t cond;
};

struct r_cstl_condition*
r_cstl_condition_create (void)
{
    struct r_cstl_condition* pCondition = r_cstl_heap_alloc (sizeof (struct r_cstl_condition));
#if R_CSTL_THREAD_DEBUG
    if (pCondition == NULL)
    {
        return NULL;
    }
    r_cstl_heap_register_allocation (
        pCondition,
        pCondition,
        sizeof (struct r_cstl_condition),
        R_CSTL_HEAP_NAME (r_cstl_condition));
#endif
    int result = pthread_cond_init (&pCondition->cond, NULL);
#if R_CSTL_THREAD_DEBUG
    if (result != 0)
    {
        r_cstl_heap_unregister_allocation (pCondition, pCondition);
        r_cstl_heap_free (pCondition);
        return NULL;
    }
#endif
    return pCondition;
}

void
r_cstl_condition_destroy (struct r_cstl_condition* pCondition)
{
    if (pCondition != NULL)
    {
#if R_CSTL_THREAD_DEBUG
        pthread_cond_destroy (&pCondition->cond);
        r_cstl_heap_unregister_allocation (pCondition, pCondition);
#endif
        r_cstl_heap_free (pCondition);
    }
}

int
r_cstl_condition_wait (struct r_cstl_condition* pCondition, struct r_cstl_mutex* pMutex)
{
#if R_CSTL_THREAD_DEBUG
    if (pCondition == NULL)
    {
        return R_CSTL_ERROR_INVALID_ARGUMENT;
    }

    if (pMutex == NULL)
    {
        return R_CSTL_ERROR_INVALID_ARGUMENT;
    }
#endif
    int result = pthread_cond_wait (&pCondition->cond, &pMutex->mutex);
#if R_CSTL_THREAD_DEBUG
    if (result != 0)
    {
        return R_CSTL_ERROR_CONDITION_WAIT_FAILED;
    }
#endif

    return R_CSTL_OK;
}

int
r_cstl_condition_signal (struct r_cstl_condition* pCondition)
{
#if R_CSTL_THREAD_DEBUG
    if (pCondition == NULL)
    {
        return R_CSTL_ERROR_INVALID_ARGUMENT;
    }
#endif

    int result = pthread_cond_signal (&pCondition->cond);
#if R_CSTL_THREAD_DEBUG
    if (result != 0)
    {
        return R_CSTL_ERROR_CONDITION_SIGNAL_FAILED;
    }
#endif
    return R_CSTL_OK;
}

int
r_cstl_condition_broadcast (struct r_cstl_condition* pCondition)
{
#if R_CSTL_THREAD_DEBUG
    if (pCondition == NULL)
    {
        return R_CSTL_ERROR_INVALID_ARGUMENT;
    }
#endif
    int result = pthread_cond_broadcast (&pCondition->cond);
#if R_CSTL_THREAD_DEBUG
    if (result != 0)
    {
        return R_CSTL_ERROR_CONDITION_SIGNAL_FAILED;
    }
#endif

    return R_CSTL_OK;
}

#endif
