#include <stdio.h>
#include "minjson.h"

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
    struct arena_allocator *aallocator;
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
    const char *lexeme;
    size_t len;
    size_t line, column;
    struct minjson_token *next;
};

struct minjson_lexer {
    struct arena_allocator *aallocator;
    struct minjson_token *tk_head;
    struct minjson_token *tk_tail;
    const char *current;
    size_t pos_line, pos_column;
};

static void lexer_advance(struct minjson_lexer *lexer, size_t step)
{
    lexer->current += step;
    lexer->pos_column += step;
}

static void lexer_skip_whitespaces(struct minjson_lexer *lexer)
{
    const char *c = lexer->current;

    while (*c == ' ' || *c == '\t' || *c == '\n' || *c == '\r') { 
        if (*c == '\n') {
            lexer->pos_column = 1;
            lexer->pos_line += 1;
        } else {
            lexer->pos_column += 1;
        }
        ++c;
    }

    lexer->current = c;
}

static void lexer_add_token(enum token_type type,
                     struct minjson_lexer *lexer,
                     size_t len)
{
    struct minjson_token *token = \
        arena_allocator_alloc(lexer->aallocator,
                              DEFAULT_ALIGNMENT,
                              sizeof(struct minjson_token));

    token->type = type;
    token->lexeme = lexer->current;
    token->len = len;
    token->line = lexer->pos_line;
    token->column = lexer->pos_column;
    token->next = NULL;

    if (lexer->tk_tail)
        lexer->tk_tail->next = token;
    else
        lexer->tk_head = token;

    lexer->tk_tail = token;
}

static int lexer_add_string(struct minjson_lexer *lexer)
{
    lexer_advance(lexer, 1); /* One past the opening " */

    const char *curr = lexer->current;
    size_t len = 0;
    while (*curr != '"') {
        if (*curr == '\n' || *curr == '\0') {
            /* TODO: Report unterminated string */
            return -1;
        }

        ++curr;
        ++len;
    }

    lexer_add_token(TK_STRING, lexer, len);
    lexer_advance(lexer, len); /* One past the closing " */

    return 0;
}

int lexer_tokenize(struct minjson_lexer *lexer)
{
    while (*lexer->current != '\0') {
        switch (*lexer->current) {
            case '{':
                lexer_add_token(TK_OPEN_CB, lexer, 1);
                break;
            case '}':
                lexer_add_token(TK_CLOSE_CB, lexer, 1);
                break;
            case '[':
                lexer_add_token(TK_OPEN_SB, lexer, 1);
                break;
            case ']':
                lexer_add_token(TK_CLOSE_SB, lexer, 1);
                break;
            case ':':
                lexer_add_token(TK_COLON, lexer, 1);
                break;
            case ',':
                lexer_add_token(TK_DELIMITER, lexer, 1);
                break;
            case '"':
                if (lexer_add_string(lexer) == -1)
                    return -1;
                break;
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                lexer_skip_whitespaces(lexer);
                /* Skips advance logic below,
                 * lexer already points to non-whitespace after function above
                 */
                continue;
            default:
                /* TODO: reports unexpected token */
                return -1;
        }
        lexer_advance(lexer, 1);
    }

    return 0;
}

void lexer_print_tokens(struct minjson_lexer *lexer)
{
// enum token_type {
//     TK_STRING,
//     TK_NUMBER,
//     TK_NULL,
//     TK_BOOL,
//     TK_OPEN_CB,
//     TK_CLOSE_CB,
//     TK_OPEN_SB,
//     TK_CLOSE_SB,
//     TK_COLON,
//     TK_DELIMITER,
// };
    char *type_name = "";
    struct minjson_token *token = lexer->tk_head;

    while (token) {
        switch (token->type) {
            case TK_STRING: type_name = "TK_STRING"; break;
            case TK_NUMBER: type_name = "TK_NUMBER"; break;
            case TK_NULL: type_name = "TK_NULL"; break;
            case TK_BOOL: type_name = "TK_BOOL"; break;
            case TK_OPEN_CB: type_name = "TK_OPEN_CB"; break;
            case TK_CLOSE_CB: type_name = "TK_CLOSE_CB"; break;
            case TK_OPEN_SB: type_name = "TK_OPEN_SB"; break;
            case TK_CLOSE_SB: type_name = "TK_CLOSE_SB"; break;
            case TK_COLON: type_name = "TK_COLON"; break;
            case TK_DELIMITER: type_name = "TK_DELIMITER"; break;
        }
        printf("(%s) %.*s\n", type_name, (int)token->len, token->lexeme);
        token = token->next;
    }
}

struct minjson_lexer *lexer_new(struct arena_allocator *aa,
                                const char *raw_json)
{
    struct minjson_lexer *lexer = NULL;
    /* If aa belongs to caller, dont free on error */
    unsigned char free_aa = 0;
    if (!aa) {
        free_aa = 1;
        aa = arena_allocator_new(DEFAULT_ARENA_SIZE);
        if (!aa)
            return NULL;
    }

    lexer = arena_allocator_alloc(aa,
                                  DEFAULT_ALIGNMENT,
                                  sizeof(struct minjson_lexer));
    if (!lexer && free_aa)
        arena_allocator_destroy(aa);

    if (lexer) {
        lexer->aallocator = aa;
        lexer->current = raw_json;
        lexer->pos_column = 1;
        lexer->pos_line = 1;
        lexer->tk_head = NULL;
        lexer->tk_tail = NULL;
    }

    return lexer;
}

struct minjson *minjson_new(struct arena_allocator *aa)
{
    struct minjson *doc = NULL;
    /* If aa belongs to caller, dont free on error */
    unsigned char free_aa = 0;
    if (!aa) {
        free_aa = 1;
        aa = arena_allocator_new(DEFAULT_ARENA_SIZE);
        if (!aa)
            return NULL;
    }

    doc = arena_allocator_alloc(aa, DEFAULT_ALIGNMENT, sizeof(struct minjson));
    if (!doc && free_aa)
        arena_allocator_destroy(aa);

    if (doc) {
        doc->root = NULL;
        doc->aallocator = aa;
    }

    return doc;
}

/* raw_json must be null terminated */
struct minjson *minjson_parse(struct arena_allocator *doc_aa,
                              const char *raw_json)
{
    struct minjson *doc = NULL;
    struct minjson_lexer *lexer = NULL;
    /* If doc_aa belongs to caller, dont free on error */
    unsigned char free_doc_aa = 0;

    if (!doc_aa) {
        free_doc_aa = 1;
        doc_aa = arena_allocator_new(DEFAULT_ARENA_SIZE);
        if (!doc_aa)
            return NULL;
    }
    doc = minjson_new(doc_aa);
    if (!doc) {
        if (free_doc_aa)
            arena_allocator_destroy(doc_aa);
        return NULL;
    }

    lexer = lexer_new(NULL, raw_json);
    if (!lexer) {
        if (free_doc_aa)
            arena_allocator_destroy(doc_aa);
        return NULL;
    }

    /* Do something here */
        
    arena_allocator_destroy(lexer->aallocator);

    return doc;
}
