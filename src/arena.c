#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#include "arena.h"

/* |x|x|x|x|x| | | | | | |
 *  ^         ^         ^
 *  |         |         |
 * start   current     end
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
    arena->end = arena->start + size - 1;

    return arena;
}

void arena_destroy(struct Arena *arena)
{
    free(arena->start);
    free(arena);
}

size_t arena_remaining_size(struct Arena *arena)
{
    return arena->end - arena->current + 1;
}

void test_arena_func(void)
{
    printf("Hello, from arena\n");
}

// void *arena_alloc(struct Arena *arena, size_t size)
// {
//     if (arena_remaining_size(arena) < size)
//         return NULL;
//     
//     ptrdiff_t pad = -(uintptr_t) & 
//     void *block = 
// }
