#include "Rl.Client/Rendering/Compositor.h"
#include <algorithm>

namespace rl
{

void Compositor::addRenderTarget(IRenderTarget* target, int priority)
{
        if (!target)
        {
                return;
        }

        {
                std::scoped_lock lock(mutex);
                uint64_t         id    = target->getId();
                bool             isNew = renderTargets.find(id) == renderTargets.end();
                renderTargets[id]      = {target, priority};
                if (isNew)
                {
                        renderTargetCount.fetch_add(1, std::memory_order_relaxed);
                }
        }

        notifyRenderTargetAdded(target);
        notifyConfigurationChanged();
}

void Compositor::removeRenderTarget(IRenderTarget* target)
{
        if (!target)
        {
                return;
        }

        removeRenderTarget(target->getId());
}

void Compositor::removeRenderTarget(uint64_t id)
{
        IRenderTarget* target = nullptr;
        {
                std::scoped_lock lock(mutex);
                auto             it = renderTargets.find(id);
                if (it != renderTargets.end())
                {
                        target = it->second.first;
                        renderTargets.erase(it);
                        renderTargetCount.fetch_sub(1, std::memory_order_relaxed);
                }
        }
        if (target)
        {
                notifyRenderTargetRemoved(target);
                notifyConfigurationChanged();
        }
}

IRenderTarget* Compositor::getRenderTarget(uint64_t id) const
{
        std::scoped_lock lock(mutex);
        auto             it = renderTargets.find(id);
        if (it != renderTargets.end())
        {
                return it->second.first;
        }
        return nullptr;
}

std::vector<IRenderTarget*> Compositor::getAllRenderTargets() const
{
        std::scoped_lock            lock(mutex);
        std::vector<IRenderTarget*> result;
        result.reserve(renderTargets.size());
        for (const auto& pair : renderTargets)
        {
                result.emplace_back(pair.second.first);
        }
        return result;
}

std::vector<IRenderTarget*> Compositor::getRenderTargetsByPriority() const
{
        std::scoped_lock                            lock(mutex);
        std::vector<std::pair<int, IRenderTarget*>> sorted;
        sorted.reserve(renderTargets.size());

        for (const auto& pair : renderTargets)
        {
                sorted.emplace_back(pair.second.second, pair.second.first);
        }

        std::sort(sorted.begin(), sorted.end(),
                  [](const auto& a, const auto& b) { return a.first < b.first; });

        std::vector<IRenderTarget*> result;
        result.reserve(sorted.size());
        for (const auto& pair : sorted)
        {
                result.emplace_back(pair.second);
        }
        return result;
}

void Compositor::registerObserver(ICompositorObserver* observer)
{
        if (!observer)
        {
                return;
        }

        std::scoped_lock lock(mutex);
        observers.insert(observer);
}

void Compositor::unregisterObserver(ICompositorObserver* observer)
{
        if (!observer)
        {
                return;
        }

        std::scoped_lock lock(mutex);
        observers.erase(observer);
}

void Compositor::clear()
{
        std::vector<IRenderTarget*> targets;
        {
                std::scoped_lock lock(mutex);
                targets.reserve(renderTargets.size());
                for (auto& pair : renderTargets)
                {
                        targets.emplace_back(pair.second.first);
                }
                renderTargets.clear();
                renderTargetCount.store(0, std::memory_order_relaxed);
        }

        for (auto& target : targets)
        {
                notifyRenderTargetRemoved(target);
        }
        notifyConfigurationChanged();
}

size_t Compositor::getRenderTargetCount() const
{
        return renderTargetCount.load(std::memory_order_relaxed);
}

bool Compositor::hasRenderTargets() const
{
        return renderTargetCount.load(std::memory_order_relaxed) > 0;
}

void Compositor::notifyRenderTargetAdded(IRenderTarget* target)
{
        std::vector<ICompositorObserver*> observersCopy;
        {
                std::scoped_lock lock(mutex);
                observersCopy.reserve(observers.size());
                observersCopy.assign(observers.begin(), observers.end());
        }

        for (auto* observer : observersCopy)
        {
                observer->onRenderTargetAdded(target);
        }
}

void Compositor::notifyRenderTargetRemoved(IRenderTarget* target)
{
        std::vector<ICompositorObserver*> observersCopy;
        {
                std::scoped_lock lock(mutex);
                observersCopy.reserve(observers.size());
                observersCopy.assign(observers.begin(), observers.end());
        }

        for (auto* observer : observersCopy)
        {
                observer->onRenderTargetRemoved(target);
        }
}

void Compositor::notifyConfigurationChanged()
{
        std::vector<ICompositorObserver*> observersCopy;
        {
                std::scoped_lock lock(mutex);
                observersCopy.reserve(observers.size());
                observersCopy.assign(observers.begin(), observers.end());
        }

        for (auto* observer : observersCopy)
        {
                observer->onCompositorConfigurationChanged();
        }
}

} // namespace rl
