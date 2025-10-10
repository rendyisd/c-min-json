CC = gcc
AR = ar
CFLAGS = -Wall -Wextra -Wpedantic

DEBUG_FLAGS = $(CFLAGS) \
			  -fsanitize=address \
			  -fsanitize=undefined \
			  -fno-sanitize-recover=all \
			  -g3 -O0 -DDEBUG \
			  -fno-omit-frame-pointer

LDFLAGS_DEBUG = -fsanitize=address \
				-fsanitize=undefined \
				-fno-sanitize-recover=all

RELEASE_FLAGS = $(CFLAGS) \
				-O3 -DNDEBUG \
				-fomit-frame-pointer
				
BUILD_DIR = build
DEBUG_DIR = $(BUILD_DIR)/debug
RELEASE_DIR = $(BUILD_DIR)/release


SRCS_LIB = src/minjson.c src/arena.c

SHARED_OBJS_DEBUG = $(patsubst src/%.c, $(DEBUG_DIR)/shared/%.o, $(SRCS_LIB))
SHARED_OBJS_RELEASE = $(patsubst src/%.c, $(RELEASE_DIR)/shared/%.o, $(SRCS_LIB))

STATIC_OBJS_DEBUG = $(patsubst src/%.c, $(DEBUG_DIR)/static/%.o, $(SRCS_LIB))
STATIC_OBJS_RELEASE = $(patsubst src/%.c, $(RELEASE_DIR)/static/%.o, $(SRCS_LIB))

SRC_TEST = src/test.c
OBJ_TEST = $(patsubst src/%.c, $(BUILD_DIR)/%.o, $(SRC_TEST))


SHARED_LIB_DEBUG = $(DEBUG_DIR)/shared/libminjson.so
SHARED_LIB_RELEASE = $(RELEASE_DIR)/shared/libminjson.so

STATIC_LIB_DEBUG = $(DEBUG_DIR)/static/libminjson.a
STATIC_LIB_RELEASE = $(RELEASE_DIR)/static/libminjson.a

TARGET_TEST = $(BUILD_DIR)/minjson_test

all: lib test

lib: shared static

shared: $(SHARED_LIB_DEBUG) $(SHARED_LIB_RELEASE)

static: $(STATIC_LIB_DEBUG) $(STATIC_LIB_RELEASE)

debug: $(SHARED_LIB_DEBUG) $(STATIC_LIB_DEBUG)

release: $(SHARED_LIB_RELEASE) $(STATIC_LIB_RELEASE)

test: $(TARGET_TEST) $(STATIC_LIB_DEBUG)

# libminjson shared
$(SHARED_LIB_DEBUG): $(SHARED_OBJS_DEBUG)
	$(CC) -shared $(SHARED_OBJS_DEBUG) -o $(SHARED_LIB_DEBUG)

$(SHARED_LIB_RELEASE): $(SHARED_OBJS_RELEASE)
	$(CC) -shared $(SHARED_OBJS_RELEASE) -o $(SHARED_LIB_RELEASE)

# libminjson static
$(STATIC_LIB_DEBUG): $(STATIC_OBJS_DEBUG)
	$(AR) rcs $@ $^

$(STATIC_LIB_RELEASE): $(STATIC_OBJS_RELEASE)
	$(AR) rcs $@ $^

# objs shared
build/debug/shared/%.o: src/%.c | $(DEBUG_DIR)/shared
	$(CC) $(DEBUG_FLAGS) -c -fPIC $< -o $@

build/release/shared/%.o: src/%.c | $(RELEASE_DIR)/shared
	$(CC) $(RELEASE_FLAGS) -c -fPIC $< -o $@

# objs static
build/debug/static/%.o: src/%.c | $(DEBUG_DIR)/static
	$(CC) $(DEBUG_FLAGS) -c $< -o $@

build/release/static/%.o: src/%.c | $(RELEASE_DIR)/static
	$(CC) $(RELEASE_FLAGS) -c $< -o $@

# test
$(TARGET_TEST): $(OBJ_TEST) $(STATIC_LIB_DEBUG)
	$(CC) $(LDFLAGS_DEBUG) $< -L$(DEBUG_DIR)/static -lminjson -o $@

$(OBJ_TEST): $(SRC_TEST) 
	$(CC) $(DEBUG_FLAGS) -c $< -o $@

# directory
$(DEBUG_DIR)/shared:
	mkdir -p $@

$(DEBUG_DIR)/static:
	mkdir -p $@

$(RELEASE_DIR)/shared:
	mkdir -p $@

$(RELEASE_DIR)/static:
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)

compile_commands:
	bear -- make all

help:
	@echo "Available commands:"
	@echo "    all              - Build both SHARED and STATIC lib for DEBUG and RELEASE config and TEST"
	@echo "    lib              - Build both SHARED and STATIC lib for DEBUG and RELEASE config"
	@echo "    shared           - Build SHARED lib for DEBUG and RELEASE config"
	@echo "    static           - Build STATIC lib for DEBUG and RELEASE config"
	@echo "    debug            - Build both SHARED and STATIC lib for DEBUG config"
	@echo "    release          - Build both SHARED and STATIC lib for RELEASE config"
	@echo "    test             - Build TEST executable. Strictly using STATIC lib DEBUG"
	@echo "    compile_commands - Generate compile_commands.json"
	@echo "    clean            - Remove build directory"
	@echo "    help             - Print this message"

.PHONY: all lib shared static debug release test clean help
