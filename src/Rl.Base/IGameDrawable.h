#ifndef RL_BASE_GAME_DRAWABLE_H
#define RL_BASE_GAME_DRAWABLE_H

#include <array>
#include <cstdint>
#include <memory>
#include <type_traits>

namespace rl
{

class GameDeviceInstance;
class IGameDrawable
{
  public:
    virtual ~IGameDrawable()                         = default;
    virtual void setup(GameDeviceInstance& device)   = 0;
    virtual void draw(GameDeviceInstance& device)    = 0;
    virtual void destroy(GameDeviceInstance& device) = 0;

    void setCanBeRendered(bool canRender);
    bool getCanBeRendered() const;

    void setIsTest(bool isTest);
    bool getIsTest() const;

    void setTestCanRender(bool canRender);
    bool getTestCanRender() const;

    void setIsCompositing(bool isCompositing);
    bool getIsCompositing() const;

  protected:
    bool canBeRendered = true;
    bool isTest        = false;
    bool testCanRender = false;
    bool isCompositing = false;
};
template <class T, typename std::enable_if<std::is_base_of<IGameDrawable, T>::value, int>::type = 0>
struct IGameDrawableID
{
    inline static T instance{};
};

template <size_t N> class GameDrawableArenaTrack
{
  private:
    struct Slot
    {
        uintptr_t key;
        bool      occupied;
    };
    std::array<Slot, N> table;
    size_t              count;

    long hashFrom(uintptr_t ptr) const noexcept;
    long findSlot(uintptr_t ptr) const noexcept;

  public:
    GameDrawableArenaTrack() noexcept;
    bool containsDrawable(uintptr_t ptr) const noexcept;
    bool insertDrawable(uintptr_t ptr) noexcept;
    void clearDrawCallbacks() noexcept;
};

struct GameDrawableArenaRegion;
template <size_t N> class GameDrawableArena : protected GameDrawableArenaTrack<N>
{
    enum class CallbackType
    {
      draw,
      setup,
      destroy,
      composite
    };

  private:
    GameDrawableArenaRegion *begin, *end;

  public:
    GameDrawableArena() noexcept;
    GameDrawableArena(const GameDrawableArena& other)            = delete;
    GameDrawableArena& operator=(const GameDrawableArena& other) = delete;
    ~GameDrawableArena() noexcept;
    void executeDrawCallback(GameDeviceInstance& device, CallbackType cb);
    void setupDrawCallbacks(GameDeviceInstance& device) noexcept;
    void callDrawCallbacks(GameDeviceInstance& device) noexcept;
    void callCompositingCallbacks(GameDeviceInstance& device) noexcept;
    void destroyDrawCallbacks(GameDeviceInstance& device) noexcept;
    void pushDrawableAddress(IGameDrawable& drawable) noexcept;
};

struct GameDrawableArenaRegion
{
    GameDrawableArenaRegion* next;
    size_t                   capacity;
    size_t                   count;
    uintptr_t                data[];
    void*                    operator new(size_t n);
    void                     operator delete(void* ptr);
};

} // namespace rl

#endif
