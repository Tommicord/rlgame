#include "Rl.Base/GameRenderDocGpuDebugguer.h"
#include "Rl.Log/Log.h"

namespace rl
{

GameRenderDocGpuDebugguer& GameRenderDocGpuDebugguer::getInstance()
{
  static GameRenderDocGpuDebugguer instance;
  return instance;
}

GameRenderDocGpuDebugguer::GameRenderDocGpuDebugguer() :

#ifdef _RL_RENDERDOC_ENABLE
    api(nullptr), module(nullptr),
#endif
    available(false)
{
#ifdef _RL_RENDERDOC_ENABLE
  module = LoadLibrary(TEXT("renderdoc.dll"));
  if (module)
  {
    pRENDERDOC_GetAPI RENDERDOC_GetAPI =
        (pRENDERDOC_GetAPI)GetProcAddress(module, "RENDERDOC_GetAPI");
    if (RENDERDOC_GetAPI)
    {
      int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_4_1, (void**)&api);
      if (ret == 1)
      {
        available = true;
        Log::info("RenderDoc API initialized successfully");
      }
      else
      {
        Log::warning("Failed to initialize RenderDoc API");
      }
    }
    else
    {
      Log::warning("Failed to get RENDERDOC_GetAPI function from renderdoc.dll");
    }
  }
  else
  {
    Log::info("RenderDoc not available; renderdoc.dll not found");
  }
#else
  Log::info("RenderDoc integration disabled, probably not compiled with _RL_RENDERDOC_ENABLE");
#endif
}

GameRenderDocGpuDebugguer::~GameRenderDocGpuDebugguer()
{
#ifdef _RL_RENDERDOC_ENABLE
  if (module)
  {
    FreeLibrary(module);
    module = nullptr;
  }
  api = nullptr;
#endif
}

bool GameRenderDocGpuDebugguer::isAvailable() const
{
  return available;
}

void GameRenderDocGpuDebugguer::startCapture()
{
#ifdef _RL_RENDERDOC_ENABLE
  if (available && api)
  {
    api->StartFrameCapture(nullptr, nullptr);
    Log::info("RenderDoc capture started");
  }
#endif
}

void GameRenderDocGpuDebugguer::endCapture()
{
#ifdef _RL_RENDERDOC_ENABLE
  if (available && api)
  {
    api->EndFrameCapture(nullptr, nullptr);
    Log::info("RenderDoc capture ended");
  }
#endif
}

void GameRenderDocGpuDebugguer::triggerCapture()
{
#ifdef _RL_RENDERDOC_ENABLE
  if (available && api)
  {
    api->TriggerCapture();
    Log::info("RenderDoc capture triggered");
  }
#endif
}

void* GameRenderDocGpuDebugguer::getAPI() const
{
#ifdef _RL_RENDERDOC_ENABLE
  return api;
#else
  return nullptr;
#endif
}

} // namespace rl
