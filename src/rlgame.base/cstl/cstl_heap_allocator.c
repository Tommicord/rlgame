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

typedef CRITICAL_SECTION r_cstl_heap_mutex;

static void
r_cstl_heap_new_mutex (r_cstl_heap_mutex* m)
{
    if (m) InitializeCriticalSection (m);
}

static void
r_cstl_heap_mutex_lock (r_cstl_heap_mutex* m)
{
    if (m) EnterCriticalSection (m);
}

static void
r_cstl_heap_mutex_unlock (r_cstl_heap_mutex* m)
{
    if (m) LeaveCriticalSection (m);
}

static void
r_cstl_heap_mutex_destroy (r_cstl_heap_mutex* m)
{
    if (m) DeleteCriticalSection (m);
}

static size_t
r_cstl_platform_page_size (void)
{
    SYSTEM_INFO si;
    GetSystemInfo (&si);
    return (size_t)si.dwPageSize;
}

static void*
r_cstl_platform_heap_map (size_t size, size_t* pOutMappedSize)
{
    if (!pOutMappedSize || size == 0) return NULL;

    size_t page_size = r_cstl_platform_page_size ();
    if (page_size == 0) return NULL;

    size_t mapped = (size + page_size - 1) & ~(page_size - 1);
    void*  p = VirtualAlloc (NULL, mapped, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!p) return NULL;

    *pOutMappedSize = mapped;
    return p;
}

static void
r_cstl_platform_heap_unmap (void* pData, size_t mappedSize)
{
    (void)mappedSize;
    if (pData) VirtualFree (pData, 0, MEM_RELEASE);
}

#else
#include <pthread.h>
#include <sys/mman.h>
#include <unistd.h>

typedef pthread_mutex_t r_cstl_heap_mutex;

static void
r_cstl_heap_new_mutex (r_cstl_heap_mutex* m)
{
    if (m) pthread_mutex_init (m, NULL);
}

static void
r_cstl_heap_mutex_lock (r_cstl_heap_mutex* m)
{
    if (m) pthread_mutex_lock (m);
}

static void
r_cstl_heap_mutex_unlock (r_cstl_heap_mutex* m)
{
    if (m) pthread_mutex_unlock (m);
}

static void
r_cstl_heap_mutex_destroy (r_cstl_heap_mutex* m)
{
    if (m) pthread_mutex_destroy (m);
}

static size_t
r_cstl_platform_page_size (void)
{
    long page_size = sysconf (_SC_PAGESIZE);
    return (page_size > 0) ? (size_t)page_size : 4096;
}

static void*
r_cstl_platform_heap_map (size_t size, size_t* pOutMappedSize)
{
    if (!pOutMappedSize || size == 0) return NULL;

    size_t page_size = r_cstl_platform_page_size ();
    size_t mapped = (size + page_size - 1) & ~(page_size - 1);
    void*  p = mmap (NULL, mapped, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) return NULL;

    *pOutMappedSize = mapped;
    return p;
}

static void
r_cstl_platform_heap_unmap (void* p, size_t mappedSize)
{
    if (p && mappedSize > 0) munmap (p, mappedSize);
}

#endif

// Intrusive free-list node stored inside free buddy blocks (no extra malloc).
struct r_cstl_free_node
{
        struct r_cstl_free_node* pNext;
};

struct r_cstl_allocation_record
{
        void*    pAllocation;
        size_t   size;
        char*    pName;
        uint64_t hash;
        uint64_t allocId;
};

struct r_cstl_owner_record
{
        void*                           pOwner;
        char*                           pTypeName;
        struct r_cstl_allocation_record* pAllocations;
        size_t                          allocCount;
        size_t                          allocCapacity;
        struct r_cstl_owner_record*      pNext;
};

struct r_cstl_block_header
{
        uint32_t magic;
        uint32_t order;
        size_t   requestedSize;
        uint64_t allocId;
};

struct r_cstl_heap_state
{
        void*                      pHeapBase;
        size_t                     totalSize;
        size_t                     mappedSize;
        size_t                     guardPageOffset;
        size_t                     guardPageSize;
        size_t                     minBlock;
        int                        maxOrder;
        struct r_cstl_free_node**   pFreeLists;
        struct r_cstl_owner_record* pOwners;
        r_cstl_heap_mutex           mutex;
        size_t                     usedBytes;
        size_t                     registeredCount;
        uint64_t                   nextAllocId;
        int                        initialized;
};

#define R_CSTL_HEAP_BLOCK_MAGIC 0x4353544CU /* 'CSTL' */
#define R_CSTL_HEAP_FREED_MAGIC 0x46524545U /* 'FREE' */

static struct r_cstl_heap_state g_heap = {0};

