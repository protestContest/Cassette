NAME = cassette

BUILD_DIR := build
SRC_DIR := src
INC_DIR := include
LIB_DIR := lib
PREFIX := $(HOME)/.local
TARGET = ./$(NAME)
SHELL = bash

SRCS := $(shell find $(SRC_DIR) -name *.c -print)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

CC = clang
INCLUDE_FLAGS = -I$(PREFIX)/include -I$(SRC_DIR) -I$(INC_DIR) -include univ.h
WFLAGS = -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter
CFLAGS = -g -O0 $(WFLAGS) $(INCLUDE_FLAGS)
LDFLAGS = -L$(PREFIX)/lib -L$(LIB_DIR) -luniv

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: run
run: $(TARGET)
	@$(TARGET)

.PHONY: install
install: $(TARGET)
	mkdir -p $(PREFIX)/bin
	install $(TARGET) $(PREFIX)/bin

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

.PHONY: clean-deps
clean-deps:
	@rm -rf deps
	@rm -f include/univ.h
	@rm -f lib/libuniv.a
