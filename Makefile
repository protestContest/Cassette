NAME = cassette

BIN = bin
BUILD = build
INCLUDE = include
LIB = lib
SRC = src
INSTALL = $(HOME)/.local

EXECTARGET = $(BIN)/$(NAME)
LIBTARGET = $(BIN)/lib$(NAME).dylib

MAIN := main
TESTFILE = test/test.ct
SRCS := $(shell find $(SRC) -name '*.c' -not -name '$(MAIN).c' -print)
OBJS := $(SRCS:$(SRC)/%.c=$(BUILD)/%.o)
MAIN_OBJ := $(BUILD)/$(MAIN).o

CC = clang
DEFS = -DDEBUG=0
INCLUDE_FLAGS = -I$(INCLUDE) -include univ/base.h
WFLAGS = -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter -pedantic
CFLAGS = -g -O0 -std=c89 $(WFLAGS) $(INCLUDE_FLAGS) $(DEFS)
LDFLAGS = -luniv -lsdl2
LIBLDFLAGS = -dynamiclib -undefined dynamic_lookup -luniv

$(EXECTARGET): $(LIBTARGET) $(MAIN_OBJ)
	@mkdir -p $(dir $@)
	@echo $@
	$(CC) $(CFLAGS) $(LDFLAGS) $(LIBTARGET) $(MAIN_OBJ) -o $@

$(LIBTARGET): $(OBJS)
	@mkdir -p $(dir $@)
	@echo $<
	$(CC) $(LIBLDFLAGS) -o $@ $(OBJS)

$(BUILD)/%.o: $(SRC)/%.c
	@mkdir -p $(dir $@)
	@echo $@
	@$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(BUILD)
	rm -rf $(BIN)

.PHONY: test
test: $(EXECTARGET)
	$(EXECTARGET) $(TESTFILE)

.PHONY: leaks
leaks: $(EXECTARGET)
	leaks -atExit -- $(EXECTARGET) $(TESTFILE)

.PHONY: entitlements
entitlements: $(EXECTARGET)
	codesign -f -s 'Apple Development' --entitlements support/entitlements.xml $(LIBTARGET)
	codesign -f -s 'Apple Development' --entitlements support/entitlements.xml $(EXECTARGET)

