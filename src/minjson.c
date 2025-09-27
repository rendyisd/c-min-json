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
    struct arena_allocator *arena;
    struct minjson_value *root;
};

struct minjson_value {
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
    struct minjson_value **values;
    size_t len, capacity;
};

/* Any number of value(any type) */
struct minjson_array { /* This itself is value of type array */
    struct minjson_value **items;
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
    size_t pos_line, pos_column;
};

static void lexer_skip_whitespaces(struct minjson_lexer *lexer, char **ptr)
{
    char *c = *ptr;

    /* - space
     * - horizontal tab
     * - line feed
     * - carriage return
     * - CRLF
     * Will be treated as 1 character
     */
    while (*c == ' ' || *c == '\t' || *c == '\n' || *c == '\r') {

        if (*c == '\r' && *(c+1) == '\n')
            ++c;
        
        if (*c == '\n') {
            lexer->pos_column = 1;
            lexer->pos_line += 1;
        } else {
            lexer->pos_column += 1;
        }

        ++c;
    }

    *ptr = c;
}

int lexer_tokenize(struct minjson_lexer *lexer, char *raw_json)
{
    while (*raw_json != '\0') {
        switch (*raw_json) {
            case '{':
                /* Parse object */
                break;
            case '"':
                /* Parse string */
                break;
            default:
                raw_json += 1;
                break;
        }
    }

    return 0;
}

int minjson_deserialize(char *raw_json)
{
    /* Lexer and arena here 
     */
    return 0;
}
