#include <stdio.h>

#include "arena.h"
#include "minjson.h"

int main(void)
{
    struct arena_allocator *aa = arena_allocator_new(DEFAULT_ARENA_SIZE);
    char *source = NULL;
    /* Code for reading a file from stack overflow */
    FILE *fp = fopen("src/test.json", "r");
    if (fp != NULL) {
        /* Go to the end of the file. */
        if (fseek(fp, 0L, SEEK_END) == 0) {
            /* Get the size of the file. */
            long bufsize = ftell(fp);
            if (bufsize == -1) { /* Error */ }

            /* Allocate our buffer to that size. */
            source = arena_allocator_alloc(aa,
                                           DEFAULT_ALIGNMENT,
                                           sizeof(char) * (bufsize + 1));

            /* Go back to the start of the file. */
            if (fseek(fp, 0L, SEEK_SET) != 0) { /* Error */ }

            /* Read the entire file into memory. */
            size_t newLen = fread(source, sizeof(char), bufsize, fp);
            if ( ferror( fp ) != 0 ) {
                fputs("Error reading file", stderr);
            } else {
                source[newLen] = '\0'; /* Just to be safe. */
            }
        }
        fclose(fp);
    } else {
        arena_allocator_destroy(aa);
        abort();
    }

    struct minjson_lexer *lexer = lexer_new(aa, source);
    if (lexer_tokenize(lexer) == -1) {
        arena_allocator_destroy(aa);
        abort();
    }
    
    lexer_print_tokens(lexer);

    arena_allocator_destroy(aa);

    return 0;
}
