NAME = cassette

TARGET = $(NAME)
SRC = src
BUILD = build

SRCS := $(shell find $(SRC) -name '*.c' -print)
OBJS := $(SRCS:$(SRC)/%.c=$(BUILD)/%.o)

CC = clang
INCLUDE_FLAGS = -I$(SRC) -include base.h
WFLAGS = -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter -pedantic
CFLAGS = -g -std=c89 $(WFLAGS) $(INCLUDE_FLAGS) -fsanitize=address
LDFLAGS =

$(TARGET): $(OBJS)
	@echo $<
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@

$(BUILD)/%.o: $(SRC)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(BUILD)
	rm -rf $(TARGET)

.PHONY: test
test: $(TARGET)
	./$(TARGET) test/test.ct && wasmer validate out.wasm

.PHONY: entitlements
entitlements: $(TARGET)
	codesign -f -s 'Apple Development' --entitlements support/entitlements.xml $(TARGET)

