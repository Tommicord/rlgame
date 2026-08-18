#include "rlgame.base/cstl/cstl_heap_allocator.h"
#include "rlgame.base/cstl/cstl_platform.h"
#include "rlgame.base/cstl/cstl_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#ifdef R_CSTL_HEAP_DEBUG
#if defined(_WIN32) && defined(_MSC_VER)
#include <crtdbg.h>
#endif
#if defined(__SANITIZE_ADDRESS__)
#define R_CSTL_HAVE_ASAN 1
#elif defined(__has_feature)
#if __has_feature(address_sanitizer)
#define R_CSTL_HAVE_ASAN 1
#endif
#endif
#ifdef R_CSTL_HAVE_ASAN
#include <sanitizer/asan_interface.h>
#endif
#define R_CSTL_HEAP_POISON_ENABLED 1
#define R_CSTL_HEAP_POISON_ALLOC   0xCD
#define R_CSTL_HEAP_POISON_FREE    0xDD
#define R_CSTL_HEAP_POISON_REDZONE 0xFD
#else
#define R_CSTL_HEAP_POISON_ENABLED 0
#define R_CSTL_HEAP_POISON_ALLOC   0
#define R_CSTL_HEAP_POISON_FREE    0
#define R_CSTL_HEAP_POISON_REDZONE 0
#endif

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef CRITICAL_SECTION R_CSTL_HeapMutex;

static void
R_CSTL_HeapMutexInit (R_CSTL_HeapMutex* m)
{
        if (m)
                InitializeCriticalSection (m);
}

static void
R_CSTL_HeapMutexLock (R_CSTL_HeapMutex* m)
{
        if (m)
                EnterCriticalSection (m);
}

static void
R_CSTL_HeapMutexUnlock (R_CSTL_HeapMutex* m)
{
        if (m)
                LeaveCriticalSection (m);
}

static void
R_CSTL_HeapMutexDestroy (R_CSTL_HeapMutex* m)
{
        if (m)
                DeleteCriticalSection (m);
}

static size_t
R_CSTL_PlatformPageSize (void)
{
        SYSTEM_INFO si;
        GetSystemInfo (&si);
        return (size_t)si.dwPageSize;
}

static void*
R_CSTL_PlatformHeapMap (size_t size, size_t* pOutMappedSize)
{
        if (!pOutMappedSize || size == 0)
                return NULL;

        size_t page_size = R_CSTL_PlatformPageSize ();
        if (page_size == 0)
                return NULL;

        size_t mapped = (size + page_size - 1) & ~(page_size - 1);
        void*  p = VirtualAlloc (NULL, mapped, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!p)
                return NULL;

        *pOutMappedSize = mapped;
        return p;
}

static void
R_CSTL_PlatformHeapUnmap (void* pData, size_t mappedSize)
{
        (void)mappedSize;
        if (pData)
                VirtualFree (pData, 0, MEM_RELEASE);
}

#else
#include <pthread.h>
#include <sys/mman.h>
#include <unistd.h>

typedef pthread_mutex_t R_CSTL_HeapMutex;

static void
R_CSTL_HeapMutexInit (R_CSTL_HeapMutex* m)
{
        if (m)
                pthread_mutex_init (m, NULL);
}

static void
R_CSTL_HeapMutexLock (R_CSTL_HeapMutex* m)
{
        if (m)
                pthread_mutex_lock (m);
}

static void
R_CSTL_HeapMutexUnlock (R_CSTL_HeapMutex* m)
{
        if (m)
                pthread_mutex_unlock (m);
}

static void
R_CSTL_HeapMutexDestroy (R_CSTL_HeapMutex* m)
{
        if (m)
                pthread_mutex_destroy (m);
}

static size_t
R_CSTL_PlatformPageSize (void)
{
        long page_size = sysconf (_SC_PAGESIZE);
        return (page_size > 0) ? (size_t)page_size : 4096;
}

