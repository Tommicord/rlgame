#include "Rl.Base/IGameDrawable.h"
#include "Rl.Base/GameDevice.h"

#if defined(_WIN32)
#include <windows.h>

namespace rl
{

void IGameDrawable::setCanBeRendered(bool canRender)
{
        canBeRendered = canRender;
}

bool IGameDrawable::getCanBeRendered() const
{
        return canBeRendered;
}

void IGameDrawable::setIsTest(bool isTestValue)
{
        isTest = isTestValue;
}

bool IGameDrawable::getIsTest() const
{
        return isTest;
}

void IGameDrawable::setTestCanRender(bool canRender)
{
        testCanRender = canRender;
}

bool IGameDrawable::getTestCanRender() const
{
        return testCanRender;
}

void IGameDrawable::setIsCompositing(bool isCompositingValue)
{
        isCompositing = isCompositingValue;
}

bool IGameDrawable::getIsCompositing() const
{
        return isCompositing;
}

void* GameDrawableArenaRegion::operator new(size_t n)
{
        SIZE_T                   size = sizeof(GameDrawableArenaRegion) + sizeof(uintptr_t) * n;
        GameDrawableArenaRegion* r    = (GameDrawableArenaRegion*)VirtualAllocEx(
            GetCurrentProcess(), /* Allocate in current process address space */
            NULL, /* Unknown position */
            size, /* Bytes to allocate */
            MEM_COMMIT | MEM_RESERVE, /* Reserve and commit allocated page */
            PAGE_READWRITE /* Permissions ( Read/Write )*/
        );
        if (r == NULL || r == INVALID_HANDLE_VALUE)
        {
                throw std::bad_alloc();
        }
        r->next     = NULL;
        r->count    = 0;
        r->capacity = n;
        return r;
}

void GameDrawableArenaRegion::operator delete(void* ptr)
{
        VirtualFreeEx(GetCurrentProcess(), /* Deallocate from current process address space */
                      (LPVOID)ptr, /* Address to deallocate */
                      0, /* Bytes to deallocate ( Unknown, deallocate entire page ) */
                      MEM_RELEASE /* Release the page ( And implicitly decommit it ) */
        );
}

#elif defined(__linux__)
#include <sys/mman.h>
#include <unistd.h>

void* GameDrawableArenaRegion::operator new(size_t n)
{
        size_t                   size = sizeof(GameDrawableArenaRegion) + sizeof(uintptr_t) * n;
        GameDrawableArenaRegion* r =
            mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        if (r == MAP_FAILED)
        {
                throw std::bad_alloc();
        }
        r->next     = NULL;
        r->count    = 0;
        r->capacity = n;
        return r;
}

void GameDrawableArenaRegion::operator delete(void* ptr)
{
        GameDrawableArenaRegion* r = static_cast<GameDrawableArenaRegion*>(ptr);
        size_t bytes = sizeof(GameDrawableArenaRegion) + sizeof(uintptr_t) * r->capacity;
        int    ret   = munmap(r, bytes);
}

#else
#include <new>

void* GameDrawableArenaRegion::operator new(size_t n)
{
        size_t size = sizeof(GameDrawableArenaRegion) + sizeof(uintptr_t) * n;
        void*  ptr  = std::malloc(size);
        if (ptr == nullptr)
        {
                throw std::bad_alloc();
        }
        return ptr;
}

void GameDrawableArenaRegion::operator delete(void* ptr)
{
        std::free(ptr);
}

#endif

template <size_t N>
GameDrawableArena<N>::GameDrawableArena() noexcept : begin(nullptr), end(nullptr)
{
}

template <size_t N> void GameDrawableArena<N>::pushDrawableAddress(IGameDrawable& drawable) noexcept
{
        // Don't register test drawables unless testCanRender is true
        if (drawable.getIsTest() && !drawable.getTestCanRender())
        {
                return;
        }
        uintptr_t ptr = reinterpret_cast<uintptr_t>(&drawable);
        if (this->containsDrawable(ptr))
        {
                return;
        }
        if (!this->insertDrawable(ptr))
        {
                return;
        }
        if (end == nullptr)
        {
                size_t capacity = N;
                void* mem       = GameDrawableArenaRegion::operator new(capacity);
                end             = (GameDrawableArenaRegion*)mem;
                begin           = end;
        }
        while (end->count >= end->capacity && end->next != nullptr)
        {
                end = end->next;
        }
        if (end->count >= end->capacity)
        {
                constexpr size_t capacity = N;
                void* mem                 = GameDrawableArenaRegion::operator new(capacity);
                end->next                 = (GameDrawableArenaRegion*)mem;
                end                       = end->next;
        }
        end->data[end->count] = ptr;
        end->count++;
}

template <size_t N>
void GameDrawableArena<N>::executeDrawCallback(GameDeviceInstance&            device,
                                               GameDrawableArena<N>::CallbackType cb)
{
        for (GameDrawableArenaRegion* r = begin; r != nullptr; r = r->next)
        {
                for (size_t i = 0; i < r->count; ++i)
                {
                        IGameDrawable* drawable = reinterpret_cast<IGameDrawable*>(r->data[i]);
                        switch (cb)
                        {
                        case CallbackType::draw:
                                {
                                        if (drawable->getCanBeRendered() &&
                                            !drawable->getIsCompositing())
                                        {
                                                drawable->draw(device);
                                        }
                                        break;
                                }
                        case CallbackType::setup:
                                {
                                        drawable->setup(device);
                                        break;
                                }
                        case CallbackType::destroy:
                                {
                                        drawable->destroy(device);
                                        break;
                                }
                        case CallbackType::composite:
                                {
                                        if (drawable->getCanBeRendered() &&
                                            drawable->getIsCompositing())
                                        {
                                                drawable->draw(device);
                                        }
                                        break;
                                }
                        }
                }
        }
}

template <size_t N>
void GameDrawableArena<N>::setupDrawCallbacks(GameDeviceInstance& device) noexcept
{
        executeDrawCallback(device, CallbackType::setup);
}

template <size_t N>
void GameDrawableArena<N>::callDrawCallbacks(GameDeviceInstance& device) noexcept
{
        executeDrawCallback(device, CallbackType::draw);
}

template <size_t N>
void GameDrawableArena<N>::callCompositingCallbacks(GameDeviceInstance& device) noexcept
{
        executeDrawCallback(device, CallbackType::composite);
}

template <size_t N>
void GameDrawableArena<N>::destroyDrawCallbacks(GameDeviceInstance& device) noexcept
{
        executeDrawCallback(device, CallbackType::destroy);
}

template <size_t N> GameDrawableArena<N>::~GameDrawableArena() noexcept
{
        for (GameDrawableArenaRegion* r = begin; r != nullptr; r = r->next)
        {
                for (size_t i = 0; i < r->count; ++i)
                {
                        IGameDrawable* drawable = reinterpret_cast<IGameDrawable*>(r->data[i]);
                        delete drawable;
                }
                r->count = 0;
        }
        end = begin;
}

template <size_t N> long GameDrawableArenaTrack<N>::hashFrom(uintptr_t ptr) const noexcept
{
        return ptr % N;
}

template <size_t N> long GameDrawableArenaTrack<N>::findSlot(uintptr_t ptr) const noexcept
{
        size_t index         = this->hashFrom(ptr);
        size_t originalIndex = index;
        while (this->table[index].occupied && this->table[index].key != ptr)
        {
                index = (index + 1) % N;
                if (index == originalIndex)
                {
                        return N;
                }
        }
        return index;
}

template <size_t N> GameDrawableArenaTrack<N>::GameDrawableArenaTrack() noexcept : count(0)
{
        for (size_t i = 0; i < N; ++i)
        {
                table[i].key      = 0;
                table[i].occupied = false;
        }
}

template <size_t N>
bool GameDrawableArenaTrack<N>::containsDrawable(uintptr_t ptr) const noexcept
{
        size_t index = this->findSlot(ptr);
        return index < N && this->table[index].occupied && this->table[index].key == ptr;
}

template <size_t N> bool GameDrawableArenaTrack<N>::insertDrawable(uintptr_t ptr) noexcept
{
        if (containsDrawable(ptr))
        {
                return false;
        }
        if (count >= N)
        {
                return false;
        }
        size_t index = this->hashFrom(ptr);
        while (this->table[index].occupied)
        {
                index = (index + 1) % N;
        }
        this->table[index].key      = ptr;
        this->table[index].occupied = true;
        this->count++;
        return true;
}

template <size_t N> void GameDrawableArenaTrack<N>::clearDrawCallbacks() noexcept
{
        for (size_t i = 0; i < N; ++i)
        {
                this->table[i].key      = 0;
                this->table[i].occupied = false;
        }
        this->count = 0;
}

template class GameDrawableArena<4096>;
template class GameDrawableArena<8192>;
template class GameDrawableArena<16384>;

} // namespace rl
