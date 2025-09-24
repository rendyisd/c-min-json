#include <stdio.h>

#include "minjson.h"
#include "arena.h"

/* Deserialization:
 * Raw jSON -> token -> syntax tree -> C struct
 */ 

enum minjson_type {
    MJ_OBJECT,
    MJ_ARRAY,
    MJ_STRING,
    MJ_NUMBER,
    MJ_TRUE,
    MJ_FALSE,
    MJ_NULL,
};

struct minjson {
    enum minjson_type type; /* Bool stored here with value NULL */
    union {
        struct minjson_object *object;
        struct minjson_array *array;
        char *string;
        double number; /* Cast to int if needed */
    } value;
};

/* Any number of key value pair */
struct minjson_object { /* This itself is value of type object */
    char **keys;
    struct minjson **values;
    size_t len, capacity;
};

/* Any number of value(any type) */
struct minjson_array { /* This itself is value of type array */
    struct minjson **items;
    size_t len, capacity;
};

enum token_type {
    TK_STRING,
    TK_NUMBER,
    TK_NULL,
    TK_BOOL,
    TK_OPEN_CB,
    TK_CLOSE_CB,
    TK_OPEN_SB,
    TK_CLOSE_SB,
    TK_COLON,
    TK_DELIMITER,
};

struct minjson_token {
    enum token_type type;
    char *lexeme;
    size_t len;
    size_t line, column;
};

struct minjson_lexer {
    struct minjson_token **tokens;
    size_t len;
    size_t c_line, c_column;
};

static void lexer_skip_whitespaces(struct minjson_lexer *lexer, char *c)