static void*
R_CSTL_PlatformHeapMap (size_t size, size_t* pOutMappedSize)
{
        if (!pOutMappedSize || size == 0)
                return NULL;

        size_t page_size = R_CSTL_PlatformPageSize ();
        size_t mapped = (size + page_size - 1) & ~(page_size - 1);
        void*  p = mmap (NULL, mapped, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (p == MAP_FAILED)
                return NULL;

        *pOutMappedSize = mapped;
        return p;
}

static void
R_CSTL_PlatformHeapUnmap (void* p, size_t mappedSize)
{
        if (p && mappedSize > 0)
                munmap (p, mappedSize);
}

#endif

// Intrusive free-list node stored inside free buddy blocks (no extra malloc).
struct R_CSTL_FreeNode
{
                struct R_CSTL_FreeNode* pNext;
};

struct R_CSTL_AllocationRecord
{
                void*    pAllocation;
                size_t   size;
                char*    pName;
                uint64_t hash;
                uint64_t allocId;
};

struct R_CSTL_OwnerRecord
{
                void*                           pOwner;
                char*                           pTypeName;
                struct R_CSTL_AllocationRecord* pAllocations;
                size_t                          allocCount;
                size_t                          allocCapacity;
                struct R_CSTL_OwnerRecord*      pNext;
};

struct R_CSTL_BlockHeader
{
                uint32_t magic;
                uint32_t order;
                size_t   requestedSize;
                uint64_t allocId;
};

struct R_CSTL_HeapState
{
                void*                      pHeapBase;
                size_t                     totalSize;
                size_t                     mappedSize;
                size_t                     guardPageOffset;
                size_t                     guardPageSize;
                size_t                     minBlock;
                int                        maxOrder;
                struct R_CSTL_FreeNode**   pFreeLists;
                struct R_CSTL_OwnerRecord* pOwners;
                R_CSTL_HeapMutex           mutex;
                size_t                     usedBytes;
                size_t                     registeredCount;
                uint64_t                   nextAllocId;
                int                        initialized;
};

#define R_CSTL_HEAP_BLOCK_MAGIC 0x4353544CU /* 'CSTL' */
#define R_CSTL_HEAP_FREED_MAGIC 0x46524545U /* 'FREE' */

static struct R_CSTL_HeapState g_heap = {0};

static int
R_CSTL_HeapIsReady (void)
{
        /* Prevent Release/LTO from treating g_heap.initialized as always zero when
         * heap functions are inlined into callers in other translation units. */
        R_CSTL_HEAP_COMPILER_BARRIER ();
        const int ready = (g_heap.initialized != 0 && g_heap.pHeapBase != NULL);
        R_CSTL_HEAP_COMPILER_BARRIER ();
        return ready;
}

static char*
R_CSTL_StrDup (const char* pName)
{
        if (!pName)
                return NULL;
        size_t len = strlen (pName) + 1;
        char*  dup = (char*)malloc (len);
        if (!dup)
                return NULL;
        memcpy (dup, pName, len);
        return dup;
}

static size_t
R_CSTL_NextPow2 (size_t v)
{
        if (v == 0)
                return 1;
        --v;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
#if SIZE_MAX > UINT32_MAX
        v |= v >> 32;
#endif
        return ++v;
}

static int
R_CSTL_SizeToOrder (size_t size)
{
        size_t block = g_heap.minBlock;
        int    order = 0;
        while (block < size)
        {
                block <<= 1;
                ++order;
        }
        return order;
}

static size_t
R_CSTL_OrderToSize (int order)
{
        return g_heap.minBlock << (size_t)order;
}

#if defined(R_CSTL_HAVE_ASAN)
static void
R_CSTL_AsanPoison (const void* addr, size_t size)
{
        if (addr && size > 0)
                __asan_poison_memory_region (addr, size);
}

static void
R_CSTL_AsanUnpoison (const void* addr, size_t size)
{
        if (addr && size > 0)
                __asan_unpoison_memory_region (addr, size);
}
#else
static void
R_CSTL_AsanPoison (const void* addr, size_t size)
{
        (void)addr;
        (void)size;
}

static void
R_CSTL_AsanUnpoison (const void* addr, size_t size)
{
        (void)addr;
        (void)size;
}
#endif

#ifdef R_CSTL_HEAP_DEBUG
static void
R_CSTL_DebugInitRuntimeChecks (void)
{
#if defined(_WIN32) && defined(_MSC_VER)
        int flags = _CrtSetDbgFlag (_CRTDBG_REPORT_FLAG);
        flags |= _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF;
        _CrtSetDbgFlag (flags);
        _CrtSetReportMode (_CRT_WARN, _CRTDBG_MODE_FILE);
        _CrtSetReportMode (_CRT_ERROR, _CRTDBG_MODE_FILE);
        _CrtSetReportMode (_CRT_ASSERT, _CRTDBG_MODE_FILE);
        _CrtSetReportFile (_CRT_WARN, _CRTDBG_FILE_STDERR);
        _CrtSetReportFile (_CRT_ERROR, _CRTDBG_FILE_STDERR);
        _CrtSetReportFile (_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif
}

static int
R_CSTL_DebugInstallGuardPage (void)
{
        if (!g_heap.pHeapBase || g_heap.mappedSize == 0)
                return -1;

        size_t page_size = R_CSTL_PlatformPageSize ();
        if (page_size == 0)
                return -1;

        size_t heap_end = (g_heap.totalSize + page_size - 1) & ~(page_size - 1);
        if (heap_end + page_size > g_heap.mappedSize)
                return -1;

        void* guard = (char*)g_heap.pHeapBase + heap_end;
#if defined(_WIN32)
        DWORD old_protect = 0;
        if (!VirtualProtect (guard, page_size, PAGE_NOACCESS, &old_protect))
                return -1;
#else
        if (mprotect (guard, page_size, PROT_NONE) != 0)
                return -1;
#endif
        g_heap.guardPageOffset = heap_end;
        g_heap.guardPageSize = page_size;
        return 0;
}

static void
R_CSTL_DebugRemoveGuardPage (void)
{
        if (!g_heap.pHeapBase || g_heap.guardPageSize == 0)
                return;

        void* guard = (char*)g_heap.pHeapBase + g_heap.guardPageOffset;
#if defined(_WIN32)
        DWORD old_protect = 0;
        VirtualProtect (guard, g_heap.guardPageSize, PAGE_READWRITE, &old_protect);
#else
        mprotect (guard, g_heap.guardPageSize, PROT_READ | PROT_WRITE);
#endif
        g_heap.guardPageOffset = 0;
        g_heap.guardPageSize = 0;
}

static void
R_CSTL_DebugPoisonBlockRedzone (struct R_CSTL_BlockHeader* pHeader)
{
#if R_CSTL_HEAP_POISON_ENABLED
        if (!pHeader)
                return;

        size_t blockSize = R_CSTL_OrderToSize ((int)pHeader->order);
        char*  user = (char*)pHeader + sizeof (*pHeader);
        size_t userCapacity = blockSize - sizeof (*pHeader);
        size_t requested = pHeader->requestedSize;
        if (requested >= userCapacity)
                return;

        size_t redzone = userCapacity - requested;
        memset (user + requested, R_CSTL_HEAP_POISON_REDZONE, redzone);
        R_CSTL_AsanPoison (user + requested, redzone);
#else
        (void)pHeader;
#endif
}

static void
R_CSTL_DebugPrepareUserBuffer (struct R_CSTL_BlockHeader* pHeader, void* pData)
{
        if (!pHeader || !pData || pHeader->requestedSize == 0)
                return;

#if R_CSTL_HEAP_POISON_ENABLED
        if (R_CSTL_HEAP_POISON_ALLOC)
                memset (pData, R_CSTL_HEAP_POISON_ALLOC, pHeader->requestedSize);
#endif
        R_CSTL_AsanUnpoison (pData, pHeader->requestedSize);
#if R_CSTL_HEAP_POISON_ENABLED
        R_CSTL_DebugPoisonBlockRedzone (pHeader);
#endif
}

static void
R_CSTL_DebugPrepareUserBufferNoPoison (struct R_CSTL_BlockHeader* pHeader, void* pData)
{
        if (!pHeader || !pData)
                return;

        R_CSTL_AsanUnpoison (pData, pHeader->requestedSize);
}

static void
R_CSTL_DebugPoisonFreedUserBuffer (struct R_CSTL_BlockHeader* pHeader, void* pUser)
{
#if R_CSTL_HEAP_POISON_ENABLED
        if (!pHeader || !pUser || pHeader->requestedSize == 0)
                return;

        if (R_CSTL_HEAP_POISON_FREE)
                memset (pUser, R_CSTL_HEAP_POISON_FREE, pHeader->requestedSize);
        R_CSTL_AsanPoison (pUser, pHeader->requestedSize);
#else
        (void)pHeader;
        (void)pUser;
#endif
}
#else
static void
R_CSTL_DebugInitRuntimeChecks (void)
{
}

static int
R_CSTL_DebugInstallGuardPage (void)
{
        return 0;
}

static void
R_CSTL_DebugDestroyGuardPage (void)
{
}

static void
R_CSTL_DebugPoisonBlockRedzone (struct R_CSTL_BlockHeader* pHeader)
{
        (void)pHeader;
}

static void
R_CSTL_DebugRemoveGuardPage (void)
{
}

static void
R_CSTL_DebugPrepareUserBuffer (struct R_CSTL_BlockHeader* pHeader, void* pData)
{
        (void)pHeader;
        (void)pData;
}

static void
R_CSTL_DebugPrepareUserBufferNoPoison (struct R_CSTL_BlockHeader* pHeader, void* pData)
{
        (void)pHeader;
        (void)pData;
}

static void
R_CSTL_DebugPoisonFreedUserBuffer (struct R_CSTL_BlockHeader* pHeader, void* pUser)
{
        (void)pHeader;
        (void)pUser;
}
#endif

static struct R_CSTL_BlockHeader*
R_CSTL_HeaderFromUserData (const void* ptr)
{
        if (!ptr)
                return NULL;
        return (struct R_CSTL_BlockHeader*)((const char*)ptr - sizeof (struct R_CSTL_BlockHeader));
}

static int
R_CSTL_HeapPointerInRange (const void* ptr)
{
        if (!ptr || !g_heap.pHeapBase || g_heap.totalSize == 0)
                return 0;
        const char* base = (const char*)g_heap.pHeapBase;
        const char* end = base + g_heap.totalSize;
        const char* p = (const char*)ptr;
        return (p >= base && p < end);
}

static int
R_CSTL_HeaderInRange (const struct R_CSTL_BlockHeader* pHeader)
{
        if (!pHeader || !g_heap.pHeapBase || g_heap.totalSize < sizeof (*pHeader))
                return 0;
        const char* base = (const char*)g_heap.pHeapBase;
        const char* end = base + g_heap.totalSize - sizeof (*pHeader) + 1;
        const char* p = (const char*)pHeader;
        return (p >= base && p < end);
}

static int
R_CSTL_IsLiveBlockHeader (const struct R_CSTL_BlockHeader* pHeader)
{
        if (!R_CSTL_HeaderInRange (pHeader))
                goto cstl_fail;
        if (pHeader->magic != R_CSTL_HEAP_BLOCK_MAGIC)
                goto cstl_fail;
        if (pHeader->order > (uint32_t)g_heap.maxOrder)
                goto cstl_fail;
        size_t offset = (size_t)((const char*)pHeader - (const char*)g_heap.pHeapBase);
        if (offset % g_heap.minBlock != 0)
                goto cstl_fail;
        size_t blockSize = R_CSTL_OrderToSize ((int)pHeader->order);
        if (offset + blockSize > g_heap.totalSize)
                goto cstl_fail;
        return 1;
cstl_fail:
        return 0;
}

static struct R_CSTL_FreeNode*
R_CSTL_FreeNodeFromOffset (size_t offset)
{
        return (struct R_CSTL_FreeNode*)((char*)g_heap.pHeapBase + offset);
}

static size_t
R_CSTL_OffsetFromFreeNode (const struct R_CSTL_FreeNode* pNode)
{
        return (size_t)((const char*)pNode - (const char*)g_heap.pHeapBase);
}

static void
R_CSTL_PushFreeLocked (int order, size_t offset)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (order < 0 || order > g_heap.maxOrder)
                return;
#endif
        struct R_CSTL_FreeNode* pNode = R_CSTL_FreeNodeFromOffset (offset);
        pNode->pNext = g_heap.pFreeLists[order];
        g_heap.pFreeLists[order] = pNode;
}

static void
R_CSTL_PushFree (int order, size_t offset)
{
        R_CSTL_HeapMutexLock (&g_heap.mutex);
        R_CSTL_PushFreeLocked (order, offset);
        R_CSTL_HeapMutexUnlock (&g_heap.mutex);
}

static int
R_CSTL_PopFreeLocked (int order, size_t* pOutOffset)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pOutOffset || order < 0 || order > g_heap.maxOrder)
                goto cstl_fail;
