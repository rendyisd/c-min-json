#ifndef MINJSON_H
#define MINJSON_H

#if DEBUG

#include <assert.h>
#define ASSERT(expr) assert(expr)

#else

#define ASSERT(expr) ((void) 0)

#endif

#include "arena.h"


struct minjson;
struct minjson_lexer;

enum minjson_error_code {
    MJ_CODE_OK,
    MJ_ERR_ALLOCATOR,
    MJ_ERR_TOKEN,
    MJ_ERR_STRING,
    MJ_ERR_LITERAL,
    MJ_ERR_NUMBER,
};
struct minjson_error {
    enum minjson_error_code code;
    char message[128];
    size_t line, column;
};

struct minjson *minjson_parse(struct arena_allocator *doc_aa,
                              const char *raw_json,
                              struct minjson_error *error);
struct minjson_error minjson_error_new(void);
struct minjson_lexer *minjson_lexer_new(struct arena_allocator *aa,
                                const char *raw_json);

int minjson_lexer_tokenize(struct minjson_lexer *lexer,
                           struct minjson_error *error);
void minjson_lexer_print_tokens(struct minjson_lexer *lexer);

#endif
