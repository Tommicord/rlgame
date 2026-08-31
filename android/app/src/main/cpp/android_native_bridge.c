#include "rlgame.base/main.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"
#include "rlgame.base/cstl/cstl_log.h"
#include "rlgame.base/cstl/cstl_string.h"

#include <android/log.h>
#include <android/native_activity.h>
#include <android/native_window.h>
#include <android/looper.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

static r_main_provider g_provider;
static ANativeWindow* g_window = NULL;
static pthread_t      g_gameThread = 0;
static bool           g_threadRunning = false;

static bool
AndroidGameLoop (void* pUserData, float deltaTime)
{
        return true;
}

static void*
GameThreadFunc (void* arg)
{
        while (g_provider.isRunning)
        {
                static struct timespec lastTime = {0};
                struct timespec        currentTime;
                clock_gettime (CLOCK_MONOTONIC, &currentTime);

                float deltaTime = 0.0f;
                if (lastTime.tv_sec != 0 || lastTime.tv_nsec != 0)
                {
                        deltaTime = (float)((currentTime.tv_sec - lastTime.tv_sec) * 1000000000LL
                                            + (currentTime.tv_nsec - lastTime.tv_nsec))
                                    / 1000000000.0f;
                }
                lastTime = currentTime;

                bool shouldContinue = g_provider.pGameLoop (g_provider.pUserData, deltaTime);
                if (!shouldContinue) g_provider.isRunning = false;
        }

        R_CSTL_LOG_INFO ("Android game thread stopped");
        return NULL;
}

JNIEXPORT void JNICALL
Java_net_rlgame_Main_nativeOnCreate (JNIEnv* env, jobject thiz)
{
        r_cstl_heap_init ();
        r_cstl_log_init ();

        R_CSTL_LOG_INFO ("Android native onCreate");

        g_provider.pGameLoop = AndroidGameLoop;
        g_provider.pUserData = NULL;
        g_provider.isRunning = false;
}

JNIEXPORT void JNICALL
Java_net_rlgame_Main_nativeOnResume (JNIEnv* env, jobject thiz)
{
        if (!g_threadRunning)
        {
                g_provider.isRunning = true;
                g_threadRunning = true;
                pthread_create (&g_gameThread, NULL, GameThreadFunc, NULL);
        }
}

JNIEXPORT void JNICALL
Java_net_rlgame_Main_nativeOnPause (JNIEnv* env, jobject thiz)
{
        g_provider.isRunning = false;
        if (g_gameThread != 0)
        {
                pthread_join (g_gameThread, NULL);
                g_gameThread = 0;
                g_threadRunning = false;
        }
}

JNIEXPORT void JNICALL
Java_net_rlgame_Main_nativeOnDestroy (JNIEnv* env, jobject thiz)
{
        g_provider.isRunning = false;
        if (g_gameThread != 0)
        {
                pthread_join (g_gameThread, NULL);
                g_gameThread = 0;
                g_threadRunning = false;
        }

        if (g_window)
        {
                ANativeWindow_release (g_window);
                g_window = NULL;
        }

        r_cstl_log_shutdown ();
        r_cstl_heap_shutdown ();
}

JNIEXPORT void JNICALL
Java_net_rlgame_Main_nativeOnSurfaceCreated (JNIEnv* env, jobject thiz)
{
        R_CSTL_LOG_INFO ("Android native surface created");
        // TODO: Initialize Vulkan surface here when g_window is available
}

JNIEXPORT void JNICALL
Java_net_rlgame_Main_nativeOnSurfaceChanged (JNIEnv* env, jobject thiz, jint width, jint height)
{
        R_CSTL_LOG_INFO ("Android native surface changed: %dx%d", width, height);
        // TODO: Handle Vulkan swapchain resize here
}

JNIEXPORT void JNICALL
Java_net_rlgame_Main_nativeOnSurfaceDestroyed (JNIEnv* env, jobject thiz)
{
        R_CSTL_LOG_INFO ("Android native surface destroyed");
        // TODO: Cleanup Vulkan resources here
}

// NativeActivity callbacks
void
ANativeActivity_onCreate (ANativeActivity* activity, void* savedState, size_t savedStateSize)
{
        // Store the activity reference for later use
        // This is called by the Android framework before onCreate
}
