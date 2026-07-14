import Rl.World.ServiceUpdaterRegistry;
import Rl.World.ServiceUpdater;
import Rl.RayLog.Macro;

import <string>;
import <vector>;
import <memory>;
#include <ranges>

namespace Rl::World
{

void ServiceUpdaterRegistry::RegisterUpdater(const std::string&              name,
                                             std::shared_ptr<ServiceUpdater> updater)
{
    if (updater)
    {
        updaters[name] = std::move(updater);
        RayLog::LogInfo(RAYLOG_TAG, "Registered updater: %s", name.c_str());
    }
    else
    {
        RayLog::LogWarning(RAYLOG_TAG, "Attempted to register null updater for: %s",
                           name.c_str());
    }
}

void ServiceUpdaterRegistry::UnregisterUpdater(const std::string& name)
{
    if (updaters.erase(name) > 0)
    {
        RayLog::LogInfo(RAYLOG_TAG, "Unregistered updater: %s", name.c_str());
    }
    else
    {
        RayLog::LogWarning(RAYLOG_TAG, "Attempted to unregister non-existent updater: %s",
                           name.c_str());
    }
}

std::shared_ptr<ServiceUpdater>
ServiceUpdaterRegistry::GetUpdater(const std::string& name) const
{
    auto it = updaters.find(name);
    if (it != updaters.end())
    {
        RayLog::LogDebug(RAYLOG_TAG, "Retrieved updater: %s", name.c_str());
        return it->second;
    }

    RayLog::LogWarning(RAYLOG_TAG, "Updater not found: %s", name.c_str());
    return nullptr;
}

bool ServiceUpdaterRegistry::HasUpdater(const std::string& name) const
{
    return updaters.find(name) != updaters.end();
}

void ServiceUpdaterRegistry::UpdateAll()
{
    for (auto& [name, updater] : updaters)
    {
        if (updater)
        {
            updater->Update();
        }
        else
        {
            RayLog::LogWarning(RAYLOG_TAG, "Null updater detected for: %s", name.c_str());
        }
    }
}

size_t ServiceUpdaterRegistry::GetUpdaterCount() const
{
    return updaters.size();
}

void ServiceUpdaterRegistry::ClearAll()
{
    const size_t count = updaters.size();
    updaters.clear();
    RayLog::LogInfo(RAYLOG_TAG, "Cleared %zu updaters", count);
}

std::vector<std::string> ServiceUpdaterRegistry::GetUpdaterNames() const
{
    std::vector<std::string> names;
    names.reserve(updaters.size());

    for (const auto& name : updaters | std::views::keys)
    {
        names.push_back(name);
    }

    return names;
}

} // namespace Rl::World
