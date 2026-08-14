#ifndef RL_BASE_TIME_H
#define RL_BASE_TIME_H

#include <cstdint>

namespace rl
{

class IMainGame;
class MainGame;
class GameTime
{
                friend class MainGame;

        public:
                static float    getDeltaTime();
                static float    getTotalTime();
                static uint64_t getCurrentTime();
                static uint64_t getFrameCount();

        private:
                GameTime() = default;

                void            init();
                void            updateCallback();
                static float    deltaTime;
                static float    totalTime;
                static uint64_t frameCount;
                static uint64_t lastFrameTime;
};

} // namespace rl

#endif // RL_BASE_TIME_H
