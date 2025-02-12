NAME = cassette

BIN = bin
BUILD = build
INCLUDE = include
LIB = lib
SRC = src
SHARE = share
INSTALL = $(HOME)/.local

DEBUG ?= 0
PROFILE ?= 0

EXECTARGET = $(BIN)/$(NAME)
LIBTARGET = $(BIN)/lib$(NAME).dylib

MAIN := main
TESTFILE = test/test.ct
SRCS := $(shell find $(SRC) -name '*.c' -not -name '$(MAIN).c' -print)
OBJS := $(SRCS:$(SRC)/%.c=$(BUILD)/%.o)
MAIN_OBJ := $(BUILD)/$(MAIN).o

CC = clang
INCLUDE_FLAGS = -I$(INCLUDE) -include univ/prefix.h
WFLAGS = -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter -pedantic
CFLAGS = -std=c89 $(WFLAGS) $(INCLUDE_FLAGS)
LIBLDFLAGS = -dynamiclib -undefined dynamic_lookup

ifeq ($(DEBUG),1)
CFLAGS += -O2 -g -fsanitize=address -fno-omit-frame-pointer -DDEBUG
	ifeq ($(PROFILE),1)
		CFLAGS += -DPROFILE
	endif
else
CFLAGS += -O2
endif

PLATFORM := $(shell uname -s)
ifeq ($(PLATFORM),Darwin)
LDFLAGS = -framework Cocoa
else ifeq ($(PLATFORM),Linux)
LDFLAGS = -lX11
LIBLDFLAGS = -shared
CFLAGS += -fPIC
LIBTARGET = $(BIN)/lib$(NAME).so
endif

$(EXECTARGET): $(LIBTARGET) $(MAIN_OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(LDFLAGS) $(LIBTARGET) $(MAIN_OBJ) -o $@

$(LIBTARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(LIBLDFLAGS) -o $@ $(OBJS)

$(BUILD)/%.o: $(SRC)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(BUILD)
	rm -rf $(BIN)

.PHONY: test
test: $(EXECTARGET)
	$(EXECTARGET) -L share $(TESTFILE)

.PHONY: leaks
leaks: $(EXECTARGET)
	leaks -atExit -- $(EXECTARGET) -L share $(TESTFILE)

.PHONY: syntax
syntax:
	bison -v support/syntax.y -o support/syntax.tab && rm support/syntax.tab

.PHONY: activity
activity:
	support/activity.sh > support/site/activity.html

.PHONY: sign
sign: $(EXECTARGET)
	codesign -f -s 'Development' --entitlements support/entitlements.xml $(LIBTARGET)
	codesign -f -s 'Development' --entitlements support/entitlements.xml $(EXECTARGET)

.PHONY: install
install: $(EXECTARGET)
	mkdir -p $(INSTALL)/bin $(INSTALL)/lib $(INSTALL)/share/$(NAME)
	cp $(EXECTARGET) $(INSTALL)/bin/$(NAME)
	cp $(LIBTARGET) $(INSTALL)/lib/lib$(NAME).dylib
	cp $(SHARE)/*.ct $(INSTALL)/share/$(NAME)

.PHONY: uninstall
uninstall:
	rm -f $(INSTALL)/bin/$(NAME)
	rm -f $(INSTALL)/lib/lib$(NAME).dylib
	rm -rf $(INSTALL)/share/$(NAME)
