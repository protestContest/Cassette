NAME = cassette
BUILD_DIR := build
SRC_DIR := src
# INC_DIR := include
# LIB_DIR := lib
DIST_DIR := dist
PREFIX := $(HOME)/.local
TARGET = $(DIST_DIR)/$(NAME)
SHELL = bash

SRCS := $(shell find $(SRC_DIR) -name *.c -print)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# DEBUG = -D DEBUG

CC = clang
INCLUDE_FLAGS = -isystem $(PREFIX)/include -include univ/base.h -I/opt/homebrew/include
CFLAGS = -g -O0 -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter $(INCLUDE_FLAGS) $(DEBUG)
LDFLAGS = -L$(PREFIX)/lib -L/opt/homebrew/lib -lbase -lcanvas -lwindow -lSDL2

$(TARGET): $(OBJS)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	@rm -rf $(BUILD_DIR) $(DIST_DIR)
	@rm -f bison*

.PHONY: run
run: $(TARGET)
	@$(TARGET)

.PHONY: parse
parse: parse-check clean
	@gsi src/parse_gen.scm grammar.txt src/parse_table.h src/parse_syms.h

.PHONY: parse-check
parse-check:
	@! (bison -o bison -v <(cat tokens.y <(sed 's/→/:/g; s/ε//g; s/\$$/EOF/g; s/$$/;/g' grammar.txt)) 2>&1 | grep '.')
