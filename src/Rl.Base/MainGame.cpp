#include "Rl.Base/MainGame.h"
#include "Rl.Base/GameDevice.h"
#include "Rl.Base/GameTime.h"
#include "Rl.Log/Log.h"
#include "Rl.Player/Player.h"
#include "Rl.Player/PlayerController.h"

#if defined(_WIN32)
#include <windows.h>
#endif

namespace rl
{

void MainGame::onCreateCallback()
{
        time.init();
        device.init();
}

void MainGame::onResizeCallback()
{
        device.recreateSwapChain();
}

void MainGame::onLaunch()
{
        [this](MainGame* game)
        {
                bool isRunning = true;
#if defined(_WIN32)
                MainGameWin32Handle& handle    = game->handle;
                GameDeviceInstance&  device    = game->device;
                GameResources&       resources = device.getGameResources();
                while (isRunning)
                {
                        MSG* msg = &handle.msg;
                        while (PeekMessage(msg, NULL, 0, 0, PM_REMOVE))
                        {
                                if (msg->message == WM_QUIT)
                                {
                                        isRunning = false;
                                }
                                TranslateMessage(msg);
                                DispatchMessage(msg);
                        }
                        time.updateCallback();
                        resources.updateCallback();
                        device.updateCallback();
                }
#else
                // TODO: Implement the main loop for other platforms (Linux, Android, etc.)
#endif
                vkDeviceWaitIdle(device.gameDevice.device);
        }(this);
}

} // namespace rl
