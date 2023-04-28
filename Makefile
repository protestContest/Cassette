NAME = cassette
BUILD_DIR := build
SRC_DIR := src
# INC_DIR := include
# LIB_DIR := lib
DIST_DIR := .
PREFIX := $(HOME)/.local
TARGET = $(DIST_DIR)/$(NAME)
SHELL = bash

SRCS := $(shell find $(SRC_DIR) -name *.c -print)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

CC = clang
# DEFINES := DEBUG_PARSE DEBUG_AST DEBUG_COMPILE DEBUG_ASSEMBLE DEBUG_VM
DEFINES := DEBUG_PARSE DEBUG_AST DEBUG_VM
INCLUDE_FLAGS = -isystem $(PREFIX)/include -include univ/base.h -I/opt/homebrew/include
CFLAGS = -g -O0 -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter $(INCLUDE_FLAGS) $(DEFINES:%=-D%)
LDFLAGS = -L$(PREFIX)/lib -L/opt/homebrew/lib -lbase -lcanvas -lwindow -lSDL2

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	@rm -rf $(BUILD_DIR) $(TARGET)
	@rm -f grammar/bison*

.PHONY: test
test: $(TARGET)
	@$(TARGET) test/test.cassette

.PHONY: run
run: $(TARGET)
	@$(TARGET)

.PHONY: parse
parse: parse-check clean
	@gsi grammar/parse_gen.scm grammar/grammar.txt src/parse_table.h src/parse_syms.h

.PHONY: parse-check
parse-check:
	@! (bison -o grammar/bison -v <(cat grammar/tokens.y <(sed 's/→/:/g; s/ε//g; s/\$$/EOF/g; s/$$/;/g' grammar/grammar.txt)) 2>&1 | grep '.')
