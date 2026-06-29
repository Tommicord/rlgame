import Rl.Base.Game;

int main()
{
  Rl::Main::Game& game = Rl::Main::Game::GetInstance();
  try
  {
    game.Run();
  }
  catch (const std::exception& e)
  {
    return 1;
  }
  return 0;
}
