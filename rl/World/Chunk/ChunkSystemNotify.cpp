import Rl.World.Chunk.ChunkSystemNotify;

import Rl.RayLog.Logger;
import Rl.RayLog.Macro;
import Rl.Base.UserInput;
import Rl.World.Chunk.ChunkSystem;

namespace Rl::World::Chunk
{
void ChunkSystemNotify::OnKeyEvent(const Input::KeyEvent& event) {
  if (event.action == Input::Action::Press || event.action == Input::Action::Repeat)
  {
    switch (event.key)
    {
    case Input::Key::W:
    case Input::Key::S:
    case Input::Key::A:
    case Input::Key::D:
    case Input::Key::Space:
    case Input::Key::LeftShift:
      if (chunkSystem)
      {
        if (!chunkSystem->IsRunning())
        {
          RayLog::LogWarning(
            RAYLOG_TAG, "The chunk system isn't running");
          return;
        }
        chunkSystem->RefreshNow();
      }
    default:
      break;
    }
  }
}
}