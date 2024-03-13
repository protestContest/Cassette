TARGET = cassette

BIN = bin
BUILD = build
INCLUDE = include
LIB = lib
SRC = src

SRCS := $(shell find $(SRC) -name '*.c' -print)
OBJS := $(SRCS:$(SRC)/%.c=$(BUILD)/%.o)

CC = clang
INCLUDE_FLAGS = -I$(INCLUDE) -include base.h
WFLAGS = -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter -pedantic
CFLAGS = -g -std=c89 $(WFLAGS) $(INCLUDE_FLAGS) -fsanitize=address
LDFLAGS = -L$(LIB) -luniv

$(BIN)/$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	@echo $<
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@

$(BUILD)/%.o: $(SRC)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(BUILD)
	rm -rf $(BIN)

.PHONY: test
test: $(BIN)/$(TARGET)
	$(BIN)/$(TARGET) test/test.ct

.PHONY: entitlements
entitlements: $(BIN)/$(TARGET)
	codesign -f -s 'Apple Development' --entitlements support/entitlements.xml $(BIN)/$(TARGET)

