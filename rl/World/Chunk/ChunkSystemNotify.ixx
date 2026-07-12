export module Rl.World.Chunk.ChunkSystemNotify;

import Rl.RayLog.Logger;
import Rl.RayLog.Macro;
import Rl.World.Chunk.ChunkSystem;
import Rl.Base.UserInput;

import <memory>;
import <algorithm>;

namespace Rl::World::Chunk
{

export class ChunkSystemNotify final : public Input::IInputObserver
{
  protected:
  static constexpr auto RAYLOG_TAG = "ChunkSystemNotify";

  public:
  ChunkSystemNotify() : IInputObserver(*this), chunkSystem(nullptr)
  {
    RayLog::LogInfo(RAYLOG_TAG, "Initialized ChunkSystem event notifier");
  }

  explicit ChunkSystemNotify(ChunkSystem& system) : IInputObserver(*this), chunkSystem(&system)
  {
    RayLog::LogInfo(RAYLOG_TAG, "Initialized ChunkSystem event notifier for ChunkSystem");
  }

  ~ChunkSystemNotify() override = default;

  void OnKeyEvent(const Input::KeyEvent& event) override;
  void OnMouseScrollEvent(const Input::MouseScrollEvent& event) override
  {
  }
  void OnMouseButtonEvent(const Input::MouseButtonEvent& event) override
  {
  }
  void OnMouseMoveEvent(const Input::MouseMoveEvent& event) override
  {
  }

  private:
  ChunkSystem* chunkSystem;
};

} // namespace Rl::World::Chunk
