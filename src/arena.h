#ifndef ARENA_H
#define ARENA_H


#if __has_feature(address_sanitizer) || defined(__SANITIZE_ADDRESS__)

#include <sanitizer/asan_interface.h>
#define ASAN_POISON_MEMORY_REGION(addr, size) \
        __asan_poison_memory_region((addr), (size))
#define ASAN_UNPOISON_MEMORY_REGION(addr, size) \
        __asan_unpoison_memory_region((addr), (size))

#else

#define ASAN_POISON_MEMORY_REGION(addr, size) \
        ((void)(addr), (void)(size))
#define ASAN_UNPOISON_MEMORY_REGION(addr, size) \
        ((void)(addr), (void)(size))

#endif 

#if DEBUG

#include <assert.h>
#define ASSERT(expr) assert(expr)

#else

#define ASSERT(expr) ((void) 0)

#endif


#define DEFAULT_ARENA_SIZE (4 * 1024)
#define DEFAULT_ALIGNMENT 8

#include <stdlib.h>

/**
 * @brief   An arena allocator that contains 1 or multiple chained arena
 */
struct arena_allocator;

/**
 * @brief   Creates a new arena_allocator.
 */
struct arena_allocator *arena_allocator_new(size_t size);

/**
 * @brief   Destroy given arena_allocator and all arena inside of it.
 */
void arena_allocator_destroy(struct arena_allocator *aa);

/**
 * @brief   Allocates size amount of bytes from the arena inside arena_allocator.
 *
 * Allocates a block of memory of the specified size and given alignment.
 * arena_allocator grows by creating a new arena if capacity is not sufficient.
 *
 * @param   aa          The arena allocator.
 * @param   alignment   The block alignment to its neighboring blocks. Must be power of 2
 * @param   size        Number of bytes to allocate
 *
 * @return  Pointer to the allocated memory, NULL on error.
 */ 
void *arena_allocator_alloc(struct arena_allocator *aa,
                            size_t alignment,
                            size_t size);

#endif
