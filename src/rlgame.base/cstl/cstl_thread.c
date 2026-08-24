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
#include <sys/syscall.h>
#else
#error "Unsupported platform for cstl_thread"
#endif

#if defined(R_CSTL_PLATFORM_WINDOWS)

struct R_CSTL_Thread
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
                        R_CSTL_ThreadFunc pFunc;
                        void*             pData;
        }* pParams = lpParam;

        R_CSTL_ThreadFunc pFunc = pParams->pFunc;
        void*             pData = pParams->pData;

        R_CSTL_HeapFree (pParams);

        pFunc (pData);
        return 0;
}

struct R_CSTL_Thread*
R_CSTL_NewThread (R_CSTL_ThreadFunc pFunc, void* pData)
{
#if R_CSTL_THREAD_DEBUG
        if (pFunc == NULL)
        {
                return NULL;
        }
#endif
        struct
        {
                        R_CSTL_ThreadFunc pFunc;
                        void*             pData;
        }* pParams = R_CSTL_HeapAlloc (sizeof (*pParams));

#if R_CSTL_THREAD_DEBUG
        if (pParams == NULL)
        {
                return NULL;
        }
#endif

        pParams->pFunc = pFunc;
        pParams->pData = pData;

        struct R_CSTL_Thread* pThread = R_CSTL_HeapAlloc (sizeof (struct R_CSTL_Thread));
#if R_CSTL_THREAD_DEBUG
        if (pThread == NULL)
        {
                R_CSTL_HeapFree (pParams);
                return NULL;
        }
        R_CSTL_HeapRegisterAllocation (
            pThread,
            pThread,
            sizeof (struct R_CSTL_Thread),
            R_CSTL_HEAP_NAME (R_CSTL_Thread));
#endif

        pThread->handle = CreateThread (NULL, 0, Win32ThreadEntry, pParams, 0, &pThread->threadId);

#if R_CSTL_THREAD_DEBUG
        if (pThread->handle == NULL)
        {
                R_CSTL_HeapUnregisterAllocation (pThread, pThread);
                R_CSTL_HeapFree (pParams);
                R_CSTL_HeapFree (pThread);
                return NULL;
        }
#endif

        return pThread;
}

int
R_CSTL_ThreadJoin (struct R_CSTL_Thread* pThread)
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
                R_CSTL_HeapUnregisterAllocation (pThread, pThread);
                R_CSTL_HeapFree (pThread);
                return R_CSTL_ERROR_THREAD_JOIN_FAILED;
        }
#endif

        CloseHandle (pThread->handle);
#if R_CSTL_THREAD_DEBUG
        R_CSTL_HeapUnregisterAllocation (pThread, pThread);
#endif
        R_CSTL_HeapFree (pThread);

        return R_CSTL_OK;
}

uint64_t
R_CSTL_ThreadGetCurrentId (void)
{
        return (uint64_t)GetCurrentThreadId ();
}

void
R_CSTL_ThreadYield (void)
{
        SwitchToThread ();
}

void
R_CSTL_ThreadSleep (uint32_t milliseconds)
{
        Sleep (milliseconds);
}

#elif defined(R_CSTL_PLATFORM_LINUX)

struct R_CSTL_Thread
{
                pthread_t thread;
                int       joined;
};

struct ThreadParams
{
                R_CSTL_ThreadFunc pFunc;
                void*             pData;
};

static void*
PthreadThreadEntry (void* arg)
{
        struct ThreadParams* pParams = arg;
        R_CSTL_ThreadFunc    pFunc = pParams->pFunc;
        void*                pData = pParams->pData;

        R_CSTL_HeapFree (pParams);

        pFunc (pData);
        return NULL;
}

