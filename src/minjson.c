#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "minjson.h"

enum minjson_type {
    MJ_OBJECT,
    MJ_ARRAY,
    MJ_STRING,
    MJ_NUMBER,
    MJ_TRUE,
    MJ_FALSE,
    MJ_NULL
};

struct minjson;

struct minjson_value {
    enum minjson_type type;
    union {
        struct minjson_object *object;
        struct minjson_array *array;
        char *string;
        double number; /* Cast to int if needed */
        int boolean;
    } value;
};


struct minjson_object_entry {
    char *key;
    struct minjson_value *value;
    struct minjson_object_entry *next;
};

/* Any number of minjson_object_entry */
struct minjson_object { /* This itself is value of type object */
    struct minjson_object_entry *head;
    struct minjson_object_entry *tail;
    size_t len;
};

struct minjson_array_entry {
    struct minjson_value *value;
    struct minjson_array_entry *next;
};

/* Any number of value(any type) */
struct minjson_array { /* This itself is value of type array */
    struct minjson_array_entry *head;
    struct minjson_array_entry *tail;
    size_t len;
};

enum token_type {
    TK_STRING,
    TK_NUMBER,
    TK_NULL,
    TK_TRUE,
    TK_FALSE,
    TK_OPEN_CB,
    TK_CLOSE_CB,
    TK_OPEN_SB,
    TK_CLOSE_SB,
    TK_COLON,
    TK_DELIMITER
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

static int is_digit(const char c)
{
    return c >= '0' && c <= '9';
}

static int is_onenine(const char c)
{
    return c >= '1' && c <= '9';
}

static int is_valid_literal_terminator(const char c)
{
    return c == '\0' || c == ' ' || c == '\t' || c == '\n' ||
           c == '\r' || c == ',' || c == '}'  || c == ']';
}

static void minjson_error_set(struct minjson_error *error,
                              enum minjson_error_code code,
                              const char *fmt,
                              size_t line,
                              size_t column)
{
    if (!error)
        return;

    error->code = code;
    error->line = line;
    error->column = column;

    if (code == MJ_ERR_ALLOCATOR)  {
        strncpy(error->message, fmt, sizeof(error->message));
    } else {
        snprintf(error->message, sizeof(error->message), fmt, line, column); 
    }
}

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
            lexer->pos_line += 1;
            lexer->pos_column = 1;
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
    const char *curr;
    size_t len = 0;

    lexer_advance(lexer, 1); /* One past the opening " */

    curr = lexer->current;
    while (*curr != '"') {
        if (*curr == '\n' || *curr == '\0')
            return -1;

        ++curr;
        ++len;
    }

    lexer_add_token(TK_STRING, lexer, len);
    lexer_advance(lexer, len); /* On " closing */

    return 0;
}

