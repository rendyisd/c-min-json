#include <unistd.h>
#include <string.h>

#include "../src/arena.c"
#include "../src/minjson.c"

__AFL_FUZZ_INIT();

int main(void)
{
    __AFL_INIT();
    char *src = NULL;

    while (__AFL_LOOP(10000)) {
        unsigned char *buf = __AFL_FUZZ_TESTCASE_BUF;
        int len = __AFL_FUZZ_TESTCASE_LEN;
        struct arena_allocator *aa = arena_allocator_new(1 << 16);
        if (!aa)
            continue;
        src = arena_allocator_alloc(aa, DEFAULT_ALIGNMENT, len + 1);
        if (!src)
            continue;
        memcpy(src, buf, len);
        src[len] = 0;

        minjson_parse(aa, src, &(struct minjson_error){});

        arena_allocator_destroy(aa);
    }

    return 0;
}
