import Rl.World.ServiceUpdaterRegistry;

namespace Rl::World
{

void ServiceUpdaterRegistry::RegisterUpdater(const std::string& name, std::shared_ptr<ServiceUpdater> updater)
{
  updaters[name] = std::move(updater);
}

void ServiceUpdaterRegistry::UnregisterUpdater(const std::string& name)
{
  updaters.erase(name);
}

std::shared_ptr<ServiceUpdater> ServiceUpdaterRegistry::GetUpdater(const std::string& name) const
{
  auto it = updaters.find(name);
  if (it != updaters.end())
    return it->second;
  
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
  }
}

size_t ServiceUpdaterRegistry::GetUpdaterCount() const
{
  return updaters.size();
}

void ServiceUpdaterRegistry::ClearAll()
{
  updaters.clear();
}

std::vector<std::string> ServiceUpdaterRegistry::GetUpdaterNames() const
{
  std::vector<std::string> names;
  names.reserve(updaters.size());
  
  for (const auto& [name, updater] : updaters)
  {
    names.push_back(name);
  }
  
  return names;
}

} // namespace Rl::World
