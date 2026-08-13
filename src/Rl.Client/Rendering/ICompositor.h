#ifndef RL_CLIENT_RENDERING_I_COMPOSITOR_H
#define RL_CLIENT_RENDERING_I_COMPOSITOR_H

#include "Rl.Client/Rendering/IRenderTarget.h"
#include <vulkan/vulkan.hpp>
#include <memory>
#include <vector>

namespace rl
{

/**
 * @brief Observer interface for compositor events
 *
 * This interface allows objects to observe compositor state changes
 * such as when render targets are added or removed.
 */
class ICompositorObserver
{
        public:
                virtual ~ICompositorObserver() = default;

                /**
                 * @brief Called when a render target is added to the compositor
                 * @param target The render target that was added
                 */
                virtual void onRenderTargetAdded(IRenderTarget* target) = 0;

                /**
                 * @brief Called when a render target is removed from the compositor
                 * @param target The render target that was removed
                 */
                virtual void onRenderTargetRemoved(IRenderTarget* target) = 0;

                /**
                 * @brief Called when the compositor's configuration changes
                 */
                virtual void onCompositorConfigurationChanged() = 0;
};

/**
 * @brief Interface for compositor implementations
 *
 * This interface defines the contract for compositors that combine
 * multiple render targets into a final output. Uses the Observer pattern
 * for event notification.
 */
class ICompositor
{
        public:
                virtual ~ICompositor() = default;

                /**
                 * @brief Add a render target to the compositor
                 * @param target The render target to add
                 * @param priority Priority for rendering order (higher = later)
                 */
                virtual void addRenderTarget(IRenderTarget* target, int priority = 0) = 0;

                /**
                 * @brief Remove a render target from the compositor
                 * @param target The render target to remove
                 */
                virtual void removeRenderTarget(IRenderTarget* target) = 0;

                /**
                 * @brief Remove a render target by ID
                 * @param id The ID of the render target to remove
                 */
                virtual void removeRenderTarget(uint64_t id) = 0;

                /**
                 * @brief Get a render target by ID
                 * @param id The ID of the render target
                 * @return Pointer to the render target, or nullptr if not found
                 */
                virtual IRenderTarget* getRenderTarget(uint64_t id) const = 0;

                /**
                 * @brief Get all render targets
                 * @return All render targets
                 */
                virtual std::vector<IRenderTarget*> getAllRenderTargets() const = 0;

                /**
                 * @brief Register an observer for compositor events
                 * @param observer The observer to register
                 */
                virtual void registerObserver(ICompositorObserver* observer) = 0;

                /**
                 * @brief Unregister an observer
                 * @param observer The observer to unregister
                 */
                virtual void unregisterObserver(ICompositorObserver* observer) = 0;

                /**
                 * @brief Clear all render targets
                 */
                virtual void clear() = 0;

                /**
                 * @brief Get the number of render targets
                 * @return Number of render targets
                 */
                virtual size_t getRenderTargetCount() const = 0;

                /**
                 * @brief Check if the compositor has any render targets
                 * @return true if has render targets, false otherwise
                 */
                virtual bool hasRenderTargets() const = 0;
};

} // namespace rl

#endif
