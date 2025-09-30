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

struct minjson_lexer *lexer_new(struct arena_allocator *aa,
                                const char *raw_json);
struct minjson *minjson_parse(struct arena_allocator *doc_aa,
                              const char *raw_json);

int lexer_tokenize(struct minjson_lexer *lexer);
void lexer_print_tokens(struct minjson_lexer *lexer);

#endif
