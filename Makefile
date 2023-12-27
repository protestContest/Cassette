NAME = cassette

### Config section: set appropriate values here

PLATFORM=Apple
FONT_PATH = \"/Library/Fonts\"
DEFAULT_FONT = \"SF-Pro.ttf\"
PREFIX = /opt/homebrew
INSTALL_PREFIX = $(HOME)/.local

### End Config

TARGET = $(NAME)
SRC_DIR = src
BUILD_DIR = build
SHARE_DIR = share

SRCS := $(shell find $(SRC_DIR) -name '*.c' -print)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

DEFINES = -DPLATFORM=$(PLATFORM) -DFONT_PATH=$(FONT_PATH) -DDEFAULT_FONT=$(DEFAULT_FONT)

CC = clang
INCLUDE_FLAGS = -I$(PREFIX)/include -I$(SRC_DIR) -include base.h
WFLAGS = -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter -pedantic
CFLAGS = -g -std=c89 $(WFLAGS) $(INCLUDE_FLAGS) $(DEFINES) -fsanitize=address
LDFLAGS = -L$(PREFIX)/lib -lSDL2 -lSDL2_ttf

ifeq ($(PLATFORM),Apple)
	LDFLAGS += -framework IOKit -framework CoreFoundation
endif

$(TARGET): $(OBJS)
	@echo $<
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(TARGET)

.PHONY: test
test: $(TARGET)
	CASSETTE_STDLIB=$(SHARE_DIR) $(TARGET) test/test.ct

.PHONY: install
install: $(TARGET)
	install -d $(INSTALL_PREFIX)/bin $(INSTALL_PREFIX)/share/$(TARGET)
	install $(TARGET) $(INSTALL_PREFIX)/bin
	install $(SHARE_DIR)/* $(INSTALL_PREFIX)/share/$(TARGET)

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

