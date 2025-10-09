#include <stdio.h>
#include <stdlib.h>

char *read_from_file(char *path)
{
    /* Code for reading a file from stack overflow */

    char *result = NULL;
    FILE *fp = fopen(path, "r");
    if (fp != NULL) {
        /* Go to the end of the file. */
        if (fseek(fp, 0L, SEEK_END) == 0) {
            /* Get the size of the file. */
            size_t new_len;
            long bufsize = ftell(fp);

            if (bufsize == -1) { /* Error */ }

            /* Allocate our buffer to that size. */
            result = malloc(bufsize + 1);

            /* Go back to the start of the file. */
            if (fseek(fp, 0L, SEEK_SET) != 0) { /* Error */ }

            /* Read the entire file into memory. */
            new_len = fread(result, sizeof(char), bufsize, fp);
            if ( ferror( fp ) != 0 ) {
                fputs("Error reading file", stderr);
            } else {
                result[new_len] = '\0';
            }
        }
        fclose(fp);
    }

    return result;

}

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
