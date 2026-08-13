#include "Rl.Base/GameTime.h"

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <time.h>
#elif defined(__ANDROID__)
#include <time.h>
#endif

namespace rl
{

float    GameTime::deltaTime     = 0.0f;
float    GameTime::totalTime     = 0.0f;
uint64_t GameTime::frameCount    = 0;
uint64_t GameTime::lastFrameTime = 0;

void GameTime::init()
{
        lastFrameTime = getCurrentTime();
        deltaTime     = 0.0f;
        totalTime     = 0.0f;
        frameCount    = 0;
}

void GameTime::updateCallback()
{
        uint64_t currentTime = getCurrentTime();
        uint64_t frameTime   = currentTime - lastFrameTime;
        lastFrameTime        = currentTime;
        deltaTime            = static_cast<float>(frameTime) / 1000.0f;

        totalTime += deltaTime;
        frameCount++;
}

float GameTime::getDeltaTime()
{
        return deltaTime;
}

float GameTime::getTotalTime()
{
        return totalTime;
}

uint64_t GameTime::getFrameCount()
{
        return frameCount;
}

uint64_t GameTime::getCurrentTime()
{
#if defined(_WIN32)
        LARGE_INTEGER frequency;
        LARGE_INTEGER counter;
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&counter);
        return static_cast<uint64_t>((counter.QuadPart * 1000) / frequency.QuadPart);
#elif defined(__linux__) || defined(__ANDROID__)
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return static_cast<uint64_t>(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#else
        return 0;
#endif
}

} // namespace rl
