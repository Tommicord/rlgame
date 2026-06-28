import Rl.Base.UserInput;

import <algorithm>;
import <chrono>;
import <mutex>;
import <thread>;
import <GLFW/glfw3.h>;

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#elif defined(__linux__)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#elif defined(__APPLE__)
#include <ApplicationServices/ApplicationServices.h>
#endif

namespace Rl::Input
{

UserInput::UserInput()
{
#if defined(__linux__)
  display = XOpenDisplay(nullptr);
#endif
}

UserInput::~UserInput()
{
  Stop();
#if defined(__linux__)
  if (display)
  {
    XCloseDisplay(static_cast<Display*>(display));
  }
#endif
}

UserInput& UserInput::GetInstance()
{
  static UserInput instance;
  return instance;
}

void UserInput::Subscribe(IInputObserver* observer)
{
  std::scoped_lock lock(observersMutex);
  observers.push_back(observer);
}

void UserInput::Unsubscribe(IInputObserver* observer)
{
  std::scoped_lock lock(observersMutex);
  std::erase(observers, observer);
}

void UserInput::NotifyKeyEvent(const KeyEvent& event)
{
  std::scoped_lock lock(observersMutex);
  for (auto* observer : observers)
  {
    observer->OnKeyEvent(event);
  }
}

void UserInput::NotifyMouseButtonEvent(const MouseButtonEvent& event)
{
  std::scoped_lock lock(observersMutex);
  for (auto* observer : observers)
  {
    observer->OnMouseButtonEvent(event);
  }
}

void UserInput::NotifyMouseMoveEvent(const MouseMoveEvent& event)
{
  std::scoped_lock lock(observersMutex);
  for (auto* observer : observers)
  {
    observer->OnMouseMoveEvent(event);
  }
}

void UserInput::NotifyMouseScrollEvent(const MouseScrollEvent& event)
{
  std::scoped_lock lock(observersMutex);
  for (auto* observer : observers)
  {
    observer->OnMouseScrollEvent(event);
  }
}

void UserInput::Start()
{
  if (running.load())
  {
    return;
  }
  running.store(true);
  thread = std::thread(&UserInput::InputThread, this);
}

void UserInput::Stop()
{
  if (!running.load())
  {
    return;
  }
  running.store(false);
  cv.notify_all();
  if (thread.joinable())
  {
    thread.join();
  }
}

void UserInput::InputThread()
{
  while (running.load())
  {
#if defined(_WIN32) || defined(_WIN64)
    PollWindowsInput();
#elif defined(__linux__)
    PollLinuxInput();
#elif defined(__APPLE__)
    PollMacInput();
#elif defined(__ANDROID__)
    PollAndroidInput();
#elif defined(__IOS__)
    PollIOSInput();
#endif
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }
}

#if defined(_WIN32) || defined(_WIN64)
Key WindowsKeyToInputKey(int vkCode)
{
  switch (vkCode)
  {
  case VK_SPACE:
    return Key::Space;
  case VK_OEM_7:
    return Key::Apostrophe;
  case VK_OEM_COMMA:
    return Key::Comma;
  case VK_OEM_MINUS:
    return Key::Minus;
  case VK_OEM_PERIOD:
    return Key::Period;
  case VK_OEM_2:
    return Key::Slash;
  case '0':
    return Key::Num0;
  case '1':
    return Key::Num1;
  case '2':
    return Key::Num2;
  case '3':
    return Key::Num3;
  case '4':
    return Key::Num4;
  case '5':
    return Key::Num5;
  case '6':
    return Key::Num6;
  case '7':
    return Key::Num7;
  case '8':
    return Key::Num8;
  case '9':
    return Key::Num9;
  case VK_OEM_1:
    return Key::Semicolon;
  case VK_OEM_PLUS:
    return Key::Equal;
  case 'A':
    return Key::A;
  case 'B':
    return Key::B;
  case 'C':
    return Key::C;
  case 'D':
    return Key::D;
  case 'E':
    return Key::E;
  case 'F':
    return Key::F;
  case 'G':
    return Key::G;
  case 'H':
    return Key::H;
  case 'I':
    return Key::I;
  case 'J':
    return Key::J;
  case 'K':
    return Key::K;
  case 'L':
    return Key::L;
  case 'M':
    return Key::M;
  case 'N':
    return Key::N;
  case 'O':
    return Key::O;
  case 'P':
    return Key::P;
  case 'Q':
    return Key::Q;
  case 'R':
    return Key::R;
  case 'S':
    return Key::S;
  case 'T':
    return Key::T;
  case 'U':
    return Key::U;
  case 'V':
    return Key::V;
  case 'W':
    return Key::W;
  case 'X':
    return Key::X;
  case 'Y':
    return Key::Y;
  case 'Z':
    return Key::Z;
  case VK_OEM_4:
    return Key::LeftBracket;
  case VK_OEM_5:
    return Key::Backslash;
  case VK_OEM_6:
    return Key::RightBracket;
  case VK_OEM_3:
    return Key::GraveAccent;
  case VK_ESCAPE:
    return Key::Escape;
  case VK_RETURN:
    return Key::Enter;
  case VK_TAB:
    return Key::Tab;
  case VK_BACK:
    return Key::Backspace;
  case VK_INSERT:
    return Key::Insert;
  case VK_DELETE:
    return Key::Delete;
  case VK_RIGHT:
    return Key::Right;
  case VK_LEFT:
    return Key::Left;
  case VK_DOWN:
    return Key::Down;
  case VK_UP:
    return Key::Up;
  case VK_PRIOR:
    return Key::PageUp;
  case VK_NEXT:
    return Key::PageDown;
  case VK_HOME:
    return Key::Home;
  case VK_END:
    return Key::End;
  case VK_CAPITAL:
    return Key::CapsLock;
  case VK_SCROLL:
    return Key::ScrollLock;
  case VK_NUMLOCK:
    return Key::NumLock;
  case VK_SNAPSHOT:
    return Key::PrintScreen;
  case VK_PAUSE:
    return Key::Pause;
  case VK_F1:
    return Key::F1;
  case VK_F2:
    return Key::F2;
  case VK_F3:
    return Key::F3;
  case VK_F4:
    return Key::F4;
  case VK_F5:
    return Key::F5;
  case VK_F6:
    return Key::F6;
  case VK_F7:
    return Key::F7;
  case VK_F8:
    return Key::F8;
  case VK_F9:
    return Key::F9;
  case VK_F10:
    return Key::F10;
  case VK_F11:
    return Key::F11;
  case VK_F12:
    return Key::F12;
  case VK_F13:
    return Key::F13;
  case VK_F14:
    return Key::F14;
  case VK_F15:
    return Key::F15;
  case VK_F16:
    return Key::F16;
  case VK_F17:
    return Key::F17;
  case VK_F18:
    return Key::F18;
  case VK_F19:
    return Key::F19;
  case VK_F20:
    return Key::F20;
  case VK_F21:
    return Key::F21;
  case VK_F22:
    return Key::F22;
  case VK_F23:
    return Key::F23;
  case VK_F24:
    return Key::F24;
  case VK_LSHIFT:
    return Key::LeftShift;
  case VK_LCONTROL:
    return Key::LeftControl;
  case VK_LMENU:
    return Key::LeftAlt;
  case VK_LWIN:
    return Key::LeftSuper;
  case VK_RSHIFT:
    return Key::RightShift;
  case VK_RCONTROL:
    return Key::RightControl;
  case VK_RMENU:
    return Key::RightAlt;
  case VK_RWIN:
    return Key::RightSuper;
  case VK_APPS:
    return Key::Menu;
  default:
    return Key::Unknown;
  }
}

void UserInput::PollWindowsInput()
{
  static int keyStates[256] = {0};

  for (int i = 0; i < 256; ++i)
  {
    const SHORT state = GetAsyncKeyState(i);
    const bool  isPressed = (state & 0x8000) != 0;

    int previousState = keyStates[i];
    keyStates[i] = isPressed ? 1 : 0;

    if (isPressed && previousState == 0)
    {
      KeyEvent event;
      event.key = WindowsKeyToInputKey(i);
      event.action = Action::Press;
      event.modifiers = 0;
      if (GetAsyncKeyState(VK_SHIFT))
        event.modifiers |= static_cast<int>(Modifier::Shift);
      if (GetAsyncKeyState(VK_CONTROL))
        event.modifiers |= static_cast<int>(Modifier::Control);
      if (GetAsyncKeyState(VK_MENU))
        event.modifiers |= static_cast<int>(Modifier::Alt);
      NotifyKeyEvent(event);
    }
    else if (!isPressed && previousState == 1)
    {
      KeyEvent event;
      event.key = WindowsKeyToInputKey(i);
      event.action = Action::Release;
      event.modifiers = 0;
      NotifyKeyEvent(event);
    }
  }
}
#endif

#if defined(__linux__)
Key X11KeyToInputKey(KeySym x11Key)
{
  switch (x11Key)
  {
  case XK_space:
    return Key::Space;
  case XK_apostrophe:
    return Key::Apostrophe;
  case XK_comma:
    return Key::Comma;
  case XK_minus:
    return Key::Minus;
  case XK_period:
    return Key::Period;
  case XK_slash:
    return Key::Slash;
  case XK_0:
    return Key::Num0;
  case XK_1:
    return Key::Num1;
  case XK_2:
    return Key::Num2;
  case XK_3:
    return Key::Num3;
  case XK_4:
    return Key::Num4;
  case XK_5:
    return Key::Num5;
  case XK_6:
    return Key::Num6;
  case XK_7:
    return Key::Num7;
  case XK_8:
    return Key::Num8;
  case XK_9:
    return Key::Num9;
  case XK_semicolon:
    return Key::Semicolon;
  case XK_equal:
    return Key::Equal;
  case XK_a:
    return Key::A;
  case XK_b:
    return Key::B;
  case XK_c:
    return Key::C;
  case XK_d:
    return Key::D;
  case XK_e:
    return Key::E;
  case XK_f:
    return Key::F;
  case XK_g:
    return Key::G;
  case XK_h:
    return Key::H;
  case XK_i:
    return Key::I;
  case XK_j:
    return Key::J;
  case XK_k:
    return Key::K;
  case XK_l:
    return Key::L;
  case XK_m:
    return Key::M;
  case XK_n:
    return Key::N;
  case XK_o:
    return Key::O;
  case XK_p:
    return Key::P;
  case XK_q:
    return Key::Q;
  case XK_r:
    return Key::R;
  case XK_s:
    return Key::S;
  case XK_t:
    return Key::T;
  case XK_u:
    return Key::U;
  case XK_v:
    return Key::V;
  case XK_w:
    return Key::W;
  case XK_x:
    return Key::X;
  case XK_y:
    return Key::Y;
  case XK_z:
    return Key::Z;
  case XK_bracketleft:
    return Key::LeftBracket;
  case XK_backslash:
    return Key::Backslash;
  case XK_bracketright:
    return Key::RightBracket;
  case XK_grave:
    return Key::GraveAccent;
  case XK_Escape:
    return Key::Escape;
  case XK_Return:
    return Key::Enter;
  case XK_Tab:
    return Key::Tab;
  case XK_BackSpace:
    return Key::Backspace;
  case XK_Insert:
    return Key::Insert;
  case XK_Delete:
    return Key::Delete;
  case XK_Right:
    return Key::Right;
  case XK_Left:
    return Key::Left;
  case XK_Down:
    return Key::Down;
  case XK_Up:
    return Key::Up;
  case XK_Prior:
    return Key::PageUp;
  case XK_Next:
    return Key::PageDown;
  case XK_Home:
    return Key::Home;
  case XK_End:
    return Key::End;
  case XK_Caps_Lock:
    return Key::CapsLock;
  case XK_Scroll_Lock:
    return Key::ScrollLock;
  case XK_Num_Lock:
    return Key::NumLock;
  case XK_Print:
    return Key::PrintScreen;
  case XK_Pause:
    return Key::Pause;
  case XK_F1:
    return Key::F1;
  case XK_F2:
    return Key::F2;
  case XK_F3:
    return Key::F3;
  case XK_F4:
    return Key::F4;
  case XK_F5:
    return Key::F5;
  case XK_F6:
    return Key::F6;
  case XK_F7:
    return Key::F7;
  case XK_F8:
    return Key::F8;
  case XK_F9:
    return Key::F9;
  case XK_F10:
    return Key::F10;
  case XK_F11:
    return Key::F11;
  case XK_F12:
    return Key::F12;
  case XK_F13:
    return Key::F13;
  case XK_F14:
    return Key::F14;
  case XK_F15:
    return Key::F15;
  case XK_F16:
    return Key::F16;
  case XK_F17:
    return Key::F17;
  case XK_F18:
    return Key::F18;
  case XK_F19:
    return Key::F19;
  case XK_F20:
    return Key::F20;
  case XK_F21:
    return Key::F21;
  case XK_F22:
    return Key::F22;
  case XK_F23:
    return Key::F23;
  case XK_F24:
    return Key::F24;
  case XK_Shift_L:
    return Key::LeftShift;
  case XK_Control_L:
    return Key::LeftControl;
  case XK_Alt_L:
    return Key::LeftAlt;
  case XK_Super_L:
    return Key::LeftSuper;
  case XK_Shift_R:
    return Key::RightShift;
  case XK_Control_R:
    return Key::RightControl;
  case XK_Alt_R:
    return Key::RightAlt;
  case XK_Super_R:
    return Key::RightSuper;
  case XK_Menu:
    return Key::Menu;
  default:
    return Key::Unknown;
  }
}

void UserInput::PollLinuxInput()
{
  Display* dpy = static_cast<Display*>(display);
  if (!dpy)
    return;

  XEvent event;
  while (XPending(dpy) > 0)
  {
    XNextEvent(dpy, &event);

    switch (event.type)
    {
    case KeyPress:
    case KeyRelease:
      {
        KeyEvent keyEvent;
        KeySym   keysym = XLookupKeysym(&event.xkey, 0);
        keyEvent.key = X11KeyToInputKey(keysym);
        keyEvent.action = (event.type == KeyPress) ? Action::Press : Action::Release;
        keyEvent.modifiers = 0;
        if (event.xkey.state & ShiftMask)
          keyEvent.modifiers |= static_cast<int>(Modifier::Shift);
        if (event.xkey.state & ControlMask)
          keyEvent.modifiers |= static_cast<int>(Modifier::Control);
        if (event.xkey.state & Mod1Mask)
          keyEvent.modifiers |= static_cast<int>(Modifier::Alt);
        NotifyKeyEvent(keyEvent);
        break;
      }
    case ButtonPress:
    case ButtonRelease:
      {
        MouseButtonEvent mouseEvent;
        mouseEvent.button = static_cast<MouseButton>(event.xbutton.button);
        mouseEvent.action = (event.type == ButtonPress) ? Action::Press : Action::Release;
        mouseEvent.modifiers = 0;
        NotifyMouseButtonEvent(mouseEvent);
        break;
      }
    case MotionNotify:
      {
        MouseMoveEvent moveEvent;
        moveEvent.x = event.xmotion.x;
        moveEvent.y = event.xmotion.y;
        NotifyMouseMoveEvent(moveEvent);
        break;
      }
    }
  }
}
#endif

#if defined(__APPLE__)
void UserInput::PollMacInput()
{
  CGEventRef event = CGEventCreate(nullptr);
  CGPoint    cursor = CGEventGetLocation(event);
  CFRelease(event);

  static CGPoint lastCursor = {0, 0};
  if (cursor.x != lastCursor.x || cursor.y != lastCursor.y)
  {
    MouseMoveEvent moveEvent;
    moveEvent.x = cursor.x;
    moveEvent.y = cursor.y;
    NotifyMouseMoveEvent(moveEvent);
    lastCursor = cursor;
  }
}
#endif

#if defined(__ANDROID__)
void UserInput::PollAndroidInput()
{
}
#endif

#if defined(__IOS__)
void UserInput::PollIOSInput()
{
}
#endif

Input::Key GlfwKeyToInputKey(int glfwKey)
{
  switch (glfwKey)
  {
  case GLFW_KEY_SPACE:
    return Input::Key::Space;
  case GLFW_KEY_APOSTROPHE:
    return Input::Key::Apostrophe;
  case GLFW_KEY_COMMA:
    return Input::Key::Comma;
  case GLFW_KEY_MINUS:
    return Input::Key::Minus;
  case GLFW_KEY_PERIOD:
    return Input::Key::Period;
  case GLFW_KEY_SLASH:
    return Input::Key::Slash;
  case GLFW_KEY_0:
    return Input::Key::Num0;
  case GLFW_KEY_1:
    return Input::Key::Num1;
  case GLFW_KEY_2:
    return Input::Key::Num2;
  case GLFW_KEY_3:
    return Input::Key::Num3;
  case GLFW_KEY_4:
    return Input::Key::Num4;
  case GLFW_KEY_5:
    return Input::Key::Num5;
  case GLFW_KEY_6:
    return Input::Key::Num6;
  case GLFW_KEY_7:
    return Input::Key::Num7;
  case GLFW_KEY_8:
    return Input::Key::Num8;
  case GLFW_KEY_9:
    return Input::Key::Num9;
  case GLFW_KEY_SEMICOLON:
    return Input::Key::Semicolon;
  case GLFW_KEY_EQUAL:
    return Input::Key::Equal;
  case GLFW_KEY_A:
    return Input::Key::A;
  case GLFW_KEY_B:
    return Input::Key::B;
  case GLFW_KEY_C:
    return Input::Key::C;
  case GLFW_KEY_D:
    return Input::Key::D;
  case GLFW_KEY_E:
    return Input::Key::E;
  case GLFW_KEY_F:
    return Input::Key::F;
  case GLFW_KEY_G:
    return Input::Key::G;
  case GLFW_KEY_H:
    return Input::Key::H;
  case GLFW_KEY_I:
    return Input::Key::I;
  case GLFW_KEY_J:
    return Input::Key::J;
  case GLFW_KEY_K:
    return Input::Key::K;
  case GLFW_KEY_L:
    return Input::Key::L;
  case GLFW_KEY_M:
    return Input::Key::M;
  case GLFW_KEY_N:
    return Input::Key::N;
  case GLFW_KEY_O:
    return Input::Key::O;
  case GLFW_KEY_P:
    return Input::Key::P;
  case GLFW_KEY_Q:
    return Input::Key::Q;
  case GLFW_KEY_R:
    return Input::Key::R;
  case GLFW_KEY_S:
    return Input::Key::S;
  case GLFW_KEY_T:
    return Input::Key::T;
  case GLFW_KEY_U:
    return Input::Key::U;
  case GLFW_KEY_V:
    return Input::Key::V;
  case GLFW_KEY_W:
    return Input::Key::W;
  case GLFW_KEY_X:
    return Input::Key::X;
  case GLFW_KEY_Y:
    return Input::Key::Y;
  case GLFW_KEY_Z:
    return Input::Key::Z;
  case GLFW_KEY_LEFT_BRACKET:
    return Input::Key::LeftBracket;
  case GLFW_KEY_BACKSLASH:
    return Input::Key::Backslash;
  case GLFW_KEY_RIGHT_BRACKET:
    return Input::Key::RightBracket;
  case GLFW_KEY_GRAVE_ACCENT:
    return Input::Key::GraveAccent;
  case GLFW_KEY_ESCAPE:
    return Input::Key::Escape;
  case GLFW_KEY_ENTER:
    return Input::Key::Enter;
  case GLFW_KEY_TAB:
    return Input::Key::Tab;
  case GLFW_KEY_BACKSPACE:
    return Input::Key::Backspace;
  case GLFW_KEY_INSERT:
    return Input::Key::Insert;
  case GLFW_KEY_DELETE:
    return Input::Key::Delete;
  case GLFW_KEY_RIGHT:
    return Input::Key::Right;
  case GLFW_KEY_LEFT:
    return Input::Key::Left;
  case GLFW_KEY_DOWN:
    return Input::Key::Down;
  case GLFW_KEY_UP:
    return Input::Key::Up;
  case GLFW_KEY_PAGE_UP:
    return Input::Key::PageUp;
  case GLFW_KEY_PAGE_DOWN:
    return Input::Key::PageDown;
  case GLFW_KEY_HOME:
    return Input::Key::Home;
  case GLFW_KEY_END:
    return Input::Key::End;
  case GLFW_KEY_CAPS_LOCK:
    return Input::Key::CapsLock;
  case GLFW_KEY_SCROLL_LOCK:
    return Input::Key::ScrollLock;
  case GLFW_KEY_NUM_LOCK:
    return Input::Key::NumLock;
  case GLFW_KEY_PRINT_SCREEN:
    return Input::Key::PrintScreen;
  case GLFW_KEY_PAUSE:
    return Input::Key::Pause;
  case GLFW_KEY_F1:
    return Input::Key::F1;
  case GLFW_KEY_F2:
    return Input::Key::F2;
  case GLFW_KEY_F3:
    return Input::Key::F3;
  case GLFW_KEY_F4:
    return Input::Key::F4;
  case GLFW_KEY_F5:
    return Input::Key::F5;
  case GLFW_KEY_F6:
    return Input::Key::F6;
  case GLFW_KEY_F7:
    return Input::Key::F7;
  case GLFW_KEY_F8:
    return Input::Key::F8;
  case GLFW_KEY_F9:
    return Input::Key::F9;
  case GLFW_KEY_F10:
    return Input::Key::F10;
  case GLFW_KEY_F11:
    return Input::Key::F11;
  case GLFW_KEY_F12:
    return Input::Key::F12;
  case GLFW_KEY_KP_0:
    return Input::Key::Kp0;
  case GLFW_KEY_KP_1:
    return Input::Key::Kp1;
  case GLFW_KEY_KP_2:
    return Input::Key::Kp2;
  case GLFW_KEY_KP_3:
    return Input::Key::Kp3;
  case GLFW_KEY_KP_4:
    return Input::Key::Kp4;
  case GLFW_KEY_KP_5:
    return Input::Key::Kp5;
  case GLFW_KEY_KP_6:
    return Input::Key::Kp6;
  case GLFW_KEY_KP_7:
    return Input::Key::Kp7;
  case GLFW_KEY_KP_8:
    return Input::Key::Kp8;
  case GLFW_KEY_KP_9:
    return Input::Key::Kp9;
  case GLFW_KEY_KP_DECIMAL:
    return Input::Key::KpDecimal;
  case GLFW_KEY_KP_DIVIDE:
    return Input::Key::KpDivide;
  case GLFW_KEY_KP_MULTIPLY:
    return Input::Key::KpMultiply;
  case GLFW_KEY_KP_SUBTRACT:
    return Input::Key::KpSubtract;
  case GLFW_KEY_KP_ADD:
    return Input::Key::KpAdd;
  case GLFW_KEY_KP_ENTER:
    return Input::Key::KpEnter;
  case GLFW_KEY_KP_EQUAL:
    return Input::Key::KpEqual;
  case GLFW_KEY_LEFT_SHIFT:
    return Input::Key::LeftShift;
  case GLFW_KEY_LEFT_CONTROL:
    return Input::Key::LeftControl;
  case GLFW_KEY_LEFT_ALT:
    return Input::Key::LeftAlt;
  case GLFW_KEY_LEFT_SUPER:
    return Input::Key::LeftSuper;
  case GLFW_KEY_RIGHT_SHIFT:
    return Input::Key::RightShift;
  case GLFW_KEY_RIGHT_CONTROL:
    return Input::Key::RightControl;
  case GLFW_KEY_RIGHT_ALT:
    return Input::Key::RightAlt;
  case GLFW_KEY_RIGHT_SUPER:
    return Input::Key::RightSuper;
  case GLFW_KEY_MENU:
    return Input::Key::Menu;
  default:
    return Input::Key::Unknown;
  }
}

} // namespace Rl::Input
