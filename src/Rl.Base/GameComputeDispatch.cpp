#include "Rl.Base/GameComputeDispatch.h"
#include "Rl.Base/IGameComputeDispatch.h"
#include "Rl.Base/GameVulkanQueueSubmitter.h"
#include "Rl.Base/GameVulkanFence.h"
#include "Rl.Base/GameError.h"

#include <stdexcept>
#include <algorithm>
#include <utility>

namespace rl
{

GameComputeDispatch::GameComputeDispatch(VkDevice      device,
                                         VkQueue       queue,
                                         VkCommandPool commandPool) :
    device(device), queue(queue), commandPool(device, commandPool, false),
    fence(device, GameVulkanFenceCreateInfo{0})
{
}

GameComputeDispatch::GameComputeDispatch(VkDevice device,
                                         VkQueue  queue,
                                         uint32_t queueFamilyIndex) :
    device(device), queue(queue), commandPool(device, queueFamilyIndex),
    fence(device, GameVulkanFenceCreateInfo{0})
{
}

GameComputeDispatch::~GameComputeDispatch()
{
  if (device != VK_NULL_HANDLE)
  {
    waitForCompletion();
  }
}

void GameComputeDispatch::dispatchSingle(IGameComputeDispatch& computeDispatch,
                                         void*                 pResource,
                                         GameVulkanSemaphore&  waitSemaphore)
{
  synchronizeWithPreviousCompletion(waitSemaphore);

  std::scoped_lock lock(computeDispatch.getGenerateMutex());
  computeDispatch.dispatch(pResource, waitSemaphore, computeDispatch.getCompletionFence());

  std::scoped_lock stateLock(stateMutex);
  lastCompletionSemaphore.setSemaphoreNonOwning(
      computeDispatch.getCompletionSemaphore().getSemaphore());
}

void GameComputeDispatch::dispatchChained(void*                pComputeDispatches,
                                          size_t               computeDispatchCount,
                                          void*                pComputeResources,
                                          size_t               computeResourceCount,
                                          GameVulkanSemaphore& initialWaitSemaphore)
{
  if (computeDispatchCount == 0)
  {
    std::scoped_lock lock(stateMutex);
    lastCompletionSemaphore.setSemaphoreNonOwning(initialWaitSemaphore.getSemaphore());
    return;
  }
  if (pComputeDispatches == nullptr)
  {
    GameError::exitWithError("pComputeDispatches cannot be null");
  }

  synchronizeWithPreviousCompletion(initialWaitSemaphore);

  std::vector<IGameComputeDispatch*> computeDispatches;
  computeDispatches.reserve(computeDispatchCount);

  IGameComputeDispatch** pComputeHeapBase =
      reinterpret_cast<IGameComputeDispatch**>(pComputeDispatches);

  for (size_t i = 0; i < computeDispatchCount; ++i)
  {
    IGameComputeDispatch* dispatch = pComputeHeapBase[i];
    if (dispatch == nullptr)
    {
      GameError::exitWithError("computeDispatch at index " + std::to_string(i) + " cannot be null");
    }
    computeDispatches.emplace_back(dispatch);
  }

  std::vector<void*> computeResources;
  computeResources.reserve(computeDispatchCount);

  if (pComputeResources != nullptr)
  {
    void** pResourceHeapBase = reinterpret_cast<void**>(pComputeResources);
    for (size_t i = 0; i < computeDispatchCount; ++i)
    {
      if (i < computeResourceCount)
      {
        computeResources.emplace_back(pResourceHeapBase[i]);
      }
      else
      {
        computeResources.emplace_back(nullptr);
      }
    }
  }
  else
  {
    for (size_t i = 0; i < computeDispatchCount; ++i)
    {
      computeResources.emplace_back(nullptr);
    }
  }

  std::vector<GameVulkanSemaphore> chainSemaphores;
  if (computeDispatchCount > 1)
  {
    chainSemaphores.reserve(computeDispatchCount - 1);
    for (size_t i = 0; i < computeDispatchCount - 1; ++i)
    {
      chainSemaphores.emplace_back(createBinarySemaphore());
    }
    std::scoped_lock lock(stateMutex);
    ownedSemaphores.insert(ownedSemaphores.end(), std::make_move_iterator(chainSemaphores.begin()),
                           std::make_move_iterator(chainSemaphores.end()));
  }

  GameVulkanSemaphore currentWaitSemaphore{};
  currentWaitSemaphore.setSemaphoreNonOwning(initialWaitSemaphore.getSemaphore());

  for (size_t i = 0; i < computeDispatchCount; ++i)
  {
    void*                 pResource        = computeResources[i];
    IGameComputeDispatch* pComputeDispatch = computeDispatches[i];

    std::scoped_lock lock(pComputeDispatch->getGenerateMutex());
    pComputeDispatch->dispatch(pResource, currentWaitSemaphore,
                               pComputeDispatch->getCompletionFence());
    GameVulkanFence& completionFence = pComputeDispatch->getCompletionFence();
    completionFence.wait();
    completionFence.reset();

    // Wait on the completion semaphore to ensure it's unsignaled before next use
    // Binary semaphores must be waited on before they can be signaled again
    VkSemaphore completionSem = pComputeDispatch->getCompletionSemaphore().getSemaphore();
    if (completionSem != VK_NULL_HANDLE)
    {
      VkSubmitInfo waitSubmit{};
      waitSubmit.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      waitSubmit.waitSemaphoreCount   = 1;
      waitSubmit.pWaitSemaphores      = &completionSem;
      VkPipelineStageFlags waitStage  = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
      waitSubmit.pWaitDstStageMask    = &waitStage;
      waitSubmit.commandBufferCount   = 0;
      waitSubmit.signalSemaphoreCount = 0;

      GameVulkanFence tempFence(device, GameVulkanFenceCreateInfo{0});
      GameVulkanQueueSubmitter::submit(queue, &waitSubmit, tempFence.getFence());
      tempFence.wait();
    }

    if (i < computeDispatchCount - 1)
    {
      std::scoped_lock stateLock(stateMutex);
      currentWaitSemaphore.setSemaphoreNonOwning(
          ownedSemaphores[ownedSemaphores.size() - (computeDispatchCount - 1 - i)].getSemaphore());
    }
    else
    {
      std::scoped_lock stateLock(stateMutex);
      lastCompletionSemaphore.setSemaphoreNonOwning(
          pComputeDispatch->getCompletionSemaphore().getSemaphore());
    }
  }
}

const GameVulkanSemaphore& GameComputeDispatch::getLastCompletionSemaphore() const
{
  return lastCompletionSemaphore;
}

GameVulkanSemaphore& GameComputeDispatch::getLastCompletionSemaphore()
{
  return lastCompletionSemaphore;
}

bool GameComputeDispatch::waitForCompletion(uint64_t timeout)
{
  std::scoped_lock lock(fenceMutex);
  if (fenceInUse.load())
  {
    fence.wait(timeout);
    safeResetFence();
    return true;
  }
  return true;
}

void GameComputeDispatch::reset()
{
  std::scoped_lock lock(stateMutex);
  lastCompletionSemaphore.setSemaphoreNonOwning(VK_NULL_HANDLE);
  ownedSemaphores.clear();
}

void GameComputeDispatch::synchronizeWithPreviousCompletion(
    const GameVulkanSemaphore& newWaitSemaphore)
{
  VkSemaphore previousSemaphore = VK_NULL_HANDLE;
  {
    std::scoped_lock lock(stateMutex);
    previousSemaphore = lastCompletionSemaphore.getSemaphore();
  }

  if (previousSemaphore != VK_NULL_HANDLE && previousSemaphore != newWaitSemaphore.getSemaphore())
  {
    GameVulkanFence localFence(device, GameVulkanFenceCreateInfo{0});

    submitWait(lastCompletionSemaphore, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, localFence);

    localFence.wait();
    localFence.reset();
  }
}

GameVulkanSemaphore GameComputeDispatch::createBinarySemaphore()
{
  GameVulkanSemaphoreCreateInfo createInfo{};
  createInfo.semaphoreType = VK_SEMAPHORE_TYPE_BINARY;
  return GameVulkanSemaphore(device, createInfo);
}

void GameComputeDispatch::safeResetFence()
{
  if (!isFenceSignaled())
  {
    fence.wait();
  }
  fence.reset();
  fenceInUse.store(false);
}

bool GameComputeDispatch::isFenceSignaled() const
{
  if (fence.getFence() == VK_NULL_HANDLE)
  {
    return true;
  }

  VkResult result = vkGetFenceStatus(device, fence.getFence());
  return result == VK_SUCCESS;
}

void GameComputeDispatch::submitWait(const GameVulkanSemaphore& waitSemaphore,
                                     VkPipelineStageFlags       waitStageMask,
                                     GameVulkanFence&           fence)
{
  VkSemaphore          waitSemaphores[] = {waitSemaphore.getSemaphore()};
  VkPipelineStageFlags waitStages[]     = {waitStageMask};

  VkSubmitInfo submitInfo{};
  submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.waitSemaphoreCount   = 1;
  submitInfo.pWaitSemaphores      = waitSemaphores;
  submitInfo.pWaitDstStageMask    = waitStages;
  submitInfo.commandBufferCount   = 0;
  submitInfo.signalSemaphoreCount = 0;

  GameVulkanQueueSubmitter::submit(queue, &submitInfo, fence.getFence());
}

} // namespace rl
