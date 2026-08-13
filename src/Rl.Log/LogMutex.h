#ifndef RL_LOG_LOG_MUTEX_H
#define RL_LOG_LOG_MUTEX_H

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__ANDROID__)
#include <pthread.h>
#else
#include <mutex>
#endif

namespace rl
{

#ifdef _WIN32
/** Windows-specific mutex using CRITICAL_SECTION */
class LogMutex
{
        public:
                /** Constructs the mutex */
                LogMutex() noexcept
                {
                        InitializeCriticalSection(&cs);
                }
                /** Destroys the mutex */
                ~LogMutex() noexcept
                {
                        DeleteCriticalSection(&cs);
                }
                /** Locks the mutex */
                void lock() noexcept
                {
                        EnterCriticalSection(&cs);
                }
                /** Unlocks the mutex */
                void unlock() noexcept
                {
                        LeaveCriticalSection(&cs);
                }

        private:
                CRITICAL_SECTION cs; /**< Windows critical section */
};
#elif defined(__ANDROID__)
/** Android-specific mutex using pthread */
class LogMutex
{
        public:
                /** Constructs the mutex */
                LogMutex() noexcept
                {
                        pthread_mutex_init(&dumpMutex, nullptr);
                }
                /** Destroys the mutex */
                ~LogMutex() noexcept
                {
                        pthread_mutex_destroy(&dumpMutex);
                }
                /** Locks the mutex */
                void lock() noexcept
                {
                        pthread_mutex_lock(&dumpMutex);
                }
                /** Unlocks the mutex */
                void unlock() noexcept
                {
                        pthread_mutex_unlock(&dumpMutex);
                }

        private:
                pthread_mutex_t dumpMutex; /**< pthread mutex */
};
#else
/** Standard mutex using std::mutex */
class LogMutex
{
        public:
                /** Default constructs the mutex */
                LogMutex() = default;
                /** Locks the mutex */
                void lock() noexcept
                {
                        dumpMutex.lock();
                }
                /** Unlocks the mutex */
                void unlock() noexcept
                {
                        dumpMutex.unlock();
                }

        private:
                std::dumpMutex dumpMutex; /**< Standard mutex */
};
#endif

/** RAII lock for LogMutex */
class LogLock
{
        public:
                /** Constructs a lock and acquires the mutex
                 * @param m The mutex to lock */
                explicit LogLock(LogMutex& m) noexcept : dumpMutex(m)
                {
                        dumpMutex.lock();
                }
                /** Destroys the lock and releases the mutex */
                ~LogLock() noexcept
                {
                        dumpMutex.unlock();
                }
                LogLock(const LogLock&)            = delete; /**< Non-copyable */
                LogLock& operator=(const LogLock&) = delete; /**< Non-assignable */

        private:
                LogMutex& dumpMutex; /**< Reference to the mutex */
};

} // namespace rl

#endif // RL_LOG_LOG_MUTEX_H