#endif

        struct R_CSTL_FreeNode* pHead = g_heap.pFreeLists[order];
        if (!pHead)
                goto cstl_fail;

        g_heap.pFreeLists[order] = pHead->pNext;
        *pOutOffset = R_CSTL_OffsetFromFreeNode (pHead);
        return 1;
cstl_fail:
        return 0;
}

static int
R_CSTL_PopFree (int order, size_t* pOutOffset)
{
        R_CSTL_HeapMutexLock (&g_heap.mutex);
        const int found = R_CSTL_PopFreeLocked (order, pOutOffset);
        R_CSTL_HeapMutexUnlock (&g_heap.mutex);
        return found;
}

static int
R_CSTL_RemoveFreeOffsetLocked (int order, size_t offset)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (order < 0 || order > g_heap.maxOrder)
                goto cstl_fail;
#endif
        struct R_CSTL_FreeNode* pPrev = NULL;
        struct R_CSTL_FreeNode* pCur = g_heap.pFreeLists[order];
        while (pCur)
        {
                if (R_CSTL_OffsetFromFreeNode (pCur) == offset)
                {
                        if (pPrev)
                                pPrev->pNext = pCur->pNext;
                        else
                                g_heap.pFreeLists[order] = pCur->pNext;
                        return 1;
                }
                pPrev = pCur;
                pCur = pCur->pNext;
        }
        return 0;
