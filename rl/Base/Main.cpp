import Rl.Base.Game;

int main()
{
  Rl::Main::Game& game = Rl::Main::Game::GetInstance();
  game.Run();
  return 0;
}
