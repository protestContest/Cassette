NAME = mem

TARGET = ../../lib/lib$(NAME).a
HEADER = $(NAME).h

SRC_DIR = .
BUILD_DIR = ../../build/$(NAME)
LIB_DIR = ../../lib
INC_DIR = ../../include

SRCS := $(shell find $(SRC_DIR) -name '*.c' -print)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

CC = clang
INCLUDE_FLAGS = -I$(SRC_DIR) -I$(INC_DIR) -include ../base.h
WFLAGS = -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter -pedantic
CFLAGS = -g -std=c89 $(WFLAGS) $(INCLUDE_FLAGS) $(DEFINES)

$(TARGET): $(OBJS)
	ar rcs $@ $(OBJS)
	cp $(HEADER) $(INC_DIR)/

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(OBJS)
	rm -rf $(TARGET)
	rm -rf $(INC_DIR)/$(HEADER)