cstl_fail:
        return 0;
}

static int
R_CSTL_RemoveFreeOffset (int order, size_t offset)
{
        R_CSTL_HeapMutexLock (&g_heap.mutex);
        const int removed = R_CSTL_RemoveFreeOffsetLocked (order, offset);
        R_CSTL_HeapMutexUnlock (&g_heap.mutex);
        return removed;
}

static void
R_CSTL_RemoveAllocationRecord (void* pAllocation)
{
        if (!pAllocation)
                return;
        for (struct R_CSTL_OwnerRecord* pOwner = g_heap.pOwners; pOwner; pOwner = pOwner->pNext)
        {
                for (size_t i = 0; i < pOwner->allocCount;)
                {
                        if (pOwner->pAllocations[i].pAllocation == pAllocation)
                        {
                                free (pOwner->pAllocations[i].pName);
                                pOwner->pAllocations[i] = pOwner->pAllocations[pOwner->allocCount - 1];
                                --pOwner->allocCount;
                                if (g_heap.registeredCount > 0)
                                        --g_heap.registeredCount;
                        }
                        else
                        {
                                ++i;
                        }
                }
        }
}

int R_CSTL_API
R_CSTL_HeapInit (size_t heap_size_bytes)
{
        if (g_heap.initialized)
                return 0;
        if (heap_size_bytes == 0)
                return -1;

        g_heap.minBlock = 32;
        size_t usable = R_CSTL_NextPow2 (heap_size_bytes);
        if (usable < g_heap.minBlock)
                usable = g_heap.minBlock;
        if (usable % g_heap.minBlock)
                usable = R_CSTL_NextPow2 (usable);

        size_t mappedSize = 0;
        size_t mapRequest = usable;
#ifdef R_CSTL_HEAP_DEBUG
        size_t guard_page = R_CSTL_PlatformPageSize ();
        if (guard_page > 0)
                mapRequest = usable + guard_page;
#endif
        void* base = R_CSTL_PlatformHeapMap (mapRequest, &mappedSize);
        if (!base || mappedSize < usable)
        {
                R_CSTL_PlatformHeapUnmap (base, mappedSize);
                return -1;
        }
        g_heap.pHeapBase = base;
        g_heap.totalSize = usable;
        g_heap.mappedSize = mappedSize;
        g_heap.usedBytes = 0;
        g_heap.nextAllocId = 1;

        size_t blocks = g_heap.totalSize / g_heap.minBlock;
        int    order = 0;
        while ((1ULL << order) < blocks)
                ++order;
        g_heap.maxOrder = order;

        g_heap.pFreeLists = (struct R_CSTL_FreeNode**)calloc (
            (size_t)g_heap.maxOrder + 1,
            sizeof (struct R_CSTL_FreeNode*));
        if (!g_heap.pFreeLists)
        {
                R_CSTL_PlatformHeapUnmap (g_heap.pHeapBase, g_heap.mappedSize);
                memset (&g_heap, 0, sizeof (g_heap));
                return -1;
        }

        R_CSTL_HeapMutexInit (&g_heap.mutex);
        R_CSTL_HeapMutexLock (&g_heap.mutex);
        R_CSTL_PushFreeLocked (g_heap.maxOrder, 0);
        R_CSTL_HeapMutexUnlock (&g_heap.mutex);
        g_heap.initialized = 1;
        R_CSTL_HEAP_COMPILER_BARRIER ();

        R_CSTL_DebugInitRuntimeChecks ();
        if (R_CSTL_DebugInstallGuardPage () != 0)
        {
                R_CSTL_HeapShutdown ();
                return -1;
        }

        return 0;
}

void R_CSTL_API
R_CSTL_HeapShutdown (void)
{
        if (!R_CSTL_HeapIsReady ())
                return;

        R_CSTL_HeapLogLeaks ();

        for (int i = 0; i <= g_heap.maxOrder; ++i)
                g_heap.pFreeLists[i] = NULL;
        free (g_heap.pFreeLists);

        struct R_CSTL_OwnerRecord* pOwner = g_heap.pOwners;
        while (pOwner)
        {
                struct R_CSTL_OwnerRecord* pNextOwner = pOwner->pNext;
                for (size_t i = 0; i < pOwner->allocCount; ++i)
                        free (pOwner->pAllocations[i].pName);
                free (pOwner->pAllocations);
                free (pOwner->pTypeName);
                free (pOwner);
                pOwner = pNextOwner;
        }

        R_CSTL_HeapMutexDestroy (&g_heap.mutex);
        R_CSTL_DebugRemoveGuardPage ();
        R_CSTL_PlatformHeapUnmap (g_heap.pHeapBase, g_heap.mappedSize);
        memset (&g_heap, 0, sizeof (g_heap));
        R_CSTL_HEAP_COMPILER_BARRIER ();
}

