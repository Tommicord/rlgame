#pragma once

#include <stdint.h>

#include "rlgame.base/cstl/cstl_thread.h"

struct R_Microbit_SpirvThreadExecutor;
typedef void (*R_Microbit_SpirvThreadExecutorTask) (void* pUserData);

struct R_Microbit_SpirvThreadExecutor*
     R_Microbit_NewSpirvThreadExecutor (uint32_t workerCount, uint32_t queueCapacity);
void R_Microbit_DeleteSpirvThreadExecutor (struct R_Microbit_SpirvThreadExecutor* pExecutor);
int  R_Microbit_SpirvThreadExecutorSubmit (
     struct R_Microbit_SpirvThreadExecutor* pExecutor,
     R_Microbit_SpirvThreadExecutorTask     task,
     void*                                  pUserData);
int      R_Microbit_SpirvThreadExecutorWait (struct R_Microbit_SpirvThreadExecutor* pExecutor);
uint32_t R_Microbit_SpirvThreadExecutorWorkerCount (const struct R_Microbit_SpirvThreadExecutor* pExecutor);
