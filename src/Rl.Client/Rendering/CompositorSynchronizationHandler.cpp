#include "Rl.Client/Rendering\CompositorSynchronizationHandler.h"
#include "Rl.Base/GameError.h"

namespace rl
{

CompositorSynchronizationHandler::CompositorSynchronizationHandler(VkDevice device,
                                                                   uint32_t maxFramesInFlight) :
    device(device), maxFramesInFlight(maxFramesInFlight)
{
  createFences();
  createSemaphores();
}

CompositorSynchronizationHandler::~CompositorSynchronizationHandler()
{
  std::scoped_lock lock(mutex);
  inFlightFences.clear();
  imageAvailableSemaphores.clear();
  renderFinishedSemaphores.clear();
}

void CompositorSynchronizationHandler::createFences()
{
  std::scoped_lock lock(mutex);
  inFlightFences.reserve(maxFramesInFlight);

  for (uint32_t i = 0; i < maxFramesInFlight; ++i)
  {
    GameVulkanFenceCreateInfo fenceInfo{};
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    GameVulkanFence fence(device, fenceInfo);
    inFlightFences.emplace_back(std::move(fence));
  }
}

void CompositorSynchronizationHandler::createSemaphores()
{
  std::scoped_lock lock(mutex);
  imageAvailableSemaphores.reserve(maxFramesInFlight);
  renderFinishedSemaphores.reserve(maxFramesInFlight);

  for (uint32_t i = 0; i < maxFramesInFlight; ++i)
  {
    GameVulkanSemaphoreCreateInfo imageAvailableInfo{};
    imageAvailableInfo.semaphoreType = VK_SEMAPHORE_TYPE_BINARY;

    GameVulkanSemaphore imageAvailable(device, imageAvailableInfo);
    imageAvailableSemaphores.emplace_back(std::move(imageAvailable));

    GameVulkanSemaphoreCreateInfo renderFinishedInfo{};
    renderFinishedInfo.semaphoreType = VK_SEMAPHORE_TYPE_BINARY;

    GameVulkanSemaphore renderFinished(device, renderFinishedInfo);
    renderFinishedSemaphores.emplace_back(std::move(renderFinished));
  }
}

GameVulkanFence& CompositorSynchronizationHandler::getFence(uint32_t frameIndex)
{
  std::scoped_lock lock(mutex);
  if (frameIndex >= inFlightFences.size())
  {
    GameError::exitWithError("CompositorSynchronizationHandler::getFence",
                             "Frame index out of bounds");
  }
  return inFlightFences[frameIndex];
}

GameVulkanSemaphore&
CompositorSynchronizationHandler::getImageAvailableSemaphore(uint32_t frameIndex)
{
  std::scoped_lock lock(mutex);
  if (frameIndex >= imageAvailableSemaphores.size())
  {
    GameError::exitWithError("CompositorSynchronizationHandler::getImageAvailableSemaphore",
                             "Frame index out of bounds");
  }
  return imageAvailableSemaphores[frameIndex];
}

GameVulkanSemaphore&
CompositorSynchronizationHandler::getRenderFinishedSemaphore(uint32_t frameIndex)
{
  std::scoped_lock lock(mutex);
  if (frameIndex >= renderFinishedSemaphores.size())
  {
    GameError::exitWithError("CompositorSynchronizationHandler::getRenderFinishedSemaphore",
                             "Frame index out of bounds");
  }
  return renderFinishedSemaphores[frameIndex];
}

void CompositorSynchronizationHandler::waitForFence(uint32_t frameIndex, uint64_t timeout)
{
  std::scoped_lock lock(mutex);
  if (frameIndex >= inFlightFences.size())
  {
    GameError::exitWithError("CompositorSynchronizationHandler::waitForFence",
                             "Frame index out of bounds");
  }

  GameVulkanFence& inFlightFence = inFlightFences[frameIndex];
  inFlightFence.wait();
}

void CompositorSynchronizationHandler::resetFence(uint32_t frameIndex)
{
  std::scoped_lock lock(mutex);
  if (frameIndex >= inFlightFences.size())
  {
    GameError::exitWithError("CompositorSynchronizationHandler::resetFence",
                             "Frame index out of bounds");
  }

  GameVulkanFence& inFlightFence = inFlightFences[frameIndex];
  inFlightFence.reset();
}

} // namespace rl