R_CSTL_API void*
R_CSTL_HeapAlloc (size_t size)
{
        if (!R_CSTL_HeapIsReady ())
                return NULL;
#if defined(R_CSTL_HEAP_DEBUG)
        if (size == 0)
                return NULL;
#endif
        size_t total = size + sizeof (struct R_CSTL_BlockHeader);
        int    target = R_CSTL_SizeToOrder (total <= g_heap.minBlock ? g_heap.minBlock : total);
        if (target > g_heap.maxOrder)
                return NULL;

        int    order = target;
        size_t offset = 0;

        R_CSTL_HeapMutexLock (&g_heap.mutex);

        while (order <= g_heap.maxOrder)
        {
                if (R_CSTL_PopFreeLocked (order, &offset))
                        break;
                ++order;
        }
        if (order > g_heap.maxOrder)
        {
                R_CSTL_HeapMutexUnlock (&g_heap.mutex);
                return NULL;
        }

        while (order > target)
        {
                --order;
                size_t sizeBlock = R_CSTL_OrderToSize (order);
                R_CSTL_PushFreeLocked (order, offset + sizeBlock);
        }

        const uint64_t allocId = g_heap.nextAllocId++;
        g_heap.usedBytes += R_CSTL_OrderToSize (order);
        R_CSTL_HeapMutexUnlock (&g_heap.mutex);

        struct R_CSTL_BlockHeader* pHeader = (struct R_CSTL_BlockHeader*)((char*)g_heap.pHeapBase + offset);
        pHeader->magic = R_CSTL_HEAP_BLOCK_MAGIC;
        pHeader->order = (uint32_t)order;
        pHeader->requestedSize = size;
        pHeader->allocId = allocId;

        void* pUser = (void*)((char*)pHeader + sizeof (struct R_CSTL_BlockHeader));
        R_CSTL_DebugPrepareUserBuffer (pHeader, pUser);
        return pUser;
}

static void*
R_CSTL_HeapAllocNoPoison (size_t size)
{
        if (!R_CSTL_HeapIsReady ())
                return NULL;
#if defined(R_CSTL_HEAP_DEBUG)
        if (size == 0)
                return NULL;
#endif
        size_t total = size + sizeof (struct R_CSTL_BlockHeader);
        int    target = R_CSTL_SizeToOrder (total <= g_heap.minBlock ? g_heap.minBlock : total);
        if (target > g_heap.maxOrder)
                return NULL;

        int    order = target;
        size_t offset = 0;

        R_CSTL_HeapMutexLock (&g_heap.mutex);

        while (order <= g_heap.maxOrder)
        {
                if (R_CSTL_PopFreeLocked (order, &offset))
                        break;
                ++order;
        }

        if (order > g_heap.maxOrder)
        {
                R_CSTL_HeapMutexUnlock (&g_heap.mutex);
                return NULL;
        }

        while (order > target)
        {
                --order;
                size_t sizeBlock = R_CSTL_OrderToSize (order);
                R_CSTL_PushFreeLocked (order, offset + sizeBlock);
        }

        const uint64_t allocId = g_heap.nextAllocId++;
        g_heap.usedBytes += R_CSTL_OrderToSize (order);
        R_CSTL_HeapMutexUnlock (&g_heap.mutex);

        struct R_CSTL_BlockHeader* pHeader = (struct R_CSTL_BlockHeader*)((char*)g_heap.pHeapBase + offset);
        pHeader->magic = R_CSTL_HEAP_BLOCK_MAGIC;
        pHeader->order = (uint32_t)order;
        pHeader->requestedSize = size;
        pHeader->allocId = allocId;

        void* pUser = (void*)((char*)pHeader + sizeof (struct R_CSTL_BlockHeader));
        R_CSTL_DebugPrepareUserBufferNoPoison (pHeader, pUser);
        return pUser;
}

R_CSTL_API void*
R_CSTL_HeapAllocAligned (size_t size, size_t alignment)
{
        if (!R_CSTL_HeapIsReady ())
                return NULL;
#if defined(R_CSTL_HEAP_DEBUG)
        if (size == 0)
                return NULL;
#endif
        if (alignment == 0 || (alignment & (alignment - 1)) != 0)
                return NULL; // alignment must be power of two
        size_t total = size + alignment + sizeof (void*);
        void*  pRaw = R_CSTL_HeapAlloc (total);
        if (!pRaw)
                return NULL;

        uintptr_t addr = (uintptr_t)pRaw + sizeof (void*);
        uintptr_t aligned = (addr + alignment - 1) & ~(alignment - 1);

        void** pOriginalPtr = (void**)(aligned - sizeof (void*));
        *pOriginalPtr = pRaw;
        return (void*)aligned;
}

void R_CSTL_API
R_CSTL_HeapFree (void* pData)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pData)
                return;
