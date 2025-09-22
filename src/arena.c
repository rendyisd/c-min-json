#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "arena.h"

/* |x|x|x|x|x| | | | | | |
 *  ^         ^           ^
 *  |         |           |
 * start   current       end (one past the malloc-ed area,
 *                            shall never be referenced,
 *                            strictly for pointer arithmetic and comparison)
 */ 

struct Arena {
    char *start;
    char *current;
    char *end;
};

struct Arena *arena_new(size_t size)
{
    struct Arena *arena = malloc(sizeof(struct Arena));

    arena->start = malloc(size);
    arena->current = arena->start;
    arena->end = arena->start + size;

    ASAN_POISON_MEMORY_REGION(arena->current, arena_remaining_size(arena));

    return arena;
}

void arena_destroy(struct Arena *arena)
{
    free(arena->start);
    free(arena);
}

size_t arena_remaining_size(struct Arena *arena)
{
    return arena->end - arena->current;
}

// ASAN wants an alignment of 8 to work. Why? No idea
void *arena_alloc(struct Arena *arena, size_t alignment, size_t size)
{
    // Alignment must always be a power of 2
    assert((alignment & (alignment - 1)) == 0);

    size_t pad = -(uintptr_t)arena->current & (alignment - 1);
    size_t total = pad + size;
    if (arena_remaining_size(arena) < total)
        return NULL;

    void *block = pad + arena->current;
    arena->current += total;

    ASAN_UNPOISON_MEMORY_REGION(block, total);
        
    return block;
}
