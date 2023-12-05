NAME = cassette

# Config section: set appropriate values here

PLATFORM=Apple
FONT_PATH = \"$(HOME)/Library/Fonts\"
DEFAULT_FONT = \"BerkeleyMono-Regular.ttf\"
PREFIX = /opt/homebrew

# End Config

TARGET = ./$(NAME)
SRC_DIR = src
BUILD_DIR = build
INSTALL_DIR = $(HOME)/.local/bin

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
	$(TARGET) -p test/project.txt

.PHONY: install
install: $(TARGET)
	install $(TARGET) $(INSTALL_DIR)
