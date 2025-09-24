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

struct arena {
    char *start;
    char *current;
    char *end;
};

struct arena *arena_new(size_t size)
{
    struct arena *a = malloc(sizeof(struct arena));

    a->start = malloc(size);
    a->current = a->start;
    a->end = a->start + size;

    ASAN_POISON_MEMORY_REGION(a->current, arena_remaining_size(a));

    return a;
}

void arena_destroy(struct arena *a)
{
    free(a->start);
    free(a);
}

size_t arena_remaining_size(struct arena *a)
{
    return a->end - a->current;
}

/* ASAN wants an alignment of 8 to work. Why? No idea */
void *arena_alloc(struct arena *a, size_t alignment, size_t size)
{
    /* Alignment must always be a power of 2 */
    assert((alignment & (alignment - 1)) == 0);

    size_t pad = -(uintptr_t)a->current & (alignment - 1);
    size_t total = pad + size;
    if (arena_remaining_size(a) < total)
        return NULL;

    void *block = pad + a->current;
    a->current += total;

    ASAN_UNPOISON_MEMORY_REGION(block, total);
        
    return block;
}