static int
r_cstl_heap_is_ready (void)
{
    /* Prevent Release/LTO from treating g_heap.initialized as always zero when
     * heap functions are inlined into callers in other translation units. */
    R_CSTL_HEAP_COMPILER_BARRIER ();
    const int ready = (g_heap.initialized != 0 && g_heap.pHeapBase);
    R_CSTL_HEAP_COMPILER_BARRIER ();
    return ready;
}

static char*
r_cstl_str_dup (const char* pName)
{
    if (!pName) return NULL;
    size_t len = strlen (pName) + 1;
    char*  dup = (char*)malloc (len);
    if (!dup) return NULL;
    memcpy (dup, pName, len);
    return dup;
}

static size_t
r_cstl_next_pow2 (size_t v)
{
    if (v == 0) return 1;
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
r_cstl_size_to_order (size_t size)
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
r_cstl_order_to_size (int order)
{
    return g_heap.minBlock << (size_t)order;
}

#if defined(R_CSTL_HAVE_ASAN)
static void
r_cstl_asan_poison (const void* addr, size_t size)
{
    if (addr && size > 0) __asan_poison_memory_region (addr, size);
}

static void
r_cstl_asan_unpoison (const void* addr, size_t size)
{
    if (addr && size > 0) __asan_unpoison_memory_region (addr, size);
}
#else
static void
r_cstl_asan_poison (const void* addr, size_t size)
{
    (void)addr;
    (void)size;
}

static void
r_cstl_asan_unpoison (const void* addr, size_t size)
{
    (void)addr;
    (void)size;
}
#endif

#ifdef R_CSTL_HEAP_DEBUG
static void
r_cstl_debug_init_runtime_checks (void)
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
r_cstl_debug_install_guard_page (void)
{
    if (!g_heap.pHeapBase || g_heap.mappedSize == 0) return -1;

    size_t page_size = r_cstl_platform_page_size ();
    if (page_size == 0) return -1;

    size_t heap_end = (g_heap.totalSize + page_size - 1) & ~(page_size - 1);
    if (heap_end + page_size > g_heap.mappedSize) return -1;

    void* guard = (char*)g_heap.pHeapBase + heap_end;
#if defined(_WIN32)
    DWORD old_protect = 0;
    if (!VirtualProtect (guard, page_size, PAGE_NOACCESS, &old_protect)) return -1;
#else
    if (mprotect (guard, page_size, PROT_NONE) != 0) return -1;
#endif
    g_heap.guardPageOffset = heap_end;
    g_heap.guardPageSize = page_size;
    return 0;
}

static void
r_cstl_debug_remove_guard_page (void)
{
    if (!g_heap.pHeapBase || g_heap.guardPageSize == 0) return;

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
r_cstl_debug_poison_block_redzone (struct r_cstl_block_header* pHeader)
{
#if R_CSTL_HEAP_POISON_ENABLED
    if (!pHeader) return;

    size_t blockSize = r_cstl_order_to_size ((int)pHeader->order);
    char*  user = (char*)pHeader + sizeof (*pHeader);
    size_t userCapacity = blockSize - sizeof (*pHeader);
    size_t requested = pHeader->requestedSize;
    if (requested >= userCapacity) return;

    size_t redzone = userCapacity - requested;
    memset (user + requested, R_CSTL_HEAP_POISON_REDZONE, redzone);
    r_cstl_asan_poison (user + requested, redzone);
#else
    (void)pHeader;
#endif
}

static void
r_cstl_debug_prepare_user_buffer (struct r_cstl_block_header* pHeader, void* pData)
{
    if (!pHeader || !pData || pHeader->requestedSize == 0) return;

#if R_CSTL_HEAP_POISON_ENABLED
    if (R_CSTL_HEAP_POISON_ALLOC) memset (pData, R_CSTL_HEAP_POISON_ALLOC, pHeader->requestedSize);
#endif
    r_cstl_asan_unpoison (pData, pHeader->requestedSize);
#if R_CSTL_HEAP_POISON_ENABLED
    r_cstl_debug_poison_block_redzone (pHeader);
#endif
}

static void
r_cstl_debug_prepare_user_buffer_no_poison (struct r_cstl_block_header* pHeader, void* pData)
{
    if (!pHeader || !pData) return;

    r_cstl_asan_unpoison (pData, pHeader->requestedSize);
}

static void
r_cstl_debug_poison_freed_user_buffer (struct r_cstl_block_header* pHeader, void* pUser)
{
#if R_CSTL_HEAP_POISON_ENABLED
    if (!pHeader || !pUser || pHeader->requestedSize == 0) return;

    if (R_CSTL_HEAP_POISON_FREE) memset (pUser, R_CSTL_HEAP_POISON_FREE, pHeader->requestedSize);
    r_cstl_asan_poison (pUser, pHeader->requestedSize);
#else
    (void)pHeader;
    (void)pUser;
#endif
}
#else
static void
r_cstl_debug_init_runtime_checks (void)
{
}

static int
r_cstl_debug_install_guard_page (void)
{
    return 0;
}

static void
r_cstl_debug_destroy_guard_page (void)
{
}

static void
r_cstl_debug_poison_block_redzone (struct r_cstl_block_header* pHeader)
{
    (void)pHeader;
}

static void
r_cstl_debug_remove_guard_page (void)
{
}

static void
r_cstl_debug_prepare_user_buffer (struct r_cstl_block_header* pHeader, void* pData)
{
    (void)pHeader;
    (void)pData;
}

static void
r_cstl_debug_prepare_user_buffer_no_poison (struct r_cstl_block_header* pHeader, void* pData)
{
    (void)pHeader;
    (void)pData;
}

static void
r_cstl_debug_poison_freed_user_buffer (struct r_cstl_block_header* pHeader, void* pUser)
{
    (void)pHeader;
    (void)pUser;
}
#endif

static struct r_cstl_block_header*
r_cstl_header_from_user_data (const void* ptr)
{
    if (!ptr) return NULL;
    return (struct r_cstl_block_header*)((const char*)ptr - sizeof (struct r_cstl_block_header));
}

static int
r_cstl_heap_pointer_in_range (const void* ptr)
{
    if (!ptr || !g_heap.pHeapBase || g_heap.totalSize == 0) return 0;
    const char* base = (const char*)g_heap.pHeapBase;
    const char* end = base + g_heap.totalSize;
    const char* p = (const char*)ptr;
    return (p >= base && p < end);
}

static int
r_cstl_header_in_range (const struct r_cstl_block_header* pHeader)
{
    if (!pHeader || !g_heap.pHeapBase || g_heap.totalSize < sizeof (*pHeader)) return 0;
    const char* base = (const char*)g_heap.pHeapBase;
    const char* end = base + g_heap.totalSize - sizeof (*pHeader) + 1;
    const char* p = (const char*)pHeader;
    return (p >= base && p < end);
}

static int
r_cstl_is_live_block_header (const struct r_cstl_block_header* pHeader)
{
    if (!r_cstl_header_in_range (pHeader)) goto cstl_fail;
    if (pHeader->magic != R_CSTL_HEAP_BLOCK_MAGIC) goto cstl_fail;
    if (pHeader->order > (uint32_t)g_heap.maxOrder) goto cstl_fail;
    size_t offset = (size_t)((const char*)pHeader - (const char*)g_heap.pHeapBase);
    if (offset % g_heap.minBlock != 0) goto cstl_fail;
    size_t blockSize = r_cstl_order_to_size ((int)pHeader->order);
    if (offset + blockSize > g_heap.totalSize) goto cstl_fail;
    return 1;
cstl_fail:
    return 0;
}

static struct r_cstl_free_node*
r_cstl_free_node_from_offset (size_t offset)
{
    return (struct r_cstl_free_node*)((char*)g_heap.pHeapBase + offset);
}

static size_t
r_cstl_offset_from_free_node (const struct r_cstl_free_node* pNode)
{
    return (size_t)((const char*)pNode - (const char*)g_heap.pHeapBase);
}

static void
r_cstl_push_free_locked (int order, size_t offset)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (order < 0 || order > g_heap.maxOrder) return;
#endif
    struct r_cstl_free_node* pNode = r_cstl_free_node_from_offset (offset);
    pNode->pNext = g_heap.pFreeLists[order];
    g_heap.pFreeLists[order] = pNode;
}

static void
r_cstl_push_free (int order, size_t offset)
{
    r_cstl_heap_mutex_lock (&g_heap.mutex);
    r_cstl_push_free_locked (order, offset);
    r_cstl_heap_mutex_unlock (&g_heap.mutex);
}

static int
r_cstl_pop_free_locked (int order, size_t* pOutOffset)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pOutOffset || order < 0 || order > g_heap.maxOrder) goto cstl_fail;
#endif

    struct r_cstl_free_node* pHead = g_heap.pFreeLists[order];
    if (!pHead) goto cstl_fail;

    g_heap.pFreeLists[order] = pHead->pNext;
    *pOutOffset = r_cstl_offset_from_free_node (pHead);
    return 1;
