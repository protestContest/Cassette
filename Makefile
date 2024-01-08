NAME = cassette

### Config section: set appropriate values here

PLATFORM=Apple
FONT_PATH = \"/Library/Fonts\"
DEFAULT_FONT = \"SF-Pro.ttf\"
PREFIX = /opt/homebrew
INSTALL_PREFIX = $(HOME)/.local

### End Config

TARGET = $(NAME)
SRC = src
BUILD = build
SHARE = share

SRCS := $(shell find $(SRC) -name '*.c' -print)
OBJS := $(SRCS:$(SRC)/%.c=$(BUILD)/%.o)

DEFINES = -DPLATFORM=$(PLATFORM) -DFONT_PATH=$(FONT_PATH) -DDEFAULT_FONT=$(DEFAULT_FONT)

CC = clang
INCLUDE_FLAGS = -I$(PREFIX)/include -I$(SRC) -include base.h
WFLAGS = -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter -pedantic
CFLAGS = -g -std=c89 $(WFLAGS) $(INCLUDE_FLAGS) $(DEFINES) -fsanitize=address
LDFLAGS = -L$(PREFIX)/lib -lSDL2 -lSDL2_ttf

ifeq ($(PLATFORM),Apple)
	LDFLAGS += -framework IOKit -framework CoreFoundation
endif

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

# hack to quickly generate symbols
.PHONY: symbols
symbols: $(OBJS)
	@rm -f $(BUILD)/main.o
	@rm -f $(BUILD)/gen_symbols.o
	$(CC) $(CFLAGS) -DGEN_SYMBOLS -c $(SRC)/main.c -o $(BUILD)/main.o
	$(CC) $(CFLAGS) -DGEN_SYMBOLS -c $(SRC)/gen_symbols.c -o $(BUILD)/gen_symbols.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o "gen_symbols"
	./gen_symbols
	@rm -f ./gen_symbols
	@rm -f $(BUILD)/main.o
	@rm -f $(BUILD)/gen_symbols.o

.PHONY: test
test: $(TARGET)
	CASSETTE_STDLIB=$(SHARE) ./$(TARGET) test/test.ct

.PHONY: install
install: $(TARGET)
	install -d $(INSTALL_PREFIX)/bin $(INSTALL_PREFIX)/share/$(TARGET)
	install $(TARGET) $(INSTALL_PREFIX)/bin
	install $(SHARE)/* $(INSTALL_PREFIX)/share/$(TARGET)

.PHONY: uninstall
uninstall:
	rm -f $(INSTALL_PREFIX)/bin/$(TARGET)
	rm -rf $(INSTALL_PREFIX)/share/$(TARGET)

.PHONY: docs
docs:
	$(MAKE) -C support/docs

.PHONY: entitlements
entitlements: $(TARGET)
	codesign -f -s 'Apple Development' --entitlements support/entitlements.xml $(TARGET)

