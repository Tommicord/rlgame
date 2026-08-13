#ifndef RL_BASE_MAIN_GAME_INPUT_H
#define RL_BASE_MAIN_GAME_INPUT_H

#include <array>
#include <cstdint>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace rl
{

struct GameInputEvent
{
                enum class Type
                {
                        None,
                        KeyPress,
                        KeyRelease,
                        KeyRepeat,
                        MouseMove,
                        MouseButtonDown,
                        MouseButtonUp,
                        MouseScroll
                };

                enum class MouseButton
                {
                        None,
                        Left,
                        Right,
                        Middle,
                        X1,
                        X2
                };

                enum class KeyCode
                {
                        Unknown,
                        Escape,
                        Key0,
                        Key1,
                        Key2,
                        Key3,
                        Key4,
                        Key5,
                        Key6,
                        Key7,
                        Key8,
                        Key9,
                        A,
                        B,
                        C,
                        D,
                        E,
                        F,
                        G,
                        H,
                        I,
                        J,
                        K,
                        L,
                        M,
                        N,
                        O,
                        P,
                        Q,
                        R,
                        S,
                        T,
                        U,
                        V,
                        W,
                        X,
                        Y,
                        Z,
                        Tab,
                        LeftShift,
                        RightShift,
                        LeftControl,
                        RightControl,
                        LeftAlt,
                        RightAlt,
                        Space,
                        Enter,
                        Backspace,
                        Insert,
                        Delete,
                        Home,
                        End,
                        PageUp,
                        PageDown,
                        Up,
                        Down,
                        Left,
                        Right,
                        F1,
                        F2,
                        F3,
                        F4,
                        F5,
                        F6,
                        F7,
                        F8,
                        F9,
                        F10,
                        F11,
                        F12
                };

                Type        type         = Type::None;
                KeyCode     keyCode      = KeyCode::Unknown;
                MouseButton button       = MouseButton::None;
                float       mouseX       = 0.0f;
                float       mouseY       = 0.0f;
                float       scrollDeltaX = 0.0f;
                float       scrollDeltaY = 0.0f;
                bool        ctrlPressed  = false;
                bool        shiftPressed = false;
                bool        altPressed   = false;
};

class IGameInputReceiver
{
        public:
                virtual ~IGameInputReceiver() = default;
                virtual void onKeyPress(GameInputEvent::KeyCode key, bool ctrl, bool shift, bool alt)
                {
                }
                virtual void onKeyRelease(GameInputEvent::KeyCode key, bool ctrl, bool shift, bool alt)
                {
                }
                virtual void onKeyRepeat(GameInputEvent::KeyCode key, bool ctrl, bool shift, bool alt)
                {
                }
                virtual void onMouseMove(float x, float y)
                {
                }
                virtual void onMouseButtonDown(GameInputEvent::MouseButton button, float x, float y)
                {
                }
                virtual void onMouseButtonUp(GameInputEvent::MouseButton button, float x, float y)
                {
                }
                virtual void onMouseScroll(float deltaX, float deltaY, float x, float y)
                {
                }
};

template <size_t N> 
class GameInput
{
        public:
                friend class MainGame;

                static void registerReceiver(IGameInputReceiver* receiver);
                static void unregisterReceiver(IGameInputReceiver* receiver);
                static void processEvent(const GameInputEvent& event);

#if defined(_WIN32)
                static void handleWin32Message(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif

        private:
                GameInput()                                      = default;
                ~GameInput()                                     = default;
                GameInput(const GameInput& other)            = delete;
                GameInput& operator=(const GameInput& other) = delete;

                struct InputReceiverPool
                {
                                std::array<IGameInputReceiver*, N> receivers{};
                                size_t                         count = 0;

                                InputReceiverPool()                                    = default;
                                InputReceiverPool(const InputReceiverPool&)            = delete;
                                InputReceiverPool& operator=(const InputReceiverPool&) = delete;
                };

                static bool               addReceiver(IGameInputReceiver* receiver);
                static bool               removeReceiver(IGameInputReceiver* receiver);
                static InputReceiverPool* getReceiverPool();

#if defined(_WIN32)
                static GameInputEvent::KeyCode     win32KeyCodeToKeyCode(WPARAM wParam);
                static GameInputEvent::MouseButton win32ButtonToMouseButton(UINT button);
#endif

                static bool keyStates[256];
                static bool ctrlPressed;
                static bool shiftPressed;
                static bool altPressed;

#if defined(_WIN32)
                static float lastMouseX;
                static float lastMouseY;
                static bool  mouseInitialized;
#endif
};

using GameInputInstance = GameInput<256>;

} // namespace rl

#endif
