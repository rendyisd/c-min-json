CC = gcc
AR = ar
CFLAGS = -Wall -Wextra -Wpedantic

DEBUG_FLAGS = $(CFLAGS) \
			  -fsanitize=address \
			  -fsanitize=undefined \
			  -fno-sanitize-recover=all \
			  -g3 -O0 -DDEBUG \
			  -fno-omit-frame-pointer

RELEASE_FLAGS = $(CFLAGS) \
				-O3 -DNDEBUG \
				-fomit-frame-pointer
				
BUILD_DIR = build
DEBUG_DIR = $(BUILD_DIR)/debug
RELEASE_DIR = $(BUILD_DIR)/release

SRCS_LIB = src/minjson.c src/arena.c
OBJS_DEBUG = $(patsubst src/%.c, $(DEBUG_DIR)/%.o, $(SRCS_LIB))
OBJS_RELEASE = $(patsubst src/%.c, $(RELEASE_DIR)/%.o, $(SRCS_LIB))

SRC_TEST = src/test.c
OBJ_TEST = $(patsubst src/%.c, $(BUILD_DIR)/%.o, $(SRC_TEST))

LIB_DEBUG = $(DEBUG_DIR)/libminjson.a
LIB_RELEASE = $(RELEASE_DIR)/libminjson.a
TARGET_TEST = $(BUILD_DIR)/minjson_test

all: lib-debug lib-release test

lib-debug: $(LIB_DEBUG)
lib-release: $(LIB_RELEASE)
test: $(TARGET_TEST)

# libminjson debug
$(LIB_DEBUG): $(OBJS_DEBUG)
	$(AR) rcs $(LIB_DEBUG) $(OBJS_DEBUG)

build/debug/%.o: src/%.c | $(DEBUG_DIR)
	$(CC) $(DEBUG_FLAGS) -c $< -o $@

$(DEBUG_DIR)/:
	mkdir -p $(DEBUG_DIR)/

# libminjson release
$(LIB_RELEASE): $(OBJS_RELEASE)
	$(AR) rcs $(LIB_RELEASE) $(OBJS_RELEASE)

build/release/%.o: src/%.c | $(RELEASE_DIR)
	$(CC) $(RELEASE_FLAGS) -c $< -o $@

# test (strictly only uses lib on debug configuration)
$(TARGET_TEST): $(OBJ_TEST)
	$(CC) $(DEBUG_FLAGS) $< -L$(DEBUG_DIR) -lminjson -o $@

$(OBJ_TEST): $(SRC_TEST) $(LIB_DEBUG)
	$(CC) $(DEBUG_FLAGS) -c $< -o $@

$(DEBUG_DIR):
	mkdir -p $(DEBUG_DIR)

$(RELEASE_DIR):
	mkdir -p $(RELEASE_DIR)

clean:
	rm -rf $(BUILD_DIR)

compile_commands:
	bear -- make all

help:
	@echo "Available commands:"
	@echo "    all              - Build both lib config and test"
	@echo "    lib-debug        - Build lib with debug config"
	@echo "    lib-release      - Build lib with release config"
	@echo "    test             - Build test executable. Only testing lib debug"
	@echo "    compile_commands - Generate compile_commands.json"
	@echo "    clean            - Remove build directory"
	@echo "    help             - Print this message"

.PHONY: all lib-debug lib-release test clean help
