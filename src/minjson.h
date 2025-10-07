#ifndef MINJSON_H
#define MINJSON_H

#if DEBUG

#include <assert.h>
#define ASSERT(expr) assert(expr)

#else

#define ASSERT(expr) ((void) 0)

#endif

#include "arena.h"


struct minjson_lexer;
struct minjson {
    struct arena_allocator *aallocator;
    struct minjson_value *root;
};
struct minjson_value;
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

/**
 * @brief   Creates a new minjson_lexer.
 *
 * Creates a minjson_lexer that holds context during the lexical analysis.
 *
 * @param   aa       An arena allocator where the lexer will live. If NULL, one
 *                   created and stored inside the lexer aallocator member. 
 * @param   raw_json Raw JSON (must be null terminated).
 *
 * @return  struct minjson_lexer * and NULL on failure
 */
struct minjson_lexer *minjson_lexer_new(struct arena_allocator *aa,
                                        const char *raw_json);

/**
 * @brief   Performs lexical analysis on the raw_json inside lexer. 
 *
 * Performs lexical analysis on the raw JSON that fills lexer with a stream
 * (linked list) of tokens.
 *
 * @param   lexer   The lexer itself that holds context of the lexing process
 *                  and pointer to the result tokens.
 * @param   error   Holds information if an error occured. Belongs to the caller.
 *
 * @return  0 on success and -1 on error.
 */
int minjson_lexer_tokenize(struct minjson_lexer *lexer,
                           struct minjson_error *error);

/**
 * @brief   Print lexer tokens
 */
void minjson_lexer_print_tokens(struct minjson_lexer *lexer);

/* ================== Public Facing API ================== */

/**
 * @brief   Performs the whole parsing operation on the given raw_json.
 *
 * The whole parsing operation includes creating lexer, doing lexical analysis,
 * and doing parsing itself.
 *
 * @param   doc_aa      The arena allocator where the result will live. If NULL,
 *                      it will be created and referenced by minjson aallocator member.
 * @param   raw_json    Raw JSON string (must be null terminated).
 * @param   error       Holds information if an error occured. Belongs to the caller.
 *
 * @return  struct minjson * that stores the JSON root value and its allocator.
 *          NULL on failure. To destroy struct minjson, simply destroy the
 *          aallocator with arena_allocator_destroy
 */
struct minjson *minjson_parse(struct arena_allocator *doc_aa,
                              const char *raw_json,
                              struct minjson_error *error);

/**
 * @brief   Creates a new minjson_error struct.
 */
struct minjson_error minjson_error_new(void);

/**
 * @brief   A wrapper to minjson_object_get.
 *
 * Just a convinient wrapper to minjson_object_get, assume the minjson root
 * value as an object.
 */
struct minjson_value *minjson_get(struct minjson *doc, const char *key);

/**
 * @brief   Retrieve a value from a JSON object by key.
 *
 * Searches the given JSON object (as value for convinience) for a key that
 * matches the given string and return its associated value.
 *
 * @param   value   A minjson_value of type object.
 * @param   key     A null terminated string.
 *
 * @return  If found, struct minjson_value of any type, else NULL.
 */
struct minjson_value *minjson_object_get(struct minjson_value *value,
                                         const char *key);

/**
 * @brief   Retrive a value from a JSON array by index.
 *
 * Returns the value located at the specified index within the given array.
 *
 * @param   value   A minjson_value of type array.
 * @param   index   Unsigned integer index.
 *
 * @return  If found, struct minjson_value of any type, else NULL.
 */
struct minjson_value *minjson_array_get(struct minjson_value *value,
                                        size_t index);

/* I believe these are pretty self explanatory */
size_t minjson_array_get_size(struct minjson_value *value);

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
