#include "microbit/spirvrunner/microbit_spirv_threadexecutor.h"
#include "microbit/microbit_platform.h"

#include "rlgame.base/cstl/cstl_heap_allocator.h"

#include <string.h>

struct R_Microbit_SpirvThreadExecutorTask
{
        R_Microbit_SpirvThreadExecutorTask task;
        void*                              pUserData;
};

struct R_Microbit_SpirvThreadExecutor
{
        struct R_Microbit_SpirvThreadExecutorTask* pTasks;
        struct R_CSTL_Thread**                     ppWorkers;
        struct R_CSTL_Mutex*                       pMutex;
        struct R_CSTL_Condition*                   pTaskAvailable;
        struct R_CSTL_Condition*                   pTaskComplete;
        uint32_t                                   workerCount;
        uint32_t                                   queueCapacity;
        uint32_t                                   queueHead;
        uint32_t                                   queueCount;
        uint32_t                                   activeTasks;
        uint8_t                                    shuttingDown;
        uint8_t                                    failed;
};

static void
R_Microbit_SpirvThreadExecutorLock (struct R_Microbit_SpirvThreadExecutor* pExecutor)
{
    R_CSTL_MutexLock (pExecutor->pMutex);
}

static void
R_Microbit_SpirvThreadExecutorUnlock (struct R_Microbit_SpirvThreadExecutor* pExecutor)
{
    R_CSTL_MutexUnlock (pExecutor->pMutex);
}

static void
R_Microbit_SpirvThreadExecutorWakeWorkers (struct R_Microbit_SpirvThreadExecutor* pExecutor)
{
    R_CSTL_ConditionBroadcast (pExecutor->pTaskAvailable);
}

static void
R_Microbit_SpirvThreadExecutorWakeWaiters (struct R_Microbit_SpirvThreadExecutor* pExecutor)
{
    R_CSTL_ConditionBroadcast (pExecutor->pTaskComplete);
}

static void
R_Microbit_SpirvThreadExecutorWorker (void* pData)
{
    struct R_Microbit_SpirvThreadExecutor* pExecutor = (struct R_Microbit_SpirvThreadExecutor*)pData;
    for (;;)
    {
        R_Microbit_SpirvThreadExecutorLock (pExecutor);
        while (pExecutor->queueCount == 0u && !pExecutor->shuttingDown)
            R_CSTL_ConditionWait (pExecutor->pTaskAvailable, pExecutor->pMutex);

        if (pExecutor->queueCount == 0u && pExecutor->shuttingDown)
        {
            R_Microbit_SpirvThreadExecutorUnlock (pExecutor);
            return;
        }

        uint32_t                                  index = pExecutor->queueHead;
        struct R_Microbit_SpirvThreadExecutorTask task = pExecutor->pTasks[index];
        pExecutor->queueHead = (index + 1u) % pExecutor->queueCapacity;
        --pExecutor->queueCount;
        ++pExecutor->activeTasks;
        R_Microbit_SpirvThreadExecutorUnlock (pExecutor);

        task.task (task.pUserData);

        R_Microbit_SpirvThreadExecutorLock (pExecutor);
        --pExecutor->activeTasks;
        if (pExecutor->queueCount == 0u && pExecutor->activeTasks == 0u)
            R_Microbit_SpirvThreadExecutorWakeWaiters (pExecutor);
        R_Microbit_SpirvThreadExecutorUnlock (pExecutor);
    }
}

