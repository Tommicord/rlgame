#ifndef RL_BASE_GAME_RDOC_PROVIDER_H
#define RL_BASE_GAME_RDOC_PROVIDER_H

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef _RL_RENDERDOC_ENABLE
#include <renderdoc_app.h>
#endif

namespace rl
{

class GameRenderDocGpuDebugguer
{
  public:
    /** Get the singleton instance
     * @return Reference to the singleton instance
     */
    static GameRenderDocGpuDebugguer& getInstance();

    // Delete copy constructor and assignment operator
    GameRenderDocGpuDebugguer(const GameRenderDocGpuDebugguer&)            = delete;
    GameRenderDocGpuDebugguer& operator=(const GameRenderDocGpuDebugguer&) = delete;

    ~GameRenderDocGpuDebugguer();

    bool isAvailable() const;
    void startCapture();
    void endCapture();
    void triggerCapture();

    /** Get RenderDoc API pointer for advanced operations
     * @return Pointer to RenderDoc API or nullptr if not available
     */
    void* getAPI() const;

  private:
    GameRenderDocGpuDebugguer();

#ifdef _RL_RENDERDOC_ENABLE
    RENDERDOC_API_1_4_1* api;
    HMODULE              module;
#endif
    bool available;
};

} // namespace rl

#endif // RL_BASE_MAIN_RDOC_PROVIDER_H