/* JSON number finite automaton states */
/* I got ChatGPT to list the states, don't judge me :( */
enum number_fa_state {
    Q0_START,
    Q1_OPT_MINUS,
    Q2_INITIAL_ZERO,        /* Accept */
    Q3_DIGIT,               /* Accept */
    Q4_FRAC,
    Q5_FRAC_DIGIT,          /* Accept */
    Q6_EXP,
    Q7_OPT_MINUS_PLUS,
    Q8_EXP_DIGIT            /* Accept */
};
static int lexer_add_number(struct minjson_lexer *lexer)
{
    const char *curr = lexer->current;
    size_t len = 0;

    /* either use strtod() to validate (no idea how to save double
     * as this is a lexer) OR handroll your own regular language parser (below) */
    enum number_fa_state state = Q0_START;

    while (!is_valid_literal_terminator(*curr)) {
        switch (state) {
            case Q0_START:
                if (*curr == '-') state = Q1_OPT_MINUS;
                else if (*curr == '0') state = Q2_INITIAL_ZERO;
                else if (is_onenine(*curr)) state = Q3_DIGIT;
                else return -1; 
                break;
            case Q1_OPT_MINUS:
                if (*curr == '0') state = Q2_INITIAL_ZERO;
                else if (is_onenine(*curr)) state = Q3_DIGIT;
                else return -1;
                break;
            case Q2_INITIAL_ZERO:
                if (*curr == '.') state = Q4_FRAC;
                else if (*curr == 'e' || *curr == 'E') state = Q6_EXP;
                else return -1;
                break;
            case Q3_DIGIT:
                if (is_digit(*curr)) state = Q3_DIGIT;
                else if (*curr == '.') state = Q4_FRAC;
                else if (*curr == 'e' || *curr == 'E') state = Q6_EXP;
                else return -1;
                break;
            case Q4_FRAC:
                if (is_digit(*curr)) state = Q5_FRAC_DIGIT;
                else return -1;
                break;
            case Q5_FRAC_DIGIT:
                if (is_digit(*curr)) state = Q5_FRAC_DIGIT;
                else if (*curr == 'e' || *curr == 'E') state = Q6_EXP;
                else return -1;
                break;
            case Q6_EXP:
                if (*curr == '+' || *curr == '-') state = Q7_OPT_MINUS_PLUS;
                else if (is_digit(*curr)) state = Q8_EXP_DIGIT;
                else return -1;
                break;
            case Q7_OPT_MINUS_PLUS:
                if (is_digit(*curr)) state = Q8_EXP_DIGIT;
                else return -1;
                break;
            case Q8_EXP_DIGIT:
                if (is_digit(*curr)) state = Q8_EXP_DIGIT;
                else return -1;
                break;
            default:
                return -1;
                break;
        }
        ++curr;
        ++len;
    }
    /* Accept states */
    if (!(state == Q2_INITIAL_ZERO || state == Q3_DIGIT ||
          state == Q5_FRAC_DIGIT || state == Q8_EXP_DIGIT))
        return -1;

    lexer_add_token(TK_NUMBER, lexer, len);
    lexer_advance(lexer, len - 1); /* So this doesn't go pass the delimiter */

    return 0;
}

/* I'm not sure what to call these (true, false, null)
 * identifier? keyword? reserved word? literal? whatever...*/
static int lexer_match_literal(struct minjson_lexer *lexer,
                                  enum token_type type,
                                  const char *literal,
                                  size_t len)
{
    /* No way to sanity check the type and the matched literal
     * Just dont use this for stupid thing, I guess.*/
    char char_after_literal;

    ASSERT(strlen(literal) == len);

    if (strncmp(lexer->current, literal, len) != 0)
        return -1;

    /* This is for catching lexer error early on, for cases like
     * truenull, truefalse, and the like*/
    char_after_literal = lexer->current[len];
    if (!is_valid_literal_terminator(char_after_literal))
        return -1;

    lexer_add_token(type, lexer, len);
    lexer_advance(lexer, len - 1);

    return 0;
}

int minjson_lexer_tokenize(struct minjson_lexer *lexer,
                           struct minjson_error *error)
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
                    goto fail_string;
                break;
            case 't':
                if (lexer_match_literal(lexer, TK_TRUE, "true", 4) == -1)
                    goto fail_literal;
                break;
            case 'f':
                if (lexer_match_literal(lexer, TK_FALSE, "false", 5) == -1)
                    goto fail_literal;
                break;
            case 'n':
                if (lexer_match_literal(lexer, TK_NULL, "null", 4) == -1)
                    goto fail_literal;
                break;
            case '0': case '1': case '2': case '3': case '4': case '5':
            case '6': case '7': case '8': case '9': case '-':
                if (lexer_add_number(lexer) == -1)
                    goto fail_number;
                break;
            case ' ': case '\t': case '\n': case '\r':
                lexer_skip_whitespaces(lexer);
                /* Skips advance logic below,
                 * lexer now points to non-whitespace after function above */
                continue;
            default:
                goto fail_token;
        }
        lexer_advance(lexer, 1);
    }

    return 0;

