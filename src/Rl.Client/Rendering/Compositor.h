#ifndef RL_CLIENT_RENDERING_COMPOSITOR_H
#define RL_CLIENT_RENDERING_COMPOSITOR_H

#include "Rl.Client/Rendering/ICompositor.h"

#include <memory>
#include <vector>
#include <map>
#include <mutex>
#include <set>
#include <atomic>

namespace rl
{

/**
 * @brief Concrete implementation of ICompositor
 *
 * Manages multiple render targets with priority-based ordering.
 * Thread-safe using mutex for concurrent access to render target collection.
 * Implements the Observer pattern to notify observers of changes.
 */
class Compositor : public ICompositor
{
  public:
    Compositor()           = default;
    ~Compositor() override = default;

    void                        addRenderTarget(IRenderTarget* target, int priority = 0) override;
    void                        removeRenderTarget(IRenderTarget* target) override;
    void                        removeRenderTarget(uint64_t id) override;
    IRenderTarget*              getRenderTarget(uint64_t id) const override;
    std::vector<IRenderTarget*> getAllRenderTargets() const override;
    void                        registerObserver(ICompositorObserver* observer) override;
    void                        unregisterObserver(ICompositorObserver* observer) override;
    void                        clear() override;
    size_t                      getRenderTargetCount() const override;
    bool                        hasRenderTargets() const override;

    /**
     * @brief Get render targets sorted by priority
     * @return Render targets in priority order
     */
    std::vector<IRenderTarget*> getRenderTargetsByPriority() const;

  private:
    void notifyRenderTargetAdded(IRenderTarget* target);
    void notifyRenderTargetRemoved(IRenderTarget* target);
    void notifyConfigurationChanged();

    mutable std::recursive_mutex                       mutex;
    std::map<uint64_t, std::pair<IRenderTarget*, int>> renderTargets;
    std::set<ICompositorObserver*>                     observers;
    mutable std::atomic<size_t>                        renderTargetCount;
};

} // namespace rl

#endif
