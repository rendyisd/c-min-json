#ifndef MINJSON_H
#define MINJSON_H

#if DEBUG

#include <assert.h>
#define ASSERT(expr) assert(expr)

#else

#define ASSERT(expr) ((void) 0)

#endif

#include "arena.h"


struct minjson {
    struct arena_allocator *aallocator;
    struct minjson_value *root;
};
struct minjson_lexer;
struct minjson_object;
struct minjson_array;

enum minjson_error_code {
    MJ_CODE_OK,
    MJ_ERR_ALLOCATOR,
    MJ_ERR_TOKEN,
    MJ_ERR_STRING,
    MJ_ERR_LITERAL,
    MJ_ERR_NUMBER,
    MJ_ERR_OBJECT,
    MJ_ERR_ARRAY,
    MJ_ERR_VALUE,
};
struct minjson_error {
    enum minjson_error_code code;
    char message[128];
    size_t line, column;
};

struct minjson_lexer *minjson_lexer_new(struct arena_allocator *aa,
                                const char *raw_json);

int minjson_lexer_tokenize(struct minjson_lexer *lexer,
                           struct minjson_error *error);

void minjson_lexer_print_tokens(struct minjson_lexer *lexer);

/* ================== Public Facing API ================== */
struct minjson *minjson_parse(struct arena_allocator *doc_aa,
                              const char *raw_json,
                              struct minjson_error *error);

struct minjson_error minjson_error_new(void);

struct minjson_value *minjson_get(struct minjson *doc, const char *key);

struct minjson_value *minjson_object_get(struct minjson_value *value,
                                         const char *key);

struct minjson_value *minjson_array_get(struct minjson_value *value,
                                        size_t index);

size_t minjson_array_get_size(struct minjson_array *array);

int minjson_value_is_null(struct minjson_value *value);

int minjson_value_is_number(struct minjson_value *value);
double minjson_value_get_number(struct minjson_value *value);

int minjson_value_is_string(struct minjson_value *value);
char *minjson_value_get_string(struct minjson_value *value);

int minjson_value_is_bool(struct minjson_value *value);
int minjson_value_get_bool(struct minjson_value *value);

int minjson_value_is_array(struct minjson_value *value);
struct minjson_array *minjson_value_get_array(struct minjson_value *value);

int minjson_value_is_object(struct minjson_value *value);
struct minjson_object *minjson_value_get_object(struct minjson_value *value);

#endif
