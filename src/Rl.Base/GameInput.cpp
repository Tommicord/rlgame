#include "Rl.Base/GameInput.h"

#if defined(_WIN32)
#include <windows.h>
#include <windowsx.h>
#endif

namespace rl
{

template <size_t N> bool GameInput<N>::keyStates[256] = {false};
template <size_t N> bool GameInput<N>::ctrlPressed    = false;
template <size_t N> bool GameInput<N>::shiftPressed   = false;
template <size_t N> bool GameInput<N>::altPressed     = false;

#if defined(_WIN32)
template <size_t N> float GameInput<N>::lastMouseX       = 0.0f;
template <size_t N> float GameInput<N>::lastMouseY       = 0.0f;
template <size_t N> bool  GameInput<N>::mouseInitialized = false;
#endif

template <size_t N>
typename GameInput<N>::InputReceiverPool* GameInput<N>::getReceiverPool()
{
        static InputReceiverPool pool;
        return &pool;
}

template <size_t N> bool GameInput<N>::addReceiver(IGameInputReceiver* receiver)
{
        if (receiver == nullptr)
        {
                return false;
        }
        auto* pool = getReceiverPool();
        for (size_t i = 0; i < pool->count; ++i)
        {
                if (pool->receivers[i] == receiver)
                {
                        return false;
                }
        }
        if (pool->count >= N)
        {
                return false;
        }
        pool->receivers[pool->count] = receiver;
        pool->count++;
        return true;
}

template <size_t N> bool GameInput<N>::removeReceiver(IGameInputReceiver* receiver)
{
        if (receiver == nullptr)
        {
                return false;
        }
        auto* pool = getReceiverPool();
        for (size_t i = 0; i < pool->count; ++i)
        {
                if (pool->receivers[i] == receiver)
                {
                        for (size_t j = i; j < pool->count - 1; ++j)
                        {
                                pool->receivers[j] = pool->receivers[j + 1];
                        }
                        pool->receivers[pool->count - 1] = nullptr;
                        pool->count--;
                        return true;
                }
        }

        return false;
}

template <size_t N> void GameInput<N>::registerReceiver(IGameInputReceiver* receiver)
{
        addReceiver(receiver);
}

template <size_t N> void GameInput<N>::unregisterReceiver(IGameInputReceiver* receiver)
{
        removeReceiver(receiver);
}

template <size_t N> void GameInput<N>::processEvent(const GameInputEvent& event)
{
        auto* pool = getReceiverPool();
        for (size_t i = 0; i < pool->count; ++i)
        {
                IGameInputReceiver* receiver = pool->receivers[i];
                if (receiver == nullptr)
                {
                        continue;
                }

                switch (event.type)
                {
                case GameInputEvent::Type::KeyPress:
                        receiver->onKeyPress(event.keyCode, event.ctrlPressed, event.shiftPressed,
                                             event.altPressed);
                        break;
                case GameInputEvent::Type::KeyRelease:
                        receiver->onKeyRelease(event.keyCode, event.ctrlPressed, event.shiftPressed,
                                               event.altPressed);
                        break;
                case GameInputEvent::Type::KeyRepeat:
                        receiver->onKeyRepeat(event.keyCode, event.ctrlPressed, event.shiftPressed,
                                              event.altPressed);
                        break;
                case GameInputEvent::Type::MouseMove:
                        receiver->onMouseMove(event.mouseX, event.mouseY);
                        break;
                case GameInputEvent::Type::MouseButtonDown:
                        receiver->onMouseButtonDown(event.button, event.mouseX, event.mouseY);
                        break;
                case GameInputEvent::Type::MouseButtonUp:
                        receiver->onMouseButtonUp(event.button, event.mouseX, event.mouseY);
                        break;
                case GameInputEvent::Type::MouseScroll:
                        receiver->onMouseScroll(event.scrollDeltaX, event.scrollDeltaY,
                                                event.mouseX, event.mouseY);
                        break;
                default:
                        break;
                }
        }
}

#if defined(_WIN32)
template <size_t N> GameInputEvent::KeyCode GameInput<N>::win32KeyCodeToKeyCode(WPARAM wParam)
{
        switch (wParam)
        {
        case VK_ESCAPE:
                return GameInputEvent::KeyCode::Escape;
        case VK_RETURN:
                return GameInputEvent::KeyCode::Enter;
        case VK_SPACE:
                return GameInputEvent::KeyCode::Space;
        case VK_BACK:
                return GameInputEvent::KeyCode::Backspace;
        case VK_TAB:
                return GameInputEvent::KeyCode::Tab;
        case VK_SHIFT:
                return GameInputEvent::KeyCode::LeftShift;
        case VK_CONTROL:
                return GameInputEvent::KeyCode::LeftControl;
        case VK_MENU:
                return GameInputEvent::KeyCode::LeftAlt;
        case VK_INSERT:
                return GameInputEvent::KeyCode::Insert;
        case VK_DELETE:
                return GameInputEvent::KeyCode::Delete;
        case VK_HOME:
                return GameInputEvent::KeyCode::Home;
        case VK_END:
                return GameInputEvent::KeyCode::End;
        case VK_PRIOR:
                return GameInputEvent::KeyCode::PageUp;
        case VK_NEXT:
                return GameInputEvent::KeyCode::PageDown;
        case VK_UP:
                return GameInputEvent::KeyCode::Up;
        case VK_DOWN:
                return GameInputEvent::KeyCode::Down;
        case VK_LEFT:
                return GameInputEvent::KeyCode::Left;
        case VK_RIGHT:
                return GameInputEvent::KeyCode::Right;
        case VK_F1:
                return GameInputEvent::KeyCode::F1;
        case VK_F2:
                return GameInputEvent::KeyCode::F2;
        case VK_F3:
                return GameInputEvent::KeyCode::F3;
        case VK_F4:
                return GameInputEvent::KeyCode::F4;
        case VK_F5:
                return GameInputEvent::KeyCode::F5;
        case VK_F6:
                return GameInputEvent::KeyCode::F6;
        case VK_F7:
                return GameInputEvent::KeyCode::F7;
        case VK_F8:
                return GameInputEvent::KeyCode::F8;
        case VK_F9:
                return GameInputEvent::KeyCode::F9;
        case VK_F10:
                return GameInputEvent::KeyCode::F10;
        case VK_F11:
                return GameInputEvent::KeyCode::F11;
        case VK_F12:
                return GameInputEvent::KeyCode::F12;
        case '0':
                return GameInputEvent::KeyCode::Key0;
        case '1':
                return GameInputEvent::KeyCode::Key1;
        case '2':
                return GameInputEvent::KeyCode::Key2;
        case '3':
                return GameInputEvent::KeyCode::Key3;
        case '4':
                return GameInputEvent::KeyCode::Key4;
        case '5':
                return GameInputEvent::KeyCode::Key5;
        case '6':
                return GameInputEvent::KeyCode::Key6;
        case '7':
                return GameInputEvent::KeyCode::Key7;
        case '8':
                return GameInputEvent::KeyCode::Key8;
        case '9':
                return GameInputEvent::KeyCode::Key9;
        case 'A':
                return GameInputEvent::KeyCode::A;
        case 'B':
                return GameInputEvent::KeyCode::B;
        case 'C':
                return GameInputEvent::KeyCode::C;
        case 'D':
                return GameInputEvent::KeyCode::D;
        case 'E':
                return GameInputEvent::KeyCode::E;
        case 'F':
                return GameInputEvent::KeyCode::F;
        case 'G':
                return GameInputEvent::KeyCode::G;
        case 'H':
                return GameInputEvent::KeyCode::H;
        case 'I':
                return GameInputEvent::KeyCode::I;
        case 'J':
                return GameInputEvent::KeyCode::J;
        case 'K':
                return GameInputEvent::KeyCode::K;
        case 'L':
                return GameInputEvent::KeyCode::L;
        case 'M':
                return GameInputEvent::KeyCode::M;
        case 'N':
                return GameInputEvent::KeyCode::N;
        case 'O':
                return GameInputEvent::KeyCode::O;
        case 'P':
                return GameInputEvent::KeyCode::P;
        case 'Q':
                return GameInputEvent::KeyCode::Q;
        case 'R':
                return GameInputEvent::KeyCode::R;
        case 'S':
                return GameInputEvent::KeyCode::S;
        case 'T':
                return GameInputEvent::KeyCode::T;
        case 'U':
                return GameInputEvent::KeyCode::U;
        case 'V':
                return GameInputEvent::KeyCode::V;
        case 'W':
                return GameInputEvent::KeyCode::W;
        case 'X':
                return GameInputEvent::KeyCode::X;
        case 'Y':
                return GameInputEvent::KeyCode::Y;
        case 'Z':
                return GameInputEvent::KeyCode::Z;
        default:
                return GameInputEvent::KeyCode::Unknown;
        }
}

template <size_t N> GameInputEvent::MouseButton GameInput<N>::win32ButtonToMouseButton(UINT button)
{
        switch (button)
        {
        case VK_LBUTTON:
                return GameInputEvent::MouseButton::Left;
        case VK_RBUTTON:
                return GameInputEvent::MouseButton::Right;
        case VK_MBUTTON:
                return GameInputEvent::MouseButton::Middle;
        case VK_XBUTTON1:
                return GameInputEvent::MouseButton::X1;
        case VK_XBUTTON2:
                return GameInputEvent::MouseButton::X2;
        default:
                return GameInputEvent::MouseButton::None;
        }
}

template <size_t N>
void GameInput<N>::handleWin32Message(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
        GameInputEvent event;
        event.type         = GameInputEvent::Type::None;
        event.ctrlPressed  = ctrlPressed;
        event.shiftPressed = shiftPressed;
        event.altPressed   = altPressed;

        switch (uMsg)
        {
        case WM_KEYDOWN:
                {
                        bool wasPressed          = keyStates[wParam & 0xFF];
                        keyStates[wParam & 0xFF] = true;

                        event.type =
                            wasPressed ? GameInputEvent::Type::KeyRepeat : GameInputEvent::Type::KeyPress;
                        event.keyCode = win32KeyCodeToKeyCode(wParam);

                        if (wParam == VK_CONTROL)
                                ctrlPressed = true;
                        if (wParam == VK_SHIFT)
                                shiftPressed = true;
                        if (wParam == VK_MENU)
                                altPressed = true;

                        event.ctrlPressed  = ctrlPressed;
                        event.shiftPressed = shiftPressed;
                        event.altPressed   = altPressed;

                        processEvent(event);
                        break;
                }
        case WM_KEYUP:
                {
                        keyStates[wParam & 0xFF] = false;
                        event.type               = GameInputEvent::Type::KeyRelease;
                        event.keyCode            = win32KeyCodeToKeyCode(wParam);

                        if (wParam == VK_CONTROL)
                                ctrlPressed = false;
                        if (wParam == VK_SHIFT)
                                shiftPressed = false;
                        if (wParam == VK_MENU)
                                altPressed = false;

                        event.ctrlPressed  = ctrlPressed;
                        event.shiftPressed = shiftPressed;
                        event.altPressed   = altPressed;

                        processEvent(event);
                        break;
                }
        case WM_MOUSEMOVE:
                {
                        float currentX = static_cast<float>(GET_X_LPARAM(lParam));
                        float currentY = static_cast<float>(GET_Y_LPARAM(lParam));

                        if (!mouseInitialized)
                        {
                                lastMouseX       = currentX;
                                lastMouseY       = currentY;
                                mouseInitialized = true;
                                break;
                        }

                        RECT rect;
                        GetClientRect(hwnd, &rect);
                        float centerX = static_cast<float>((rect.left + rect.right) / 2);
                        float centerY = static_cast<float>((rect.top + rect.bottom) / 2);

                        event.type   = GameInputEvent::Type::MouseMove;
                        event.mouseX = currentX - centerX;
                        event.mouseY = currentY - centerY;

                        processEvent(event);

                        POINT center = {(rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2};
                        ClientToScreen(hwnd, &center);
                        SetCursorPos(center.x, center.y);

                        break;
                }
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_XBUTTONDOWN:
                {
                        event.type = GameInputEvent::Type::MouseButtonDown;
                        event.button =
                            (uMsg == WM_XBUTTONDOWN)
                                ? ((GET_XBUTTON_WPARAM(wParam) == XBUTTON1)
                                       ? GameInputEvent::MouseButton::X1
                                       : GameInputEvent::MouseButton::X2)
                                : win32ButtonToMouseButton(uMsg - WM_LBUTTONDOWN + VK_LBUTTON);
                        event.mouseX = static_cast<float>(GET_X_LPARAM(lParam));
                        event.mouseY = static_cast<float>(GET_Y_LPARAM(lParam));
                        processEvent(event);
                        break;
                }
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_XBUTTONUP:
                {
                        event.type = GameInputEvent::Type::MouseButtonUp;
                        event.button =
                            (uMsg == WM_XBUTTONUP)
                                ? ((GET_XBUTTON_WPARAM(wParam) == XBUTTON1)
                                       ? GameInputEvent::MouseButton::X1
                                       : GameInputEvent::MouseButton::X2)
                                : win32ButtonToMouseButton(uMsg - WM_LBUTTONUP + VK_LBUTTON);
                        event.mouseX = static_cast<float>(GET_X_LPARAM(lParam));
                        event.mouseY = static_cast<float>(GET_Y_LPARAM(lParam));
                        processEvent(event);
                        break;
                }
        case WM_MOUSEWHEEL:
                {
                        event.type = GameInputEvent::Type::MouseScroll;
                        event.scrollDeltaY =
                            static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;
                        POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                        ScreenToClient(hwnd, &pt);
                        event.mouseX = static_cast<float>(pt.x);
                        event.mouseY = static_cast<float>(pt.y);
                        processEvent(event);
                        break;
                }
        case WM_MOUSEHWHEEL:
                {
                        event.type = GameInputEvent::Type::MouseScroll;
                        event.scrollDeltaX =
                            static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;
                        POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                        ScreenToClient(hwnd, &pt);
                        event.mouseX = static_cast<float>(pt.x);
                        event.mouseY = static_cast<float>(pt.y);
                        processEvent(event);
                        break;
                }
        default:
                break;
        }
}
#endif

template class GameInput<64>;
template class GameInput<128>;
template class GameInput<256>;

} // namespace rl
