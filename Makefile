TARGET = rye
BUILD_DIR := build
SRC_DIR := src
INC_DIR := include
LIB_DIR := lib
BIN_DIR := bin

SRCS := $(shell find $(SRC_DIR) -name *.c -print)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

CC = clang
DBG = lldb
INCLUDE_FLAGS = -I$(INC_DIR) -include $(INC_DIR)/base.h
CFLAGS = -O0 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter $(INCLUDE_FLAGS)
LDFLAGS = -L$(LIB_DIR) -lpthread

TEST_ARG = test.rye

$(BIN_DIR)/$(TARGET): $(OBJS)
	$(MKDIR_P) $(dir $@)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: run
run: $(BIN_DIR)/$(TARGET)
	@$(BIN_DIR)/$(TARGET)

.PHONY: test
test: $(BIN_DIR)/$(TARGET)
	@$(BIN_DIR)/$(TARGET) $(TEST_ARG)

.PHONY: debug
debug: $(BIN_DIR)/$(TARGET)
	$(DBG) $(BIN_DIR)/$(TARGET)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

.PHONY: watch
watch: $(BIN_DIR)/$(TARGET)
	@fswatch -0 -o $(SRC_DIR) | xargs -0 -n1 -I{} make test

MKDIR_P ?= mkdir -p