struct R_CSTL_Thread*
R_CSTL_NewThread (R_CSTL_ThreadFunc pFunc, void* pData)
{
#if R_CSTL_THREAD_DEBUG
        if (pFunc == NULL)
        {
                return NULL;
        }
#endif

        struct ThreadParams* pParams = R_CSTL_HeapAlloc (sizeof (struct ThreadParams));
#if R_CSTL_THREAD_DEBUG
        if (pParams == NULL)
        {
                return NULL;
        }
#endif

        pParams->pFunc = pFunc;
        pParams->pData = pData;

        struct R_CSTL_Thread* pThread = R_CSTL_HeapAlloc (sizeof (struct R_CSTL_Thread));
#if R_CSTL_THREAD_DEBUG
        if (pThread == NULL)
        {
                R_CSTL_HeapFree (pParams);
                return NULL;
        }
        R_CSTL_HeapRegisterAllocation (
            pThread,
            pThread,
            sizeof (struct R_CSTL_Thread),
            R_CSTL_HEAP_NAME (R_CSTL_Thread));
#endif

        pThread->joined = 0;

        int result = pthread_create (&pThread->thread, NULL, PthreadThreadEntry, pParams);
#if R_CSTL_THREAD_DEBUG
        if (result != 0)
        {
                R_CSTL_HeapUnregisterAllocation (pThread, pThread);
                R_CSTL_HeapFree (pParams);
                R_CSTL_HeapFree (pThread);
                return NULL;
        }
#endif

        return pThread;
}

int
R_CSTL_ThreadJoin (struct R_CSTL_Thread* pThread)
{
#if R_CSTL_THREAD_DEBUG
        if (pThread == NULL)
        {
                return R_CSTL_ERROR_INVALID_ARGUMENT;
        }

        if (pThread->joined)
        {
#if R_CSTL_THREAD_DEBUG
                R_CSTL_HeapUnregisterAllocation (pThread, pThread);
#endif
                R_CSTL_HeapFree (pThread);
                return R_CSTL_ERROR_INVALID_ARGUMENT;
        }
#endif

        int result = pthread_join (pThread->thread, NULL);
#if R_CSTL_THREAD_DEBUG
        if (result != 0)
        {
                R_CSTL_HeapUnregisterAllocation (pThread, pThread);
                R_CSTL_HeapFree (pThread);
                return R_CSTL_ERROR_THREAD_JOIN_FAILED;
        }
#endif

#if R_CSTL_THREAD_DEBUG
        R_CSTL_HeapUnregisterAllocation (pThread, pThread);
#endif
        R_CSTL_HeapFree (pThread);
        return R_CSTL_OK;
}

uint64_t
R_CSTL_ThreadGetCurrentId (void)
{
        return (uint64_t)syscall (SYS_gettid);
}

void
R_CSTL_ThreadYield (void)
{
        sched_yield ();
}

void
R_CSTL_ThreadSleep (uint32_t milliseconds)
{
        usleep (milliseconds * 1000);
}

#endif

#if defined(R_CSTL_PLATFORM_WINDOWS)

struct R_CSTL_Mutex
{
                CRITICAL_SECTION cs;
};

struct R_CSTL_Mutex*
R_CSTL_NewMutex (void)
{
        struct R_CSTL_Mutex* pMutex = R_CSTL_HeapAlloc (sizeof (struct R_CSTL_Mutex));
#if R_CSTL_THREAD_DEBUG
        if (pMutex == NULL)
        {
                return NULL;
        }
        R_CSTL_HeapRegisterAllocation (
            pMutex,
            pMutex,
            sizeof (struct R_CSTL_Mutex),
            R_CSTL_HEAP_NAME (R_CSTL_Mutex));
#endif
        InitializeCriticalSection (&pMutex->cs);
        return pMutex;
}

void
R_CSTL_MutexDestroy (struct R_CSTL_Mutex* pMutex)
{
        if (pMutex != NULL)
        {
#if R_CSTL_THREAD_DEBUG
                DeleteCriticalSection (&pMutex->cs);
                R_CSTL_HeapUnregisterAllocation (pMutex, pMutex);
#endif
                R_CSTL_HeapFree (pMutex);
        }
}

int
R_CSTL_MutexLock (struct R_CSTL_Mutex* pMutex)
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
R_CSTL_MutexTryLock (struct R_CSTL_Mutex* pMutex, int* pOutLocked)
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
R_CSTL_MutexUnlock (struct R_CSTL_Mutex* pMutex)
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

struct R_CSTL_Mutex
{
                pthread_mutex_t mutex;
};

