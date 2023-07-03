NAME = cassette

BUILD_DIR := build
SRC_DIR := src2
INC_DIR := include
LIB_DIR := lib
PREFIX := $(HOME)/.local
TARGET = ./$(NAME)
SHELL = bash

SRCS := $(shell find $(SRC_DIR) -name *.c -print)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

DEFINES += DEBUG_PARSE
# DEFINES += DEBUG_AST
# DEFINES += DEBUG_COMPILE
DEFINES += DEBUG_VM

CC = clang
INCLUDE_FLAGS = -I$(PREFIX)/include -I$(INC_DIR) -include univ.h
CFLAGS = -g -O0 -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter $(INCLUDE_FLAGS) $(DEFINES:%=-D%)
LDFLAGS = -L$(PREFIX)/lib -L$(LIB_DIR) -luniv

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: test
test: $(TARGET)
	@$(TARGET) test/test.csst

.PHONY: run
run: $(TARGET)
	@$(TARGET)

.PHONY: parse
parse: parse-check clean
	@gsi grammar/parse_gen.scm grammar/grammar.txt src/parse_table.h src/parse_syms.h

.PHONY: parse-check
parse-check:
	@! (bison -o grammar/bison -v <(cat grammar/tokens.y <(sed 's/→/:/g; s/ε//g; s/\$$/EOF/g; s/$$/;/g' grammar/grammar.txt)) 2>&1 | grep '.')

.PHONY: deps
deps:
	@rm -rf deps/univ
	@mkdir -p deps
	@git clone https://git.sr.ht/~zjm/univ/ deps/univ && cd deps/univ
	@make -C deps/univ && cp deps/univ/dist/libuniv.a lib/ && cp deps/univ/dist/univ.h include/

.PHONY:
readme:
	@mkdir -p $(BUILD_DIR)
	@pandoc -s --template support/template.html --metadata title=Cassette README.md > $(BUILD_DIR)/index.html

.PHONY: clean-all
clean-all: clean clean-deps

.PHONY: clean
clean:
	@rm -rf $(BUILD_DIR) $(TARGET)
	@rm -f grammar/bison*

.PHONY: clean-deps
clean-deps:
	@rm -rf deps
	@rm include/univ.h
	@rm lib/libuniv.a
