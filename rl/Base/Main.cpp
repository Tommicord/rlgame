import Rl.Base.Game;
import Rl.RayLog.Macro;
import Rl.RayLog.Logger;

import <stdexcept>;

int main()
{
    Rl::Main::Game& game = Rl::Main::Game::GetInstance();
    try
    {
        game.Run();
    }
    catch (std::exception& e)
    {
        Rl::RayLog::LogFatal("Game", e.what());
        return 1;
    }
    return 0;
}
