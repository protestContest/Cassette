TARGET = rye
BUILD_DIR := build
SRC_DIR := src
INC_DIR := include
LIB_DIR := lib

SRCS := $(shell find $(SRC_DIR) -name *.c -print)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

CC = clang
DBG = lldb
INCLUDE_FLAGS = -I$(INC_DIR) -include $(INC_DIR)/base.h
CFLAGS = -O0 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter $(INCLUDE_FLAGS)
LDFLAGS = -L$(LIB_DIR)

$(BUILD_DIR)/$(TARGET): $(OBJS)
	$(MKDIR_P) $(dir $@)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: run
run: $(BUILD_DIR)/$(TARGET)
	@$(BUILD_DIR)/$(TARGET)

.PHONY: debug
debug: $(BUILD_DIR)/$(TARGET)
	$(DBG) $(BUILD_DIR)/$(TARGET)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

MKDIR_P ?= mkdir -p
