#include <stdio.h>

#include "arena.h"
#include "min_json.h"

int main(void)
{
    printf("Hello, from test!\n");
    test_minjson_func();

    char buf[16];

    ASAN_POISON_MEMORY_REGION(buf, sizeof(buf));
    buf[0] = 'A';

    ASAN_UNPOISON_MEMORY_REGION(buf, sizeof(buf));
    buf[0] = 'B';

    printf("%c\n", buf[0]);

    return 0;
}
