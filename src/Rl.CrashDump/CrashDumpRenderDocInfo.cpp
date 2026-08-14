#include "Rl.CrashDump/CrashDumpRenderDocInfo.h"
#include "Rl.Log/Log.h"

#ifdef _RL_RENDERDOC_ENABLE
#include "Rl.Base/GameRenderDocGpuDebugguer.h"
#include <renderdoc_app.h>
#endif

#include <sstream>

namespace rl
{

std::string CrashDumpRenderDocInfo::collectRenderDocContext()
{
  std::ostringstream oss;

  oss << collectAvailabilityInfo();
  oss << collectAPIVersion();
  oss << collectCaptureState();
  oss << collectTriggerInfo();
  oss << collectActiveCaptures();

  return oss.str();
}

std::string CrashDumpRenderDocInfo::collectTriggerInfo()
{
  std::ostringstream oss;

#ifdef _RL_RENDERDOC_ENABLE
  GameRenderDocGpuDebugguer& provider = GameRenderDocGpuDebugguer::getInstance();
  void*                      api      = provider.getAPI();

  if (api && provider.isAvailable())
  {
    RENDERDOC_API_1_7_0* rdocApi = static_cast<RENDERDOC_API_1_7_0*>(api);
    oss << "Trigger Capture Available: " << (rdocApi->TriggerCapture ? "yes" : "no") << "\n";
    // Some APIs expose trigger configuration or hotkey info; attempt to read
    // if available.
    if (rdocApi->SetCaptureOptionU32)
    {
      oss << "Capture options can be modified at runtime\n";
    }
  }
  else
  {
    oss << "Trigger Capture: Not available (API not initialized)\n";
  }
#else
  oss << "Trigger Capture: Not compiled in\n";
#endif

  return oss.str();
}

std::string CrashDumpRenderDocInfo::collectAvailabilityInfo()
{
  std::ostringstream oss;

#ifdef _RL_RENDERDOC_ENABLE
  GameRenderDocGpuDebugguer& provider = GameRenderDocGpuDebugguer::getInstance();

  oss << "RenderDoc compiled-in: yes\n";
  oss << "RenderDoc available: " << (provider.isAvailable() ? "yes" : "no") << "\n";
  oss << "RenderDoc API pointer: " << provider.getAPI() << "\n";
#else
  oss << "RenderDoc compiled-in: no (disabled at compile time)\n";
  oss << "RenderDoc available: N/A\n";
  oss << "RenderDoc API pointer: nullptr\n";
#endif

  return oss.str();
}

std::string CrashDumpRenderDocInfo::collectAPIVersion()
{
  std::ostringstream oss;

#ifdef _RL_RENDERDOC_ENABLE
  GameRenderDocGpuDebugguer& provider = GameRenderDocGpuDebugguer::getInstance();
  void*                      api      = provider.getAPI();

  if (api && provider.isAvailable())
  {
    RENDERDOC_API_1_7_0* rdocApi = static_cast<RENDERDOC_API_1_7_0*>(api);

    oss << "RenderDoc API version: 1.7.0\n";
    oss << "RenderDoc API type: RENDERDOC_API_1_7_0\n";
  }
  else
  {
    oss << "RenderDoc API version: Not available (API not initialized)\n";
  }
#else
  oss << "RenderDoc API version: Not compiled in\n";
#endif

  return oss.str();
}

std::string CrashDumpRenderDocInfo::collectCaptureState()
{
  std::ostringstream oss;

#ifdef _RL_RENDERDOC_ENABLE
  GameRenderDocGpuDebugguer& provider = GameRenderDocGpuDebugguer::getInstance();
  void*                      api      = provider.getAPI();

  if (api && provider.isAvailable())
  {
    RENDERDOC_API_1_7_0* rdocApi = static_cast<RENDERDOC_API_1_7_0*>(api);

    if (rdocApi->IsFrameCapturing)
    {
      bool isCapturing = rdocApi->IsFrameCapturing();
      oss << "Frame Capture Status: " << (isCapturing ? "Active" : "Inactive") << "\n";
    }
    else
    {
      oss << "Frame Capture Status: Unknown (function not available)\n";
    }

    if (rdocApi->GetCaptureOptionU32)
    {
      uint32_t allowValidation = rdocApi->GetCaptureOptionU32(eRENDERDOC_Option_APIValidation);
      uint32_t captureCallstacks =
          rdocApi->GetCaptureOptionU32(eRENDERDOC_Option_CaptureCallstacks);
      uint32_t refAllResources = rdocApi->GetCaptureOptionU32(eRENDERDOC_Option_RefAllResources);

      oss << "Capture Options:\n";
      oss << "  Allow validation: " << (allowValidation ? "enabled" : "disabled") << "\n";
      oss << "  Capture callstacks: " << (captureCallstacks ? "enabled" : "disabled") << "\n";
      oss << "  Ref all resources: " << (refAllResources ? "enabled" : "disabled") << "\n";
    }

    if (rdocApi->GetCaptureFilePathTemplate)
    {
      const char* pathTemplate = rdocApi->GetCaptureFilePathTemplate();
      if (pathTemplate)
      {
        oss << "Capture file path template: " << pathTemplate << "\n";
      }
    }
  }
  else
  {
    oss << "Frame Capture Status: Not available (API not initialized)\n";
    oss << "Capture Options: Not available\n";
  }
#else
  oss << "Frame Capture Status: Not compiled in\n";
  oss << "Capture Options: Not compiled in\n";
#endif

  return oss.str();
}

std::string CrashDumpRenderDocInfo::collectActiveCaptures()
{
  std::ostringstream oss;

#ifdef _RL_RENDERDOC_ENABLE
  GameRenderDocGpuDebugguer& provider = GameRenderDocGpuDebugguer::getInstance();
  void*                      api      = provider.getAPI();

  if (api && provider.isAvailable())
  {
    RENDERDOC_API_1_7_0* rdocApi = static_cast<RENDERDOC_API_1_7_0*>(api);

    if (rdocApi->GetNumCaptures)
    {
      uint32_t numCaptures = rdocApi->GetNumCaptures();
      oss << "Number of captures: " << numCaptures << "\n";

      if (numCaptures > 0 && rdocApi->GetCapture)
      {
        for (uint32_t i = 0; i < numCaptures; i++)
        {
          char     capturePath[256]{};
          uint32_t pathLen;
          uint64_t timestamp;

          if (rdocApi->GetCapture(i, capturePath, &pathLen, &timestamp))
          {
            oss << "  Capture " << i << ": " << capturePath << "\n";
          }
        }
      }
    }
    else
    {
      oss << "Number of captures: Unknown (function not available)\n";
    }

    oss << "Target window handle: Not available in API 1.7.0\n";
  }
  else
  {
    oss << "Number of captures: Not available (API not initialized)\n";
  }
#else
  oss << "Number of captures: Not compiled in\n";
#endif

  return oss.str();
}

} // namespace rl