cstl_fail:
    return 0;
}

static int
r_cstl_pop_free (int order, size_t* pOutOffset)
{
    r_cstl_heap_mutex_lock (&g_heap.mutex);
    const int found = r_cstl_pop_free_locked (order, pOutOffset);
    r_cstl_heap_mutex_unlock (&g_heap.mutex);
    return found;
}

static int
r_cstl_remove_free_offset_locked (int order, size_t offset)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (order < 0 || order > g_heap.maxOrder) goto cstl_fail;
#endif
    struct r_cstl_free_node* pPrev = NULL;
    struct r_cstl_free_node* pCur = g_heap.pFreeLists[order];
    while (pCur)
    {
        if (r_cstl_offset_from_free_node (pCur) == offset)
        {
            if (pPrev) pPrev->pNext = pCur->pNext;
            else g_heap.pFreeLists[order] = pCur->pNext;
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
r_cstl_remove_free_offset (int order, size_t offset)
{
    r_cstl_heap_mutex_lock (&g_heap.mutex);
    const int removed = r_cstl_remove_free_offset_locked (order, offset);
    r_cstl_heap_mutex_unlock (&g_heap.mutex);
    return removed;
}

static void
r_cstl_remove_allocation_record (void* pAllocation)
{
    if (!pAllocation) return;
    for (struct r_cstl_owner_record* pOwner = g_heap.pOwners; pOwner; pOwner = pOwner->pNext)
    {
        for (size_t i = 0; i < pOwner->allocCount;)
        {
            if (pOwner->pAllocations[i].pAllocation == pAllocation)
            {
                free (pOwner->pAllocations[i].pName);
                pOwner->pAllocations[i] = pOwner->pAllocations[pOwner->allocCount - 1];
                --pOwner->allocCount;
                if (g_heap.registeredCount > 0) --g_heap.registeredCount;
            }
            else
            {
                ++i;
            }
        }
    }
}

int R_CSTL_API
r_cstl_heap_init (size_t heap_size_bytes)
{
    if (g_heap.initialized) return 0;
    if (heap_size_bytes == 0) return -1;

    g_heap.minBlock = 32;
    size_t usable = r_cstl_next_pow2 (heap_size_bytes);
    if (usable < g_heap.minBlock) usable = g_heap.minBlock;
    if (usable % g_heap.minBlock) usable = r_cstl_next_pow2 (usable);

    size_t mappedSize = 0;
    size_t mapRequest = usable;
#ifdef R_CSTL_HEAP_DEBUG
    size_t guard_page = r_cstl_platform_page_size ();
    if (guard_page > 0) mapRequest = usable + guard_page;
#endif
    void* base = r_cstl_platform_heap_map (mapRequest, &mappedSize);
    if (!base || mappedSize < usable)
    {
        r_cstl_platform_heap_unmap (base, mappedSize);
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

    g_heap.pFreeLists
        = (struct r_cstl_free_node**)calloc ((size_t)g_heap.maxOrder + 1, sizeof (struct r_cstl_free_node*));
    if (!g_heap.pFreeLists)
    {
        r_cstl_platform_heap_unmap (g_heap.pHeapBase, g_heap.mappedSize);
        memset (&g_heap, 0, sizeof (g_heap));
        return -1;
    }

    r_cstl_heap_new_mutex (&g_heap.mutex);
    r_cstl_heap_mutex_lock (&g_heap.mutex);
    r_cstl_push_free_locked (g_heap.maxOrder, 0);
    r_cstl_heap_mutex_unlock (&g_heap.mutex);
    g_heap.initialized = true;
    R_CSTL_HEAP_COMPILER_BARRIER ();

    r_cstl_debug_init_runtime_checks ();
    if (r_cstl_debug_install_guard_page () != 0)
    {
        r_cstl_heap_shutdown ();
        return -1;
    }

    return 0;
}

void R_CSTL_API
r_cstl_heap_shutdown (void)
{
    if (!r_cstl_heap_is_ready ()) return;

    r_cstl_heap_log_leaks ();

    for (int i = 0; i <= g_heap.maxOrder; ++i)
        g_heap.pFreeLists[i] = NULL;
    free (g_heap.pFreeLists);

    struct r_cstl_owner_record* pOwner = g_heap.pOwners;
    while (pOwner)
    {
        struct r_cstl_owner_record* pNextOwner = pOwner->pNext;
        for (size_t i = 0; i < pOwner->allocCount; ++i)
            free (pOwner->pAllocations[i].pName);
        free (pOwner->pAllocations);
        free (pOwner->pTypeName);
        free (pOwner);
        pOwner = pNextOwner;
    }

    r_cstl_heap_mutex_destroy (&g_heap.mutex);
    r_cstl_debug_remove_guard_page ();
    r_cstl_platform_heap_unmap (g_heap.pHeapBase, g_heap.mappedSize);
    memset (&g_heap, 0, sizeof (g_heap));
    R_CSTL_HEAP_COMPILER_BARRIER ();
}

R_CSTL_API void*
r_cstl_heap_alloc (size_t size)
{
    if (!r_cstl_heap_is_ready ()) return NULL;
#if defined(R_CSTL_HEAP_DEBUG)
    if (size == 0) return NULL;
#endif
    size_t total = size + sizeof (struct r_cstl_block_header);
    int    target = r_cstl_size_to_order (total <= g_heap.minBlock ? g_heap.minBlock : total);
    if (target > g_heap.maxOrder) return NULL;

    int    order = target;
    size_t offset = 0;

    r_cstl_heap_mutex_lock (&g_heap.mutex);

    while (order <= g_heap.maxOrder)
    {
        if (r_cstl_pop_free_locked (order, &offset)) break;
        ++order;
    }
    if (order > g_heap.maxOrder)
    {
        r_cstl_heap_mutex_unlock (&g_heap.mutex);
        return NULL;
    }

    while (order > target)
    {
        --order;
        size_t sizeBlock = r_cstl_order_to_size (order);
        r_cstl_push_free_locked (order, offset + sizeBlock);
    }

    const uint64_t allocId = g_heap.nextAllocId++;
    g_heap.usedBytes += r_cstl_order_to_size (order);
    r_cstl_heap_mutex_unlock (&g_heap.mutex);

    struct r_cstl_block_header* pHeader = (struct r_cstl_block_header*)((char*)g_heap.pHeapBase + offset);
    pHeader->magic = R_CSTL_HEAP_BLOCK_MAGIC;
    pHeader->order = (uint32_t)order;
    pHeader->requestedSize = size;
    pHeader->allocId = allocId;

    void* pUser = (void*)((char*)pHeader + sizeof (struct r_cstl_block_header));
    r_cstl_debug_prepare_user_buffer (pHeader, pUser);
    return pUser;
}

static void*
r_cstl_heap_alloc_no_poison (size_t size)
{
    if (!r_cstl_heap_is_ready ()) return NULL;
#if defined(R_CSTL_HEAP_DEBUG)
    if (size == 0) return NULL;
#endif
    size_t total = size + sizeof (struct r_cstl_block_header);
    int    target = r_cstl_size_to_order (total <= g_heap.minBlock ? g_heap.minBlock : total);
    if (target > g_heap.maxOrder) return NULL;

    int    order = target;
    size_t offset = 0;

    r_cstl_heap_mutex_lock (&g_heap.mutex);

    while (order <= g_heap.maxOrder)
    {
        if (r_cstl_pop_free_locked (order, &offset)) break;
        ++order;
    }

    if (order > g_heap.maxOrder)
    {
        r_cstl_heap_mutex_unlock (&g_heap.mutex);
        return NULL;
    }

    while (order > target)
    {
        --order;
        size_t sizeBlock = r_cstl_order_to_size (order);
        r_cstl_push_free_locked (order, offset + sizeBlock);
    }

    const uint64_t allocId = g_heap.nextAllocId++;
    g_heap.usedBytes += r_cstl_order_to_size (order);
    r_cstl_heap_mutex_unlock (&g_heap.mutex);

    struct r_cstl_block_header* pHeader = (struct r_cstl_block_header*)((char*)g_heap.pHeapBase + offset);
    pHeader->magic = R_CSTL_HEAP_BLOCK_MAGIC;
    pHeader->order = (uint32_t)order;
    pHeader->requestedSize = size;
    pHeader->allocId = allocId;

    void* pUser = (void*)((char*)pHeader + sizeof (struct r_cstl_block_header));
    r_cstl_debug_prepare_user_buffer_no_poison (pHeader, pUser);
    return pUser;
}

R_CSTL_API void*
r_cstl_heap_alloc_aligned (size_t size, size_t alignment)
{
    if (!r_cstl_heap_is_ready ()) return NULL;
#if defined(R_CSTL_HEAP_DEBUG)
    if (size == 0) return NULL;
#endif
    if (alignment == 0 || (alignment & (alignment - 1)) != 0) return NULL; // alignment must be power of two
    size_t total = size + alignment + sizeof (void*);
    void*  pRaw = r_cstl_heap_alloc (total);
    if (!pRaw) return NULL;

    uintptr_t addr = (uintptr_t)pRaw + sizeof (void*);
    uintptr_t aligned = (addr + alignment - 1) & ~(alignment - 1);

    void** pOriginalPtr = (void**)(aligned - sizeof (void*));
    *pOriginalPtr = pRaw;
    return (void*)aligned;
}

void R_CSTL_API
r_cstl_heap_free (void* pData)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pData) return;
#endif
    if (!r_cstl_heap_is_ready ())
    {
        R_CSTL_LOG_ERROR ("r_cstl_heap_free: heap not initialized");
        return;
    }
    // Check if this might be an aligned allocation by checking if pointer is in range
    // If not, it might be an aligned allocation so retrieve original pointer
    if (!r_cstl_heap_pointer_in_range (pData))
    {
        // Try to retrieve original pointer from aligned allocation
        void** pOriginalPtr = (void**)((char*)pData - sizeof (void*));
        pData = *pOriginalPtr;
        if (!pData || !r_cstl_heap_pointer_in_range (pData))
        {
            R_CSTL_LOG_ERROR ("r_cstl_heap_free: pointer %p outside heap range", pData);
            return;
        }
    }

    if (!r_cstl_heap_pointer_in_range (pData))
    {
        R_CSTL_LOG_ERROR ("r_cstl_heap_free: pointer %p outside heap range", pData);
        return;
    }
    struct r_cstl_block_header* pHeader = r_cstl_header_from_user_data (pData);
    if (!r_cstl_header_in_range (pHeader))
    {
        R_CSTL_LOG_ERROR ("r_cstl_heap_free: invalid header for pointer %p", pData);
        return;
    }
    if (pHeader->magic == R_CSTL_HEAP_FREED_MAGIC)
    {
        R_CSTL_LOG_ERROR (
            "r_cstl_heap_free: double free detected for %p (allocId=0x%016" PRIx64 ")\n",
            pData,
            pHeader->allocId);
        return;
    }
    if (pHeader->magic != R_CSTL_HEAP_BLOCK_MAGIC)
    {
        R_CSTL_LOG_ERROR ("r_cstl_heap_free: invalid or corrupted pointer %p", pData);
        return;
    }

    int order = (int)pHeader->order;
    if (order < 0 || order > g_heap.maxOrder)
    {
        R_CSTL_LOG_ERROR ("r_cstl_heap_free: invalid order %d for ptr %p", order, pData);
        return;
    }

    size_t offset = (size_t)((char*)pHeader - (char*)g_heap.pHeapBase);
    if (offset % g_heap.minBlock != 0)
    {
        R_CSTL_LOG_ERROR ("r_cstl_heap_free: misaligned block at %p", pData);
        return;
    }

    size_t blockSize = r_cstl_order_to_size (order);
    if (offset + blockSize > g_heap.totalSize)
    {
        R_CSTL_LOG_ERROR ("r_cstl_heap_free: block extends past heap for %p", pData);
        return;
    }

    r_cstl_debug_poison_freed_user_buffer (pHeader, pData);
    pHeader->magic = R_CSTL_HEAP_FREED_MAGIC;

    size_t curOffset = offset;
    int    curOrder = order;

    r_cstl_heap_mutex_lock (&g_heap.mutex);
#ifdef R_CSTL_HEAP_DEBUG
    r_cstl_remove_allocation_record (pData);
#endif
    while (curOrder < g_heap.maxOrder)
    {
        size_t buddy = curOffset ^ r_cstl_order_to_size (curOrder);
        if (r_cstl_remove_free_offset_locked (curOrder, buddy))
        {
            curOffset = (curOffset < buddy) ? curOffset : buddy;
            ++curOrder;
            continue;
        }
        break;
    }

    r_cstl_push_free_locked (curOrder, curOffset);
    g_heap.usedBytes -= blockSize;
    r_cstl_heap_mutex_unlock (&g_heap.mutex);
}

R_CSTL_API void*
r_cstl_heap_realloc (void* pData, size_t newSize)
{
    if (!r_cstl_heap_is_ready ()) return NULL;
    if (!pData) return r_cstl_heap_alloc (newSize);
    if (newSize == 0)
    {
        r_cstl_heap_free (pData);
        return NULL;
    }

#ifdef R_CSTL_HEAP_DEBUG
    if (!r_cstl_heap_is_valid_pointer (pData)) return NULL;
#endif

    struct r_cstl_block_header* pHeader = r_cstl_header_from_user_data (pData);
#ifdef R_CSTL_HEAP_DEBUG
    if (!r_cstl_is_live_block_header (pHeader)) return NULL;
#endif

    size_t totalNeeded = newSize + sizeof (struct r_cstl_block_header);
    int    targetOrder = r_cstl_size_to_order (totalNeeded <= g_heap.minBlock ? g_heap.minBlock : totalNeeded);
    int    currentOrder = (int)pHeader->order;

    if (targetOrder == currentOrder)
    {
        pHeader->requestedSize = newSize;
        r_cstl_debug_prepare_user_buffer (pHeader, pData);
        return pData;
    }

    if (targetOrder < currentOrder)
    {
        pHeader->requestedSize = newSize;
        r_cstl_debug_prepare_user_buffer (pHeader, pData);
        return pData;
    }

    void* pNew = r_cstl_heap_alloc_no_poison (newSize);
    if (!pNew) return NULL;

    size_t copyBytes = pHeader->requestedSize;
    if (copyBytes > newSize) copyBytes = newSize;
    if (copyBytes > 0) memcpy (pNew, pData, copyBytes);

    r_cstl_heap_free (pData);

    struct r_cstl_block_header* pNewHeader = r_cstl_header_from_user_data (pNew);
    r_cstl_debug_poison_block_redzone (pNewHeader);

    return pNew;
}

static struct r_cstl_owner_record*
r_cstl_find_or_create_heap_owner (void* pOwner)
{
    if (!pOwner) return NULL;

    r_cstl_heap_mutex_lock (&g_heap.mutex);
    for (struct r_cstl_owner_record* pRecord = g_heap.pOwners; pRecord; pRecord = pRecord->pNext)
    {
        if (pRecord->pOwner == pOwner)
        {
            r_cstl_heap_mutex_unlock (&g_heap.mutex);
            return pRecord;
        }
    }

    struct r_cstl_owner_record* pNew = (struct r_cstl_owner_record*)malloc (sizeof (struct r_cstl_owner_record));
    if (!pNew)
    {
        R_CSTL_LOG_ERROR ("r_cstl_find_or_create_heap_owner: metadata allocation failed");
        r_cstl_heap_mutex_unlock (&g_heap.mutex);
        return NULL;
    }
    memset (pNew, 0, sizeof (*pNew));
    pNew->pOwner = pOwner;
    pNew->pNext = g_heap.pOwners;
    g_heap.pOwners = pNew;
    r_cstl_heap_mutex_unlock (&g_heap.mutex);
    return pNew;
}

static uint64_t
r_cstl_heap_ComputeHash (const void* a, const void* b, size_t size, const char* pName)
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
r_cstl_heap_register_allocation (void* pOwner, void* pAllocation, size_t size, const char* pName)
{
#ifndef R_CSTL_HEAP_DEBUG
    (void)pOwner;
    (void)pAllocation;
    (void)size;
    (void)pName;
    return 1;
#else
    if (!pOwner || !pAllocation || size == 0) return 0;
    if (!g_heap.initialized || !g_heap.pHeapBase) return 0;
    if (!r_cstl_heap_is_valid_pointer (pAllocation))
    {
        R_CSTL_LOG_ERROR ("r_cstl_heap_register_allocation: %p is not a live heap allocation", pAllocation);
        return 0;
    }
    struct r_cstl_block_header* pHeader = r_cstl_header_from_user_data (pAllocation);
    if (pHeader->requestedSize != size)
    {
        R_CSTL_LOG_ERROR (
            "r_cstl_heap_register_allocation: size mismatch for %p "
            "(expected %zu, got %zu)\n",
            pAllocation,
            pHeader->requestedSize,
            size);
        return 0;
    }

    struct r_cstl_owner_record* pOwnerRecord = r_cstl_find_or_create_heap_owner (pOwner);
    if (!pOwnerRecord) return 0;

    r_cstl_heap_mutex_lock (&g_heap.mutex);
    if (pOwnerRecord->allocCount == pOwnerRecord->allocCapacity)
    {
        size_t ncap = pOwnerRecord->allocCapacity ? pOwnerRecord->allocCapacity * 2 : 8;
        struct r_cstl_allocation_record* n = (struct r_cstl_allocation_record*)realloc (
            pOwnerRecord->pAllocations,
            ncap * sizeof (struct r_cstl_allocation_record));
        if (!n)
        {
            r_cstl_heap_mutex_unlock (&g_heap.mutex);
            R_CSTL_LOG_DEBUG ("r_cstl_heap_register_allocation: metadata realloc failed");
            return 0;
        }
        pOwnerRecord->pAllocations = n;
        pOwnerRecord->allocCapacity = ncap;
    }

    struct r_cstl_allocation_record* pRecord = &pOwnerRecord->pAllocations[pOwnerRecord->allocCount++];
    pRecord->pAllocation = pAllocation;
    pRecord->size = size;
    pRecord->pName = r_cstl_str_dup (pName);
    if (pName && !pRecord->pName)
    {
        --pOwnerRecord->allocCount;
        r_cstl_heap_mutex_unlock (&g_heap.mutex);
        R_CSTL_LOG_ERROR ("r_cstl_heap_register_allocation: name strdup failed");
        return 0;
    }
    pRecord->allocId = pHeader->allocId;
    pRecord->hash = r_cstl_heap_ComputeHash (pOwner, pAllocation, size, pName);
    ++g_heap.registeredCount;
    r_cstl_heap_mutex_unlock (&g_heap.mutex);
    return pRecord->hash;
#endif
}

void
r_cstl_heap_unregister_allocation (void* pOwner, void* pAllocation)
{
#ifndef R_CSTL_HEAP_DEBUG
    (void)pOwner;
    (void)pAllocation;
    return;
#else
    if (!pOwner || !pAllocation) return;
    if (!g_heap.initialized) return;

    r_cstl_heap_mutex_lock (&g_heap.mutex);
    for (struct r_cstl_owner_record* pOwnerRecord = g_heap.pOwners; pOwnerRecord;
         pOwnerRecord = pOwnerRecord->pNext)
    {
        if (pOwnerRecord->pOwner != pOwner) continue;
        for (size_t i = 0; i < pOwnerRecord->allocCount; ++i)
        {
            if (pOwnerRecord->pAllocations[i].pAllocation != pAllocation) continue;
            free (pOwnerRecord->pAllocations[i].pName);
            pOwnerRecord->pAllocations[i] = pOwnerRecord->pAllocations[pOwnerRecord->allocCount - 1];
            --pOwnerRecord->allocCount;
            if (g_heap.registeredCount > 0) --g_heap.registeredCount;
            break;
        }
        break;
    }
    r_cstl_heap_mutex_unlock (&g_heap.mutex);
#endif
}

size_t
r_cstl_heap_check_object_leaks (void* pOwner)
{
    if (!pOwner || !g_heap.initialized) return 0;

    r_cstl_heap_mutex_lock (&g_heap.mutex);
    size_t leaked = 0;
    for (struct r_cstl_owner_record* pOwnerRecord = g_heap.pOwners; pOwnerRecord;
         pOwnerRecord = pOwnerRecord->pNext)
    {
        if (pOwnerRecord->pOwner != pOwner) continue;
        for (size_t i = 0; i < pOwnerRecord->allocCount; ++i)
        {
            struct r_cstl_allocation_record* ar = &pOwnerRecord->pAllocations[i];
            int                             stillLive = r_cstl_heap_is_valid_pointer (ar->pAllocation);
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
    r_cstl_heap_mutex_unlock (&g_heap.mutex);
    return leaked;
}

size_t
r_cstl_heap_log_leaks (void)
{
    if (!g_heap.initialized) return 0;

    r_cstl_heap_mutex_lock (&g_heap.mutex);
    size_t total = 0;
    size_t bytes = 0;
    for (struct r_cstl_owner_record* pOwnerRecord = g_heap.pOwners; pOwnerRecord;
         pOwnerRecord = pOwnerRecord->pNext)
    {
        if (pOwnerRecord->allocCount == 0) continue;
        R_CSTL_LOG_ERROR (
            "Owner %p (type=%s) has %zu leaked allocations:",
            pOwnerRecord->pOwner,
            pOwnerRecord->pTypeName ? pOwnerRecord->pTypeName : "(unknown)",
            pOwnerRecord->allocCount);
        for (size_t i = 0; i < pOwnerRecord->allocCount; ++i)
        {
            struct r_cstl_allocation_record* ar = &pOwnerRecord->pAllocations[i];
            int stillLive = r_cstl_is_live_block_header (r_cstl_header_from_user_data (ar->pAllocation));

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
        R_CSTL_LOG_ERROR ("R_CSTL heap leak summary: %zu allocations, %zu bytes tracked", total, bytes);
    }
    r_cstl_heap_mutex_unlock (&g_heap.mutex);
    return total;
}

int
r_cstl_heap_is_valid_pointer (const void* ptr)
{
    if (!g_heap.initialized || !ptr) return 0;
    if (!r_cstl_heap_pointer_in_range (ptr)) return 0;
#ifdef R_CSTL_HEAP_DEBUG
    return r_cstl_is_live_block_header (r_cstl_header_from_user_data (ptr));
#else
    const struct r_cstl_block_header* pHeader = r_cstl_header_from_user_data (ptr);
    return pHeader->magic == R_CSTL_HEAP_BLOCK_MAGIC;
#endif
}

size_t
r_cstl_heap_get_registered_count (void)
{
    if (!g_heap.initialized) return 0;
    r_cstl_heap_mutex_lock (&g_heap.mutex);
    size_t count = g_heap.registeredCount;
    r_cstl_heap_mutex_unlock (&g_heap.mutex);
    return count;
}

size_t
r_cstl_heap_GetTotalSize (void)
{
    return g_heap.totalSize;
}

size_t
r_cstl_heap_GetUsedSize (void)
{
    if (!g_heap.initialized) return 0;
    r_cstl_heap_mutex_lock (&g_heap.mutex);
    size_t used = g_heap.usedBytes;
    r_cstl_heap_mutex_unlock (&g_heap.mutex);
    return used;
}

#ifdef R_CSTL_HEAP_DEBUG
int
r_cstl_heap_debug_verify (void)
{
    if (!g_heap.initialized || !g_heap.pHeapBase) return 1;
    if (g_heap.usedBytes > g_heap.totalSize) return 2;
#if defined(_WIN32) && defined(_MSC_VER)
    if (!_CrtCheckMemory ()) return 3;
#endif
    return 0;
}
#endif
