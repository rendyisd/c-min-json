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
    struct arena *next;
};

struct arena_allocator {
    struct arena *head;   
    struct arena *tail;
};

static size_t arena_remaining_size(struct arena *a)
{
    return a->end - a->current;
}

static struct arena *arena_new(size_t size)
{
    struct arena *a = malloc(sizeof(struct arena));
    if (!a)
        return NULL;

    a->start = malloc(size);
    if (!a->start) {
        free(a);
        return NULL;
    }
    a->current = a->start;
    a->end = a->start + size;
    a->next = NULL;

    ASAN_POISON_MEMORY_REGION(a->current, arena_remaining_size(a));

    return a;
}

static void arena_destroy(struct arena *a)
{
    if (a) {
        free(a->start);
        free(a);
    }
}

static struct arena *arena_allocator_grow(struct arena_allocator* aa,
                                          size_t size)
{
    struct arena *a = arena_new(size);
    if (!a)
        return NULL;

    if (aa->tail)
        aa->tail->next = a;
    else
        aa->head = a;

    aa->tail = a;

    return a;
}

struct arena_allocator *arena_allocator_new(size_t size)
{
    struct arena_allocator *aa = malloc(sizeof(struct arena_allocator));
    if (!aa)
        return NULL;

    struct arena *a = arena_new(size);
    if (!a) {
        free(aa);
        return NULL;
    }

    aa->head = a;
    aa->tail = a;

    return aa;
}

void arena_allocator_destroy(struct arena_allocator *aa)
{
    struct arena *ar;
    struct arena *prev_ar;
    if (aa) {
        ar = aa->head;
        while (ar) {
            prev_ar = ar; 
            ar = ar->next;
            arena_destroy(prev_ar);
        }
        free(aa);
    }
}

/* ASAN wants an alignment of 8 to work. Why? No idea */
void *arena_allocator_alloc(struct arena_allocator *aa, size_t alignment, size_t size)
{
    /* Alignment must always be a power of 2 */
    assert((alignment & (alignment - 1)) == 0);

    size_t pad;
    size_t total;
    size_t grow_size;
    struct arena *available_ar = aa->head;
    while (available_ar) { 
        pad = -(uintptr_t)available_ar->current & (alignment - 1);
        total = pad + size;
        if (arena_remaining_size(available_ar) >= total)
            break; 
        available_ar = available_ar->next;
    }

    if (!available_ar) {
        /* Safer this way than using total */
        grow_size = size + alignment;
        if (grow_size < DEFAULT_ARENA_SIZE) 
            grow_size = DEFAULT_ARENA_SIZE;
        available_ar = arena_allocator_grow(aa, grow_size);
        if (!available_ar)
            return NULL;
    }

    void *block = pad + (char *)available_ar->current;
    available_ar->current += total;

    ASAN_UNPOISON_MEMORY_REGION(block, size);
        
    return block;
}
