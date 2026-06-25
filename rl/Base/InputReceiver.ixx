export module Rl.Base.InputReceiver;

import <atomic>;
import <condition_variable>;
import <functional>;
import <memory>;
import <mutex>;
import <thread>;
import <vector>;

namespace Rl::Input
{

export enum class Key
{
  Unknown      = 0,
  Space        = 32,
  Apostrophe   = 39,
  Comma        = 44,
  Minus        = 45,
  Period       = 46,
  Slash        = 47,
  Num0         = 48,
  Num1         = 49,
  Num2         = 50,
  Num3         = 51,
  Num4         = 52,
  Num5         = 53,
  Num6         = 54,
  Num7         = 55,
  Num8         = 56,
  Num9         = 57,
  Semicolon    = 59,
  Equal        = 61,
  A            = 65,
  B            = 66,
  C            = 67,
  D            = 68,
  E            = 69,
  F            = 70,
  G            = 71,
  H            = 72,
  I            = 73,
  J            = 74,
  K            = 75,
  L            = 76,
  M            = 77,
  N            = 78,
  O            = 79,
  P            = 80,
  Q            = 81,
  R            = 82,
  S            = 83,
  T            = 84,
  U            = 85,
  V            = 86,
  W            = 87,
  X            = 88,
  Y            = 89,
  Z            = 90,
  LeftBracket  = 91,
  Backslash    = 92,
  RightBracket = 93,
  GraveAccent  = 96,
  World1       = 161,
  World2       = 162,
  Escape       = 256,
  Enter        = 257,
  Tab          = 258,
  Backspace    = 259,
  Insert       = 260,
  Delete       = 261,
  Right        = 262,
  Left         = 263,
  Down         = 264,
  Up           = 265,
  PageUp       = 266,
  PageDown     = 267,
  Home         = 268,
  End          = 269,
  CapsLock     = 280,
  ScrollLock   = 281,
  NumLock      = 282,
  PrintScreen  = 283,
  Pause        = 284,
  F1           = 290,
  F2           = 291,
  F3           = 292,
  F4           = 293,
  F5           = 294,
  F6           = 295,
  F7           = 296,
  F8           = 297,
  F9           = 298,
  F10          = 299,
  F11          = 300,
  F12          = 301,
  F13          = 302,
  F14          = 303,
  F15          = 304,
  F16          = 305,
  F17          = 306,
  F18          = 307,
  F19          = 308,
  F20          = 309,
  F21          = 310,
  F22          = 311,
  F23          = 312,
  F24          = 313,
  F25          = 314,
  Kp0          = 320,
  Kp1          = 321,
  Kp2          = 322,
  Kp3          = 323,
  Kp4          = 324,
  Kp5          = 325,
  Kp6          = 326,
  Kp7          = 327,
  Kp8          = 328,
  Kp9          = 329,
  KpDecimal    = 330,
  KpDivide     = 331,
  KpMultiply   = 332,
  KpSubtract   = 333,
  KpAdd        = 334,
  KpEnter      = 335,
  KpEqual      = 336,
  LeftShift    = 340,
  LeftControl  = 341,
  LeftAlt      = 342,
  LeftSuper    = 343,
  RightShift   = 344,
  RightControl = 345,
  RightAlt     = 346,
  RightSuper   = 347,
  Menu         = 348
};

export enum class MouseButton
{
  Unknown = 0,
  Left    = 1,
  Right   = 2,
  Middle  = 3,
  Button4 = 4,
  Button5 = 5,
  Button6 = 6,
  Button7 = 7,
  Button8 = 8
};

export enum class Action
{
  Release = 0,
  Press   = 1,
  Repeat  = 2
};

export enum class Modifier
{
  None     = 0,
  Shift    = 1,
  Control  = 2,
  Alt      = 4,
  Super    = 8,
  CapsLock = 16,
  NumLock  = 32
};

export struct KeyEvent
{
  Key    key;
  Action action;
  int    modifiers;
};

export struct MouseButtonEvent
{
  MouseButton button;
  Action      action;
  int         modifiers;
};

export struct MouseMoveEvent
{
  double x, y;
};

export struct MouseScrollEvent
{
  double xoffset, yoffset;
};

export class InputObserver
{
  public:
  virtual ~InputObserver()                                       = default;
  virtual void OnKeyEvent(const KeyEvent& event)                 = 0;
  virtual void OnMouseButtonEvent(const MouseButtonEvent& event) = 0;
  virtual void OnMouseMoveEvent(const MouseMoveEvent& event)     = 0;
  virtual void OnMouseScrollEvent(const MouseScrollEvent& event) = 0;
};

export class InputReceiver
{
  public:
  static InputReceiver& GetInstance();
  InputReceiver(const InputReceiver&)            = delete;
  InputReceiver& operator=(const InputReceiver&) = delete;
  InputReceiver(InputReceiver&&)                 = delete;
  InputReceiver& operator=(InputReceiver&&)      = delete;
  void           Subscribe(InputObserver* observer);
  void           Unsubscribe(InputObserver* observer);
  void           NotifyKeyEvent(const KeyEvent& event);
  void           NotifyMouseButtonEvent(const MouseButtonEvent& event);
  void           NotifyMouseMoveEvent(const MouseMoveEvent& event);
  void           NotifyMouseScrollEvent(const MouseScrollEvent& event);
  void           Start();
  void           Stop();

  private:
  InputReceiver();
  ~InputReceiver();
  void                        InputThread();
  void                        PollWindowsInput();
  void                        PollLinuxInput();
  void                        PollMacInput();
  void                        PollAndroidInput();
  void                        PollIOSInput();
  std::vector<InputObserver*> observers;
  std::mutex                  observersMutex;
  std::thread                 inputThread;
  std::atomic<bool>           running{false};
  std::condition_variable     cv;
  std::mutex                  cvMutex;
#if defined(__linux__)
  void* display;
#endif
};

} // namespace Rl::Input
