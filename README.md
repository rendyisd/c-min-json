# MinJSON
A minimalistic JSON parser and accessor library written in and for C.

**MinJSON** provides a small and and self-contained implementation of a simple and
minimalistic JSON parser. As of now it only allows lexical analysis, parsing, and
accessing JSON data. Builder is not (yet?) available, also no string literal escape
sequence yet.

---

## Features
- No external dependency required
- Thread safe
- Easily integrated into existing project
- Performs lexical analysis, recursive descent parsing, and error handling
- Simple and minimalistic (I guess)

---

## Building
You can use **MinJSON** in two ways:
- Copying the sources (except test.c) and headers into your project and compile them together with your code
- Run `make release` or `make debug` and copy the `.so` file and headers into your project and link it to your code
> **_NOTE:_** Run `make help` to see all avaiable make subcommands.

---

## Example
For example we have this following JSON:
```json
{
    "school": "A School",
    "students": [
        {
            "id": 67,
            "name": "Adrian",
            "scores": [78, 80, 75]
        },
        {
            "id": 68,
            "name": "Jane Doe",
            "scores": [90, 85, 92]
        },
        {
            "id": 69,
            "name": "Enrique",
            "scores": [78, 75, 80]
        }
    ]
}
```
To parse and then access the value above we can do:
```c
#include "arena.h"
#include "minjson.h"

int main(void)
{
    struct arena_allocator *aa = arena_allocator_new(DEFAULT_ARENA_SIZE);

    /* No, this function is not included in the library.
     * But basically, raw_json must be null terminated.*/
    char *raw_json = read_from_file("src/example.json");

    struct minjson_error error = minjson_error_new();
    struct minjson *root = minjson_parse(aa, raw_json, &error);
    if (!root) {
        arena_allocator_destroy(aa);
        fprintf(stderr, "Error: %s\n", error.message);
        exit(-1);
    }

    /* This is just a wrapper it takes the root value and assume its an object */
    struct minjson_value *school = minjson_get(root, "school");

    /* You should generally check value type before using unless
     * you are sure whats in it */
    if (minjson_value_is_string(school))
        printf("School name: %s\n\n", minjson_value_get_string(school));

    struct minjson_value *students = minjson_get(root, "students");
    size_t n_students = minjson_array_get_size(students);

    for (size_t i = 0; i < n_students; ++i) {
        struct minjson_value *student = minjson_array_get(students, i);

        struct minjson_value *student_id = minjson_object_get(student, "id");
        struct minjson_value *student_name = minjson_object_get(student, "name");
        struct minjson_value *student_scores = minjson_object_get(student, "scores");
        size_t n_scores = minjson_array_get_size(student_scores);

        /* Cast to int as JSON number is stored as double */
        printf("ID: %d\n", (int)minjson_value_get_number(student_id));
        printf("Name: %s\n", minjson_value_get_string(student_name));
        printf("Score: ");

        for (size_t j = 0; j < n_scores; ++j) {
            struct minjson_value *score = minjson_array_get(student_scores, j);
            printf("%d ", (int)minjson_value_get_number(score));
        }

        printf("\n\n");
    }

    arena_allocator_destroy(aa);
    free(raw_json);

    return 0;
}
```

---

## Things to improve
- Implement string literal escape sequence
- Test cases
- For now its parse only, no building JSON (yet? This fits my use case for now)
- Error handling is still really funky. Works fine but the diagnosis doesn't
  give the proper error info as its only sees the current token \
  NOTE: Using a look a head of 1 should fix this. But not a priority for now.
- Give user flexibility to use their own allocator, including the one that requires context