#endif
        if (!R_CSTL_HeapIsReady ())
        {
                R_CSTL_LOG_ERROR ("R_CSTL_HeapFree: heap not initialized");
                return;
        }
        // Check if this might be an aligned allocation by checking if pointer is in range
        // If not, it might be an aligned allocation so retrieve original pointer
        if (!R_CSTL_HeapPointerInRange (pData))
        {
                // Try to retrieve original pointer from aligned allocation
                void** pOriginalPtr = (void**)((char*)pData - sizeof (void*));
                pData = *pOriginalPtr;
                if (!pData || !R_CSTL_HeapPointerInRange (pData))
                {
                        R_CSTL_LOG_ERROR ("R_CSTL_HeapFree: pointer %p outside heap range", pData);
                        return;
                }
        }

        if (!R_CSTL_HeapPointerInRange (pData))
        {
                R_CSTL_LOG_ERROR ("R_CSTL_HeapFree: pointer %p outside heap range", pData);
                return;
        }
        struct R_CSTL_BlockHeader* pHeader = R_CSTL_HeaderFromUserData (pData);
        if (!R_CSTL_HeaderInRange (pHeader))
        {
                R_CSTL_LOG_ERROR ("R_CSTL_HeapFree: invalid header for pointer %p", pData);
                return;
        }
        if (pHeader->magic == R_CSTL_HEAP_FREED_MAGIC)
        {
                R_CSTL_LOG_ERROR (
                    "R_CSTL_HeapFree: double free detected for %p (allocId=0x%016" PRIx64 ")\n",
                    pData,
                    pHeader->allocId);
                return;
        }
        if (pHeader->magic != R_CSTL_HEAP_BLOCK_MAGIC)
        {
                R_CSTL_LOG_ERROR ("R_CSTL_HeapFree: invalid or corrupted pointer %p", pData);
                return;
        }

        int order = (int)pHeader->order;
        if (order < 0 || order > g_heap.maxOrder)
        {
                R_CSTL_LOG_ERROR ("R_CSTL_HeapFree: invalid order %d for ptr %p", order, pData);
                return;
        }

        size_t offset = (size_t)((char*)pHeader - (char*)g_heap.pHeapBase);
        if (offset % g_heap.minBlock != 0)
        {
                R_CSTL_LOG_ERROR ("R_CSTL_HeapFree: misaligned block at %p", pData);
                return;
        }

        size_t blockSize = R_CSTL_OrderToSize (order);
        if (offset + blockSize > g_heap.totalSize)
        {
                R_CSTL_LOG_ERROR ("R_CSTL_HeapFree: block extends past heap for %p", pData);
                return;
        }

        R_CSTL_DebugPoisonFreedUserBuffer (pHeader, pData);
        pHeader->magic = R_CSTL_HEAP_FREED_MAGIC;

        size_t curOffset = offset;
        int    curOrder = order;

        R_CSTL_HeapMutexLock (&g_heap.mutex);
#ifdef R_CSTL_HEAP_DEBUG
        R_CSTL_RemoveAllocationRecord (pData);
#endif
        while (curOrder < g_heap.maxOrder)
        {
                size_t buddy = curOffset ^ R_CSTL_OrderToSize (curOrder);
                if (R_CSTL_RemoveFreeOffsetLocked (curOrder, buddy))
                {
                        curOffset = (curOffset < buddy) ? curOffset : buddy;
                        ++curOrder;
                        continue;
                }
                break;
        }

        R_CSTL_PushFreeLocked (curOrder, curOffset);
        g_heap.usedBytes -= blockSize;
        R_CSTL_HeapMutexUnlock (&g_heap.mutex);
}

R_CSTL_API void*
R_CSTL_HeapRealloc (void* pData, size_t newSize)
{
        if (!R_CSTL_HeapIsReady ())
                return NULL;
        if (!pData)
                return R_CSTL_HeapAlloc (newSize);
        if (newSize == 0)
        {
                R_CSTL_HeapFree (pData);
                return NULL;
        }

#ifdef R_CSTL_HEAP_DEBUG
        if (!R_CSTL_HeapIsValidPointer (pData))
                return NULL;
#endif

        struct R_CSTL_BlockHeader* pHeader = R_CSTL_HeaderFromUserData (pData);
#ifdef R_CSTL_HEAP_DEBUG
        if (!R_CSTL_IsLiveBlockHeader (pHeader))
                return NULL;
#endif

        size_t totalNeeded = newSize + sizeof (struct R_CSTL_BlockHeader);
        int targetOrder = R_CSTL_SizeToOrder (totalNeeded <= g_heap.minBlock ? g_heap.minBlock : totalNeeded);
        int currentOrder = (int)pHeader->order;

        if (targetOrder == currentOrder)
        {
                pHeader->requestedSize = newSize;
                R_CSTL_DebugPrepareUserBuffer (pHeader, pData);
                return pData;
        }

        if (targetOrder < currentOrder)
        {
                pHeader->requestedSize = newSize;
                R_CSTL_DebugPrepareUserBuffer (pHeader, pData);
                return pData;
        }

        void* pNew = R_CSTL_HeapAllocNoPoison (newSize);
        if (!pNew)
                return NULL;

        size_t copyBytes = pHeader->requestedSize;
        if (copyBytes > newSize)
                copyBytes = newSize;
        if (copyBytes > 0)
                memcpy (pNew, pData, copyBytes);

        R_CSTL_HeapFree (pData);

        struct R_CSTL_BlockHeader* pNewHeader = R_CSTL_HeaderFromUserData (pNew);
        R_CSTL_DebugPoisonBlockRedzone (pNewHeader);

        return pNew;
}

