NAME = rye
BUILD_DIR := build
SRC_DIR := src
INC_DIR := include
DIST_DIR := dist
PREFIX := $(HOME)/.local
TARGET = $(DIST_DIR)/$(NAME)

SRCS := $(shell find $(SRC_DIR) -name *.c -print)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

CC = clang
INCLUDE_FLAGS = -isystem $(PREFIX)/include -I$(INC_DIR) -include univ/base.h
CFLAGS = -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter $(INCLUDE_FLAGS)
LDFLAGS = -L$(PREFIX)/lib -lbase

$(TARGET): $(OBJS)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(DIST_DIR)

.PHONY: run
run: $(TARGET)
	@$(TARGET)
