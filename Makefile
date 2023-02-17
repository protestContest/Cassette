BUILD_DIR := build
SRC_DIR := src
INC_DIR := include
BIN_DIR := bin
PREFIX := $(HOME)/.local
TARGET = $(BIN_DIR)/rye

SRCS := $(shell find $(SRC_DIR) -name *.c -print)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

CC = clang
INCLUDE_FLAGS = -I$(INC_DIR) -I$(PREFIX)/include -include base.h
CFLAGS = -Os -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter $(INCLUDE_FLAGS)
LDFLAGS = -L$(PREFIX)/lib -luniv

$(TARGET): $(OBJS)
	$(MKDIR_P) $(dir $@)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: run
run: $(TARGET)
	@$(BIN_DIR)/$(TARGET)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)/*

MKDIR_P ?= mkdir -p