static struct R_CSTL_OwnerRecord*
R_CSTL_FindOrCreateHeapOwner (void* pOwner)
{
        if (!pOwner)
                return NULL;

        R_CSTL_HeapMutexLock (&g_heap.mutex);
        for (struct R_CSTL_OwnerRecord* pRecord = g_heap.pOwners; pRecord; pRecord = pRecord->pNext)
        {
                if (pRecord->pOwner == pOwner)
                {
                        R_CSTL_HeapMutexUnlock (&g_heap.mutex);
                        return pRecord;
                }
        }

        struct R_CSTL_OwnerRecord* pNew
            = (struct R_CSTL_OwnerRecord*)malloc (sizeof (struct R_CSTL_OwnerRecord));
        if (!pNew)
        {
                R_CSTL_LOG_ERROR ("R_CSTL_FindOrCreateHeapOwner: metadata allocation failed");
                R_CSTL_HeapMutexUnlock (&g_heap.mutex);
                return NULL;
        }
        memset (pNew, 0, sizeof (*pNew));
        pNew->pOwner = pOwner;
        pNew->pNext = g_heap.pOwners;
        g_heap.pOwners = pNew;
        R_CSTL_HeapMutexUnlock (&g_heap.mutex);
        return pNew;
}

static uint64_t
R_CSTL_Heap_ComputeHash (const void* a, const void* b, size_t size, const char* pName)
{
        uint64_t             h = 14695981039346656037ULL;
        const unsigned char* p = (const unsigned char*)&a;
        for (size_t i = 0; i < sizeof (a); ++i)
        {
                h ^= p[i];
                h *= 1099511628211ULL;
        }
        p = (const unsigned char*)&b;
        for (size_t i = 0; i < sizeof (b); ++i)
        {
                h ^= p[i];
                h *= 1099511628211ULL;
        }
        unsigned char buf[8];
        for (int i = 0; i < 8; i++)
                buf[i] = (unsigned char)((size >> (i * 8)) & 0xff);
        p = buf;
        for (size_t i = 0; i < 8; ++i)
        {
                h ^= p[i];
                h *= 1099511628211ULL;
        }
        if (pName)
        {
                const unsigned char* s = (const unsigned char*)pName;
                while (*s)
                {
                        h ^= *s++;
                        h *= 1099511628211ULL;
                }
        }
        return h;
}

uint64_t
R_CSTL_HeapRegisterAllocation (void* pOwner, void* pAllocation, size_t size, const char* pName)
{
#ifndef R_CSTL_HEAP_DEBUG
        (void)pOwner;
        (void)pAllocation;
        (void)size;
        (void)pName;
        return 1;
#else
        if (!pOwner || !pAllocation || size == 0)
                return 0;
        if (!g_heap.initialized || !g_heap.pHeapBase)
                return 0;
        if (!R_CSTL_HeapIsValidPointer (pAllocation))
        {
                R_CSTL_LOG_ERROR (
                    "R_CSTL_HeapRegisterAllocation: %p is not a live heap allocation",
                    pAllocation);
                return 0;
        }
        struct R_CSTL_BlockHeader* pHeader = R_CSTL_HeaderFromUserData (pAllocation);
        if (pHeader->requestedSize != size)
        {
                R_CSTL_LOG_ERROR (
                    "R_CSTL_HeapRegisterAllocation: size mismatch for %p "
                    "(expected %zu, got %zu)\n",
                    pAllocation,
                    pHeader->requestedSize,
                    size);
                return 0;
        }

        struct R_CSTL_OwnerRecord* pOwnerRecord = R_CSTL_FindOrCreateHeapOwner (pOwner);
        if (!pOwnerRecord)
                return 0;

        R_CSTL_HeapMutexLock (&g_heap.mutex);
        if (pOwnerRecord->allocCount == pOwnerRecord->allocCapacity)
        {
                size_t ncap = pOwnerRecord->allocCapacity ? pOwnerRecord->allocCapacity * 2 : 8;
                struct R_CSTL_AllocationRecord* n = (struct R_CSTL_AllocationRecord*)realloc (
                    pOwnerRecord->pAllocations,
                    ncap * sizeof (struct R_CSTL_AllocationRecord));
                if (!n)
                {
                        R_CSTL_HeapMutexUnlock (&g_heap.mutex);
                        R_CSTL_LOG_DEBUG ("R_CSTL_HeapRegisterAllocation: metadata realloc failed");
                        return 0;
                }
                pOwnerRecord->pAllocations = n;
                pOwnerRecord->allocCapacity = ncap;
        }

        struct R_CSTL_AllocationRecord* pRecord = &pOwnerRecord->pAllocations[pOwnerRecord->allocCount++];
        pRecord->pAllocation = pAllocation;
        pRecord->size = size;
        pRecord->pName = R_CSTL_StrDup (pName);
        if (pName && !pRecord->pName)
        {
                --pOwnerRecord->allocCount;
                R_CSTL_HeapMutexUnlock (&g_heap.mutex);
                R_CSTL_LOG_ERROR ("R_CSTL_HeapRegisterAllocation: name strdup failed");
                return 0;
        }
        pRecord->allocId = pHeader->allocId;
        pRecord->hash = R_CSTL_Heap_ComputeHash (pOwner, pAllocation, size, pName);
        ++g_heap.registeredCount;
        R_CSTL_HeapMutexUnlock (&g_heap.mutex);
        return pRecord->hash;
#endif
}

