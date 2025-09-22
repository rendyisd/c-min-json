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


#define COMMON_ARENA_SIZE 1024

#include <stdlib.h>

struct Arena;

struct Arena *arena_new(size_t size);
void arena_destroy(struct Arena *arena);
size_t arena_remaining_size(struct Arena *arena);

void test_arena_func(void);

#endif
