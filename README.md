# MinJSON
**MinJSON** provides a small and and self-contained implementation of a simple and
minimalistic JSON parser and accessor for C.

---

## Features
- Simple and minimalistic
- Thread safe
- No external dependency required
- Uses arena allocator
- Performs lexical analysis, recursive descent parsing, and provides error message
- UTF-8/Unicode compliant
- ANSI C compliant EXCEPT for the use of `snprintf` in `minjson_error_set`
> **_NOTE:_** If you really need to compile this with C89 standard use `sprintf`
(not reccomended) and increase `char message[128]` buffer size. Or just pass
error as NULL but you wont have access to error message.

---

## Documentation
The documentation for each public facing API is in `c-minjson.h`

---

## Building
You can use **MinJSON** in two ways:
- Copying the sources (except test.c) and headers into your project and compile them together with your code
- Run `make shared` or `make static` and link to your binary accordingly
> **_NOTE:_** Run `make help` to see all avaiable make subcommands.

---

## Example
For example we have this following JSON:
```json
{
    "school": "\uD83D\uDE80 A School \uD83D\uDE0A",
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

    struct minjson_error error;
    struct minjson *root;
    struct minjson_value *school;
    struct minjson_value *students;
    size_t n_students;
    size_t i;

    error = minjson_error_new();
    root = minjson_parse(aa, raw_json, &error);
    if (!root) {
        free(raw_json);
        arena_allocator_destroy(aa);
        fprintf(stderr, "Error: %s\n", error.message);
        exit(-1);
    }

    /* This is just a wrapper it takes the root value and assume its an object */
    school = minjson_get(root, "school");

    /* You should generally check value type before using unless
     * you are sure whats in it */
    if (minjson_value_is_string(school))
        printf("School name: %s\n\n", minjson_value_get_string(school));

    students = minjson_get(root, "students");
    n_students = minjson_array_get_size(students);
    for (i = 0; i < n_students; ++i) {
        struct minjson_value *student = minjson_array_get(students, i);

        struct minjson_value *student_id = minjson_object_get(student, "id");
        struct minjson_value *student_name = minjson_object_get(student, "name");
        struct minjson_value *student_scores = minjson_object_get(student, "scores");
        size_t n_scores = minjson_array_get_size(student_scores);
        size_t j;

        /* Cast to int as JSON number is stored as double */
        printf("ID: %d\n", (int)minjson_value_get_number(student_id));
        printf("Name: %s\n", minjson_value_get_string(student_name));
        printf("Score: ");

        for (j = 0; j < n_scores; ++j) {
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
Output:
```
School name: 🚀 A School 😊

ID: 67
Name: Adrian
Score: 78 80 75 

ID: 68
Name: Jane Doe
Score: 90 85 92 

ID: 69
Name: Enrique
Score: 78 75 80
```

---

## To be Implemented
These maybe implemented, maybe not:
- For now its parse only, no building JSON (yet? This fits my use case for now)
- Error handling is still really funky. Works fine but at some cases the error
  info doesn't represent the actual error.
- Give user flexibility to use their own allocator, including the one that
  requires context