fail_string:
    minjson_error_set(error,
                      MJ_ERR_STRING,
                      "unterminated string at line %zu, column %zu",
                      lexer->pos_line,
                      lexer->pos_column);
    return -1;

fail_literal:
    minjson_error_set(error,
                      MJ_ERR_LITERAL,
                      "unexpected literal at line %zu, column %zu",
                      lexer->pos_line,
                      lexer->pos_column);
    return -1;

fail_number:
    minjson_error_set(error,
                      MJ_ERR_NUMBER,
                      "invalid number sequence at line %zu, column %zu",
                      lexer->pos_line,
                      lexer->pos_column);
    return -1;

fail_token:
    minjson_error_set(error,
                      MJ_ERR_TOKEN,
                      "unexpected token at line %zu, column %zu",
                      lexer->pos_line,
                      lexer->pos_column);
    return -1;
}

void minjson_lexer_print_tokens(struct minjson_lexer *lexer)
{
    char *type_name = "";
    struct minjson_token *token = lexer->tk_head;

    while (token) {
        switch (token->type) {
            case TK_STRING: type_name = "TK_STRING"; break;
            case TK_NUMBER: type_name = "TK_NUMBER"; break;
            case TK_NULL: type_name = "TK_NULL"; break;
            case TK_TRUE: type_name = "TK_TRUE"; break;
            case TK_FALSE: type_name = "TK_FALSE"; break;
            case TK_OPEN_CB: type_name = "TK_OPEN_CB"; break;
            case TK_CLOSE_CB: type_name = "TK_CLOSE_CB"; break;
            case TK_OPEN_SB: type_name = "TK_OPEN_SB"; break;
            case TK_CLOSE_SB: type_name = "TK_CLOSE_SB"; break;
            case TK_COLON: type_name = "TK_COLON"; break;
            case TK_DELIMITER: type_name = "TK_DELIMITER"; break;
            default: type_name = "ERROR_UNKNOWN_TOKEN"; break;
        }
        printf("(%s) %.*s\n", type_name, (int)token->len, token->lexeme);
        token = token->next;
    }
}