void
R_CSTL_HeapUnregisterAllocation (void* pOwner, void* pAllocation)
{
#ifndef R_CSTL_HEAP_DEBUG
        (void)pOwner;
        (void)pAllocation;
        return;
#else
        if (!pOwner || !pAllocation)
                return;
        if (!g_heap.initialized)
                return;

        R_CSTL_HeapMutexLock (&g_heap.mutex);
        for (struct R_CSTL_OwnerRecord* pOwnerRecord = g_heap.pOwners; pOwnerRecord;
             pOwnerRecord = pOwnerRecord->pNext)
        {
                if (pOwnerRecord->pOwner != pOwner)
                        continue;
                for (size_t i = 0; i < pOwnerRecord->allocCount; ++i)
                {
                        if (pOwnerRecord->pAllocations[i].pAllocation != pAllocation)
                                continue;
                        free (pOwnerRecord->pAllocations[i].pName);
                        pOwnerRecord->pAllocations[i]
                            = pOwnerRecord->pAllocations[pOwnerRecord->allocCount - 1];
                        --pOwnerRecord->allocCount;
                        if (g_heap.registeredCount > 0)
                                --g_heap.registeredCount;
                        break;
                }
                break;
        }
        R_CSTL_HeapMutexUnlock (&g_heap.mutex);
#endif
}

size_t
R_CSTL_HeapCheckObjectLeaks (void* pOwner)
{
        if (!pOwner || !g_heap.initialized)
                return 0;

        R_CSTL_HeapMutexLock (&g_heap.mutex);
        size_t leaked = 0;
        for (struct R_CSTL_OwnerRecord* pOwnerRecord = g_heap.pOwners; pOwnerRecord;
             pOwnerRecord = pOwnerRecord->pNext)
        {
                if (pOwnerRecord->pOwner != pOwner)
                        continue;
                for (size_t i = 0; i < pOwnerRecord->allocCount; ++i)
                {
                        struct R_CSTL_AllocationRecord* ar = &pOwnerRecord->pAllocations[i];
                        int stillLive = R_CSTL_HeapIsValidPointer (ar->pAllocation);
                        R_CSTL_LOG_ERROR (
                            "Heap Leak: owner=%p alloc=%p size=%zu name=%s "
                            "allocId=0x%016" PRIx64 " hash=0x%016" PRIx64 " live=%d",
                            pOwner,
                            ar->pAllocation,
                            ar->size,
                            ar->pName ? ar->pName : "(null)",
                            ar->allocId,
                            ar->hash,
                            stillLive);
                        ++leaked;
                }
                break;
        }
        R_CSTL_HeapMutexUnlock (&g_heap.mutex);
        return leaked;
}

size_t
R_CSTL_HeapLogLeaks (void)
{
        if (!g_heap.initialized)
                return 0;

        R_CSTL_HeapMutexLock (&g_heap.mutex);
        size_t total = 0;
        size_t bytes = 0;
        for (struct R_CSTL_OwnerRecord* pOwnerRecord = g_heap.pOwners; pOwnerRecord;
             pOwnerRecord = pOwnerRecord->pNext)
        {
                if (pOwnerRecord->allocCount == 0)
                        continue;
                R_CSTL_LOG_ERROR (
                    "Owner %p (type=%s) has %zu leaked allocations:",
                    pOwnerRecord->pOwner,
                    pOwnerRecord->pTypeName ? pOwnerRecord->pTypeName : "(unknown)",
                    pOwnerRecord->allocCount);
                for (size_t i = 0; i < pOwnerRecord->allocCount; ++i)
                {
                        struct R_CSTL_AllocationRecord* ar = &pOwnerRecord->pAllocations[i];
                        int                             stillLive
                            = R_CSTL_IsLiveBlockHeader (R_CSTL_HeaderFromUserData (ar->pAllocation));

                        R_CSTL_LOG_ERROR (
                            "alloc=%p size=%zu name=%s allocId=0x%016" PRIx64 "hash=0x%016" PRIx64 " live=%d",
                            ar->pAllocation,
                            ar->size,
                            ar->pName ? ar->pName : "(null)",
                            ar->allocId,
                            ar->hash,
                            stillLive);
                        ++total;
                        bytes += ar->size;
                }
        }
        if (total > 0)
        {
                R_CSTL_LOG_ERROR (
                    "R_CSTL heap leak summary: %zu allocations, %zu bytes tracked",
                    total,
                    bytes);
        }
        R_CSTL_HeapMutexUnlock (&g_heap.mutex);
        return total;
}

int
R_CSTL_HeapIsValidPointer (const void* ptr)
{
        if (!g_heap.initialized || !ptr)
                return 0;
        if (!R_CSTL_HeapPointerInRange (ptr))
                return 0;
#ifdef R_CSTL_HEAP_DEBUG
        return R_CSTL_IsLiveBlockHeader (R_CSTL_HeaderFromUserData (ptr));
#else
        const struct R_CSTL_BlockHeader* pHeader = R_CSTL_HeaderFromUserData (ptr);
        return pHeader->magic == R_CSTL_HEAP_BLOCK_MAGIC;
#endif
}

size_t
R_CSTL_HeapGetRegisteredCount (void)
{
        if (!g_heap.initialized)
                return 0;
        R_CSTL_HeapMutexLock (&g_heap.mutex);
        size_t count = g_heap.registeredCount;
        R_CSTL_HeapMutexUnlock (&g_heap.mutex);
        return count;
}

size_t
R_CSTL_Heap_GetTotalSize (void)
{
        return g_heap.totalSize;
}

size_t
R_CSTL_Heap_GetUsedSize (void)
{
        if (!g_heap.initialized)
                return 0;
        R_CSTL_HeapMutexLock (&g_heap.mutex);
        size_t used = g_heap.usedBytes;
        R_CSTL_HeapMutexUnlock (&g_heap.mutex);
        return used;
}

#ifdef R_CSTL_HEAP_DEBUG
int
R_CSTL_HeapDebugVerify (void)
{
        if (!g_heap.initialized || !g_heap.pHeapBase)
                return 1;
        if (g_heap.usedBytes > g_heap.totalSize)
                return 2;
#if defined(_WIN32) && defined(_MSC_VER)
        if (!_CrtCheckMemory ())
                return 3;
#endif
        return 0;
}
#endif