struct R_CSTL_Mutex*
R_CSTL_NewMutex (void)
{
        struct R_CSTL_Mutex* pMutex = R_CSTL_HeapAlloc (sizeof (struct R_CSTL_Mutex));
#if R_CSTL_THREAD_DEBUG
        if (pMutex == NULL)
        {
                return NULL;
        }
        R_CSTL_HeapRegisterAllocation (
            pMutex,
            pMutex,
            sizeof (struct R_CSTL_Mutex),
            R_CSTL_HEAP_NAME (R_CSTL_Mutex));
#endif
        int result = pthread_mutex_init (&pMutex->mutex, NULL);
#if R_CSTL_THREAD_DEBUG
        if (result != 0)
        {
                R_CSTL_HeapUnregisterAllocation (pMutex, pMutex);
                R_CSTL_HeapFree (pMutex);
                return NULL;
        }
#endif
        return pMutex;
}

void
R_CSTL_MutexDestroy (struct R_CSTL_Mutex* pMutex)
{
        if (pMutex != NULL)
        {
#if R_CSTL_THREAD_DEBUG
                pthread_mutex_destroy (&pMutex->mutex);
                R_CSTL_HeapUnregisterAllocation (pMutex, pMutex);
#endif
                R_CSTL_HeapFree (pMutex);
        }
}

int
R_CSTL_MutexLock (struct R_CSTL_Mutex* pMutex)
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
R_CSTL_MutexTryLock (struct R_CSTL_Mutex* pMutex, int* pOutLocked)
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
R_CSTL_MutexUnlock (struct R_CSTL_Mutex* pMutex)
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

struct R_CSTL_Condition
{
                CONDITION_VARIABLE cv;
};

struct R_CSTL_Condition*
R_CSTL_ConditionCreate (void)
{
        struct R_CSTL_Condition* pCondition = R_CSTL_HeapAlloc (sizeof (struct R_CSTL_Condition));
#if R_CSTL_THREAD_DEBUG
        if (pCondition == NULL)
        {
                return NULL;
        }
        R_CSTL_HeapRegisterAllocation (
            pCondition,
            pCondition,
            sizeof (struct R_CSTL_Condition),
            R_CSTL_HEAP_NAME (R_CSTL_Condition));
#endif
        InitializeConditionVariable (&pCondition->cv);
        return pCondition;
}

void
R_CSTL_ConditionDestroy (struct R_CSTL_Condition* pCondition)
{
        if (pCondition != NULL)
        {
#if R_CSTL_THREAD_DEBUG
                R_CSTL_HeapUnregisterAllocation (pCondition, pCondition);
#endif
                R_CSTL_HeapFree (pCondition);
        }
}

int
R_CSTL_ConditionWait (struct R_CSTL_Condition* pCondition, struct R_CSTL_Mutex* pMutex)
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
R_CSTL_ConditionSignal (struct R_CSTL_Condition* pCondition)
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
R_CSTL_ConditionBroadcast (struct R_CSTL_Condition* pCondition)
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

struct R_CSTL_Condition
{
                pthread_cond_t cond;
};

struct R_CSTL_Condition*
R_CSTL_ConditionCreate (void)
{
        struct R_CSTL_Condition* pCondition = R_CSTL_HeapAlloc (sizeof (struct R_CSTL_Condition));
#if R_CSTL_THREAD_DEBUG
        if (pCondition == NULL)
        {
                return NULL;
        }
        R_CSTL_HeapRegisterAllocation (
            pCondition,
            pCondition,
            sizeof (struct R_CSTL_Condition),
            R_CSTL_HEAP_NAME (R_CSTL_Condition));
#endif
        int result = pthread_cond_init (&pCondition->cond, NULL);
#if R_CSTL_THREAD_DEBUG
        if (result != 0)
        {
                R_CSTL_HeapUnregisterAllocation (pCondition, pCondition);
                R_CSTL_HeapFree (pCondition);
                return NULL;
        }
#endif
        return pCondition;
}

void
R_CSTL_ConditionDestroy (struct R_CSTL_Condition* pCondition)
{
        if (pCondition != NULL)
        {
#if R_CSTL_THREAD_DEBUG
                pthread_cond_destroy (&pCondition->cond);
                R_CSTL_HeapUnregisterAllocation (pCondition, pCondition);
#endif
                R_CSTL_HeapFree (pCondition);
        }
}

int
R_CSTL_ConditionWait (struct R_CSTL_Condition* pCondition, struct R_CSTL_Mutex* pMutex)
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
R_CSTL_ConditionSignal (struct R_CSTL_Condition* pCondition)
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
R_CSTL_ConditionBroadcast (struct R_CSTL_Condition* pCondition)
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