struct minjson_lexer *minjson_lexer_new(struct arena_allocator *aa,
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
        lexer->pos_line = 1;
        lexer->pos_column = 1;
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

/* This should always belong to the caller so just return the whole struct */
struct minjson_error minjson_error_new(void)
{
    struct minjson_error error;
    error.code = MJ_CODE_OK;
    error.line = 0;
    error.column = 0;

    return error;
}

struct minjson_value *minjson_parse_value(struct minjson_token **token,
                                          struct arena_allocator *aa,
                                          struct minjson_error *error);

int minjson_object_create_entry(struct minjson_object *object,
                                struct arena_allocator *aa,
                                char *key,
                                struct minjson_value *value)
{
    struct minjson_object_entry *entry = \
        arena_allocator_alloc(aa,
                              DEFAULT_ALIGNMENT,
                              sizeof(struct minjson_object_entry));
    if (!entry)
        return -1;

    entry->key = key;
    entry->value = value;
    entry->next = NULL;
    if (object->tail)
        object->tail->next = entry;
    else
        object->head = entry;
    object->tail = entry;
    ++object->len;

    return 0;
}

struct minjson_object *minjson_parse_object(struct minjson_token **token,
                                            struct arena_allocator *aa,
                                            struct minjson_error *error)
{
    struct minjson_token *current = *token;
    struct minjson_object *object = \
        arena_allocator_alloc(aa,
                              DEFAULT_ALIGNMENT,
                              sizeof(struct minjson_object));
    if (!object)
        goto fail_allocator;
    object->head = NULL;
    object->tail =  NULL;
    object->len = 0;

    if (current->next && current->next->type == TK_CLOSE_CB) {
        current = current->next;
        *token = current;
        return object;
    }

    /* string -> colon -> value, if delimiter repeat cycle, else expect '}' */
    while (current && current->type != TK_CLOSE_CB) {
        char *key;
        struct minjson_value *value;
        current = current->next;

        /* KEY */
        if (!current || current->type != TK_STRING)
            goto fail_expected_string;
        key = arena_allocator_alloc(aa, DEFAULT_ALIGNMENT, current->len + 1);
        memcpy(key, current->lexeme, current->len);
        key[current->len] = '\0';

        /* COLON */
        current = current->next;
        if (!current || current->type != TK_COLON)
            goto fail_expected_colon;

        /* VALUE */
        current = current->next;
        if(!current)
            goto fail_expected_value;
        value = minjson_parse_value(&current, aa, error);
        if (!value)
            return NULL;

        if (minjson_object_create_entry(object, aa, key, value) == -1)
            goto fail_allocator;

        /* DELIMITER */
        if (!current || current->type != TK_DELIMITER)
            break;
    }

    if (!current || current->type != TK_CLOSE_CB)
        goto fail_expected_closing_brace;

    *token = current;

    return object;

fail_allocator: 
    minjson_error_set(error, MJ_ERR_ALLOCATOR, "memory allocator failed", 0, 0);
    return NULL;

fail_expected_string:
    minjson_error_set(error,
                      MJ_ERR_OBJECT,
                      "syntax error, expected string at line %zu, column %zu",
                      current->line, current->column);
    return NULL;

fail_expected_colon:
    minjson_error_set(error,
                      MJ_ERR_OBJECT,
                      "syntax error, expected ':' at line %zu, column %zu",
                      current->line, current->column);
    return NULL;

fail_expected_value:
    minjson_error_set(error,
                      MJ_ERR_OBJECT,
                      "syntax error, expected value at line %zu, column %zu",
                      current->line, current->column);
    return NULL;

fail_expected_closing_brace:
    if (!current) {
        minjson_error_set(error,
                          MJ_ERR_OBJECT,
                          "syntax error, unexpected EOF",
                          0, 0);
    } else {
        minjson_error_set(error,
                          MJ_ERR_OBJECT,
                          "syntax error, expected '}' at end of object, line %zu, column %zu",
                          current->line, current->column);
    }
    return NULL;
}

int minjson_array_create_entry(struct minjson_array* array,
                               struct arena_allocator* aa,
                               struct minjson_value *value)
{
    struct minjson_array_entry *entry = \
        arena_allocator_alloc(aa,
                              DEFAULT_ALIGNMENT,
                              sizeof(struct minjson_array_entry));
    if(!entry)
        return -1;
    entry->value = value;
    entry->next = NULL;
    if (array->tail)
        array->tail->next = entry;
    else
        array->head = entry;
    array->tail = entry;
    ++array->len;

    return 0;
}
 
struct minjson_array *minjson_parse_array(struct minjson_token **token,
                                          struct arena_allocator *aa,
                                          struct minjson_error *error)
{
    struct minjson_token *current = *token;
    struct minjson_array *array = \
        arena_allocator_alloc(aa,
                              DEFAULT_ALIGNMENT,
                              sizeof(struct minjson_array));
    if (!array)
        goto fail_allocator;

    array->head = NULL;
    array->tail = NULL;
    array->len = 0;

    if (current->next && current->next->type == TK_CLOSE_SB) {
        current = current->next;
        *token = current;
        return array;
    }

    while (current && current->type != TK_CLOSE_SB) {
        struct minjson_value *value;
        /* VALUE */
        current = current->next;
        if(!current)
            goto fail_expected_value;
        value = minjson_parse_value(&current, aa, error);
        if (!value)
            return NULL;

        if (minjson_array_create_entry(array, aa, value) == -1)
            goto fail_allocator;

        /* DELIMITER */
        if (!current || current->type != TK_DELIMITER)
            break;
    }

    if (!current || current->type != TK_CLOSE_SB)
        goto fail_expected_closing_bracket;

    *token = current;

    return array;

fail_allocator: 
    minjson_error_set(error, MJ_ERR_ALLOCATOR, "memory allocator failed", 0, 0);
    return NULL;

fail_expected_value:
    minjson_error_set(error,
                      MJ_ERR_OBJECT,
                      "syntax error, expected value at line %zu, column %zu",
                      current->line, current->column);
    return NULL;

fail_expected_closing_bracket:
    if (!current) {
        minjson_error_set(error,
                          MJ_ERR_OBJECT,
                          "syntax error, unexpected EOF",
                          0, 0);
    } else {
        minjson_error_set(error,
                          MJ_ERR_OBJECT,
                          "syntax error, expected ']' at end of array, line %zu, column %zu",
                          current->line, current->column);
    }
    return NULL;
}

struct minjson_value *minjson_parse_value(struct minjson_token **token,
                                          struct arena_allocator *aa,
                                          struct minjson_error *error)
{
    struct minjson_value *val = NULL;
    struct minjson_token *current = *token;
    char *temp = NULL;
    if (current) {
        val = arena_allocator_alloc(aa,
                                     DEFAULT_ALIGNMENT,
                                     sizeof(struct minjson_value));
        if (!val)
            goto fail_allocator;

        switch (current->type) {
            case TK_OPEN_CB:
                val->type = MJ_OBJECT;
                val->value.object = minjson_parse_object(&current, aa, error);
                if (!val->value.object)
                    return NULL;
                break;
            case TK_OPEN_SB:
                val->type = MJ_ARRAY;
                val->value.array = minjson_parse_array(&current, aa, error);
                if (!val->value.array)
                    return NULL;
                break;
            case TK_TRUE:
                val->type = MJ_TRUE;
                val->value.boolean = 1;
                break;
            case TK_FALSE:
                val->type = MJ_FALSE;
                val->value.boolean = 0;
                break;
            case TK_NULL:
                val->type = MJ_NULL;
                val->value.boolean = 0;
                break;
            case TK_NUMBER: 
                temp = arena_allocator_alloc(aa,
                                             DEFAULT_ALIGNMENT,
                                             current->len + 1);
                if (!temp)
                    goto fail_allocator;

                val->type = MJ_NUMBER;

                memcpy(temp, current->lexeme, current->len);
                temp[current->len] = '\0';

                val->value.number = strtod(temp, NULL);
                break;
            case TK_STRING:
                temp = arena_allocator_alloc(aa,
                                             DEFAULT_ALIGNMENT,
                                             current->len + 1);
                if (!temp)
                    goto fail_allocator;

                val->type = MJ_STRING;

                memcpy(temp, current->lexeme, current->len);
                temp[current->len] = '\0';

                val->value.string = temp;
                break;
            default:
                goto fail_unexpected_token;
                break;
        }
        current = current->next;
    } else {
        goto fail_expected_value;
    }

    *token = current;

    return val;

fail_allocator:
    minjson_error_set(error, MJ_ERR_ALLOCATOR, "memory allocator failed", 0, 0);
    return NULL;

/* Purely to handle an empty JSON, object and array have their own */
fail_expected_value: 
    minjson_error_set(error,
                      MJ_ERR_VALUE,
                      "syntax error, expected value at line%zu, column %zu",
                      1,
                      1);
    return NULL;

fail_unexpected_token:
    minjson_error_set(error,
                      MJ_ERR_TOKEN,
                      "syntax error, unexpected token at line %zu, column %zu",
                      current->line,
                      current->column);
    return NULL;
}

/* ================== Public Facing API ================== */

/* raw_json must be null terminated */
struct minjson *minjson_parse(struct arena_allocator *doc_aa,
                              const char *raw_json,
                              struct minjson_error *error)
{
    struct minjson *doc = NULL;
    struct minjson_lexer *lexer = NULL;
    struct minjson_token *current = NULL;
    /* If doc_aa belongs to caller, dont free on error */
    unsigned char free_doc_aa = 0;

    if (!doc_aa) {
        free_doc_aa = 1;
        doc_aa = arena_allocator_new(DEFAULT_ARENA_SIZE);
        if (!doc_aa)
            goto fail_allocator;
    }
    doc = minjson_new(doc_aa);
    if (!doc)
        goto fail_allocator;

    lexer = minjson_lexer_new(NULL, raw_json);
    if (!lexer)
        goto fail_allocator;

    /* Lexical analysis here */
    if (minjson_lexer_tokenize(lexer, error) == -1)
        goto fail;

    /* Parsing here */
    current = lexer->tk_head;
    doc->root = minjson_parse_value(&current, doc_aa, error);
    if (!doc->root)
        goto fail;

    /* There shouldn't be anymore token */
    if (current)
        goto fail_unexpected_token;
        
    arena_allocator_destroy(lexer->aallocator);

    return doc;

fail_allocator:
    minjson_error_set(error, MJ_ERR_ALLOCATOR, "memory allocator failed", 0, 0);
    if (free_doc_aa)
        arena_allocator_destroy(doc_aa);
    if (lexer)
        arena_allocator_destroy(lexer->aallocator);
    return NULL;

fail_unexpected_token:
    minjson_error_set(error,
                      MJ_ERR_TOKEN,
                      "syntax error, unexpected token at line %zu, column %zu",
                      current->line,
                      current->column);
    if (lexer)
        arena_allocator_destroy(lexer->aallocator);
    if (free_doc_aa)
        arena_allocator_destroy(doc_aa);
    return NULL;

fail:
    if (free_doc_aa)
        arena_allocator_destroy(doc_aa);
    if (lexer)
        arena_allocator_destroy(lexer->aallocator);
    return NULL;

}

struct minjson_value *minjson_get(struct minjson *doc, const char *key)
{
    struct minjson_value *value;

    if (!doc || doc->root->type != MJ_OBJECT)
        return NULL;

    value = minjson_object_get(doc->root, key);

    return value;
}

struct minjson_value *minjson_object_get(struct minjson_value *value,
                                         const char *key)
{
    struct minjson_object_entry *entry;

    if (!value || !key || !minjson_value_is_object(value))
        return NULL;
    
    entry = value->value.object->head;
    for (; entry; entry = entry->next)
        if (strcmp(key, entry->key) == 0)
            return entry->value;
 
    return NULL;
}

struct minjson_value *minjson_array_get(struct minjson_value *value,
                                        size_t index)
{
    struct minjson_array *array;
    struct minjson_array_entry *entry;
    size_t i;

    if (!value || !minjson_value_is_array(value))
        return NULL;

    array = minjson_value_get_array(value);
    if (index >= array->len)
        return NULL;

    entry = array->head;
    for (i = 0; entry && i != index; ++i)
        entry = entry->next;

    return entry ? entry->value : NULL;
}

size_t minjson_array_get_size(struct minjson_value* value)
{
    ASSERT(minjson_value_is_array(value));

    /* I really dont want to return 0 on error as it would introduce bug */
    return value->value.array->len;
}

int minjson_value_is_null(struct minjson_value *value)
{
    return (value && value->type == MJ_NULL);
}

int minjson_value_is_number(struct minjson_value *value)
{
    return (value && value->type == MJ_NUMBER);
}
double minjson_value_get_number(struct minjson_value *value)
{
    return value->value.number;
}

int minjson_value_is_string(struct minjson_value *value)
{
    return (value && value->type == MJ_STRING);
}
char *minjson_value_get_string(struct minjson_value *value)
{
    return value->value.string;
}

int minjson_value_is_bool(struct minjson_value *value)
{
    return (value && (value->type == MJ_TRUE || value->type == MJ_FALSE));
}
int minjson_value_get_bool(struct minjson_value *value)
{
    return value->value.boolean;
}

int minjson_value_is_array(struct minjson_value *value)
{
    return (value && value->type == MJ_ARRAY);
}
struct minjson_array *minjson_value_get_array(struct minjson_value *value)
{
    return value->value.array;
}

int minjson_value_is_object(struct minjson_value *value)
{
    return (value && value->type == MJ_OBJECT);
}
struct minjson_object *minjson_value_get_object(struct minjson_value *value)
{
    return value->value.object;
}
