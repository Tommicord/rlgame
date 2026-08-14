#ifndef RL_CLIENT_RENDERING_COMPOSITOR_MANAGER_H
#define RL_CLIENT_RENDERING_COMPOSITOR_MANAGER_H

#include "Rl.Client/Rendering/ICompositor.h"
#include "Rl.Client/Rendering/Compositor.h"
#include "Rl.Client/Rendering/IRenderTarget.h"
#include "Rl.Client/Rendering/CompositorSynchronizationHandler.h"

#include "Rl.Base/IGameDrawable.h"
#include "Rl.Base/GameDevice.h"
#include "Rl.Base/GameVulkanBuffer.h"
#include "Rl.Base/GameVulkanSampler.h"

#include <memory>
#include <mutex>
#include <vulkan/vulkan.hpp>

namespace rl
{

/**
 * @brief Singleton manager for compositing operations
 *
 * Provides a centralized, thread-safe interface for managing compositing
 * operations. Integrates with the IGameDrawable system to participate
 * in the rendering pipeline. Handles the actual compositing by rendering
 * fullscreen quads with texture sampling.
 */
class CompositorManager : public IGameDrawable,
                          public ICompositorObserver
{
    static constexpr size_t defaultMaxFramesInFlight = 2;

  public:
    /**
     * @brief Get the singleton instance
     * @return Reference to the singleton instance
     */
    static CompositorManager& getInstance();

    /**
     * @brief Initialize the compositor manager
     * @param device Vulkan device
     * @param physicalDevice Vulkan physical device
     * @param maxFramesInFlight Maximum frames in flight
     */
    static void initialize(VkDevice         device,
                           VkPhysicalDevice physicalDevice,
                           uint32_t         maxFramesInFlight = defaultMaxFramesInFlight);
    ~CompositorManager() override = default;

    /**
     * @brief Shutdown the compositor manager
     */
    static void shutdown();

    /**
     * @brief Check if the compositor manager is initialized
     * @return true if initialized, false otherwise
     */
    static bool isInitialized();

    /**
     * @brief Get the underlying compositor
     * @return Reference to the compositor
     */
    Compositor& getCompositor();

    /**
     * @brief Add a render target to the compositor
     * @param target The render target to add
     * @param priority Priority for rendering order
     */
    void addRenderTarget(IRenderTarget* target, int priority = 0);

    /**
     * @brief Remove a render target from the compositor
     * @param target The render target to remove
     */
    void removeRenderTarget(IRenderTarget* target);

    /**
     * @brief Remove a render target by ID
     * @param id The ID of the render target to remove
     */
    void removeRenderTarget(uint64_t id);

    /**
     * @brief Get a render target by ID
     * @param id The ID of the render target
     * @return Pointer to the render target
     */
    IRenderTarget* getRenderTarget(uint64_t id) const;

    /**
     * @brief Get all render targets
     * @return All render targets
     */
    std::vector<IRenderTarget*> getAllRenderTargets() const;

    /**
     * @brief Clear all render targets
     */
    void clearRenderTargets();

    /**
     * @brief Get the synchronization manager
     * @return Reference to the synchronization manager
     */
    CompositorSynchronizationHandler& getSynchronizationManager();

    void setup(GameDeviceInstance& device) override;
    void draw(GameDeviceInstance& device) override;
    void destroy(GameDeviceInstance& device) override;

    void onRenderTargetAdded(IRenderTarget* target) override;
    void onRenderTargetRemoved(IRenderTarget* target) override;
    void onCompositorConfigurationChanged() override;

  private:
    CompositorManager();

    CompositorManager(const CompositorManager&)            = delete;
    CompositorManager& operator=(const CompositorManager&) = delete;
    CompositorManager(CompositorManager&&)                 = delete;
    CompositorManager& operator=(CompositorManager&&)      = delete;

    void createFullscreenQuadPipeline(GameDeviceInstance& device);
    void createDescriptorSets(GameDeviceInstance& device);
    void updateDescriptorSets();
    void renderFullscreenQuad(GameDeviceInstance& device, GameVulkanImageView& textureView);

    static std::atomic<bool> initializedFlag;

    VkDevice         device;
    VkPhysicalDevice physicalDevice;

    std::unique_ptr<Compositor>                       compositor;
    std::unique_ptr<CompositorSynchronizationHandler> syncManager;

    VkPipelineLayout             pipelineLayout   = VK_NULL_HANDLE;
    VkPipeline                   graphicsPipeline = VK_NULL_HANDLE;
    GameVulkanBuffer             vertexBuffer;
    GameVulkanSampler            sampler;
    VkDescriptorPool             descriptorPool      = VK_NULL_HANDLE;
    VkDescriptorSetLayout        descriptorSetLayout = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptorSets;

    bool               initialized = false;
    mutable std::mutex mutex;
};

} // namespace rl

#endif
