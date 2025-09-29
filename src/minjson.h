#ifndef MINJSON_H
#define MINJSON_H

#include "arena.h"

struct minjson;

struct minjson *minjson_parse(struct arena_allocator *doc_aa,
                              const char *raw_json);

#endif