struct R_Microbit_SpirvThreadExecutor*
R_Microbit_NewSpirvThreadExecutor (uint32_t workerCount, uint32_t queueCapacity)
{
    if (workerCount == 0u) workerCount = 1u;
    if (queueCapacity == 0u) queueCapacity = workerCount * 8u;
    if (queueCapacity < workerCount) queueCapacity = workerCount;
    struct R_Microbit_SpirvThreadExecutor* pExecutor
        = (struct R_Microbit_SpirvThreadExecutor*)R_CSTL_HeapAlloc (sizeof (*pExecutor));
    if (!pExecutor) return NULL;
    memset (pExecutor, 0, sizeof (*pExecutor));

    pExecutor->workerCount = workerCount;
    pExecutor->queueCapacity = queueCapacity;
    pExecutor->pTasks = (struct R_Microbit_SpirvThreadExecutorTask*)R_CSTL_HeapAlloc (
        (size_t)queueCapacity * sizeof (*pExecutor->pTasks));
    pExecutor->ppWorkers
        = (struct R_CSTL_Thread**)R_CSTL_HeapAlloc ((size_t)workerCount * sizeof (*pExecutor->ppWorkers));
    pExecutor->pMutex = R_CSTL_NewMutex ();
    pExecutor->pTaskAvailable = R_CSTL_ConditionCreate ();
    pExecutor->pTaskComplete = R_CSTL_ConditionCreate ();

    if (!pExecutor->pTasks || !pExecutor->ppWorkers || !pExecutor->pMutex || !pExecutor->pTaskAvailable
        || !pExecutor->pTaskComplete)
    {
        R_CSTL_ConditionDestroy (pExecutor->pTaskAvailable);
        R_CSTL_ConditionDestroy (pExecutor->pTaskComplete);
        R_CSTL_MutexDestroy (pExecutor->pMutex);
        R_CSTL_HeapFree (pExecutor->pTasks);
        R_CSTL_HeapFree (pExecutor->ppWorkers);
        R_CSTL_HeapFree (pExecutor);
        return NULL;
    }

    for (uint32_t i = 0; i < workerCount; ++i)
    {
        pExecutor->ppWorkers[i] = R_CSTL_NewThread (R_Microbit_SpirvThreadExecutorWorker, pExecutor);
        if (!pExecutor->ppWorkers[i])
        {
            pExecutor->failed = 1u;
            pExecutor->workerCount = i;
            pExecutor->shuttingDown = 1u;
            R_Microbit_SpirvThreadExecutorWakeWorkers (pExecutor);
            for (uint32_t j = 0; j < i; ++j)
                R_CSTL_ThreadJoin (pExecutor->ppWorkers[j]);
            R_CSTL_ConditionDestroy (pExecutor->pTaskAvailable);
            R_CSTL_ConditionDestroy (pExecutor->pTaskComplete);
            R_CSTL_MutexDestroy (pExecutor->pMutex);
            R_CSTL_HeapFree (pExecutor->pTasks);
            R_CSTL_HeapFree (pExecutor->ppWorkers);
            R_CSTL_HeapFree (pExecutor);
            return NULL;
        }
    }

    return pExecutor;
}

int
R_Microbit_SpirvThreadExecutorSubmit (
    struct R_Microbit_SpirvThreadExecutor* pExecutor,
    R_Microbit_SpirvThreadExecutorTask     task,
    void*                                  pUserData)
{
    R_MICROBIT_ASSERT(pExecutor);
    R_Microbit_SpirvThreadExecutorLock (pExecutor);
    if (pExecutor->shuttingDown || pExecutor->failed)
    {
        R_Microbit_SpirvThreadExecutorUnlock (pExecutor);
        return -2;
    }
    if (pExecutor->queueCount == pExecutor->queueCapacity)
    {
        R_Microbit_SpirvThreadExecutorUnlock (pExecutor);
        return -3;
    }
    uint32_t index = (pExecutor->queueHead + pExecutor->queueCount) % pExecutor->queueCapacity;
    pExecutor->pTasks[index].task = task;
    pExecutor->pTasks[index].pUserData = pUserData;
    ++pExecutor->queueCount;
    R_CSTL_ConditionSignal (pExecutor->pTaskAvailable);
    R_Microbit_SpirvThreadExecutorUnlock (pExecutor);
    return 0;
}

int
R_Microbit_SpirvThreadExecutorWait (struct R_Microbit_SpirvThreadExecutor* pExecutor)
{
    R_MICROBIT_ASSERT(pExecutor);
    R_Microbit_SpirvThreadExecutorLock (pExecutor);
    while (pExecutor->queueCount != 0u || pExecutor->activeTasks != 0u)
        R_CSTL_ConditionWait (pExecutor->pTaskComplete, pExecutor->pMutex);
    int result = pExecutor->failed ? -2 : 0;
    R_Microbit_SpirvThreadExecutorUnlock (pExecutor);
    return result;
}

void
R_Microbit_DeleteSpirvThreadExecutor (struct R_Microbit_SpirvThreadExecutor* pExecutor)
{
    R_MICROBIT_ASSERT(pExecutor);
    R_Microbit_SpirvThreadExecutorWait (pExecutor);
    R_Microbit_SpirvThreadExecutorLock (pExecutor);
    pExecutor->shuttingDown = 1u;
    R_Microbit_SpirvThreadExecutorWakeWorkers (pExecutor);
    R_Microbit_SpirvThreadExecutorUnlock (pExecutor);
    for (uint32_t i = 0; i < pExecutor->workerCount; ++i)
        R_CSTL_ThreadJoin (pExecutor->ppWorkers[i]);
    R_CSTL_ConditionDestroy (pExecutor->pTaskAvailable);
    R_CSTL_ConditionDestroy (pExecutor->pTaskComplete);
    R_CSTL_MutexDestroy (pExecutor->pMutex);
    R_CSTL_HeapFree (pExecutor->pTasks);
    R_CSTL_HeapFree (pExecutor->ppWorkers);
    R_CSTL_HeapFree (pExecutor);
}

uint32_t
R_Microbit_SpirvThreadExecutorWorkerCount (const struct R_Microbit_SpirvThreadExecutor* pExecutor)
{
    R_MICROBIT_ASSERT (pExecutor);
    return pExecutor->workerCount;
}
