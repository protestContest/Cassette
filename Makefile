NAME = cassette

TARGET = ./$(NAME)

SRC_DIR = src
BUILD_DIR = build
INSTALL_DIR = $(HOME)/.local/bin

SRCS := $(shell find $(SRC_DIR) -name '*.c' -print)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Uncomment this to enable debugging support
DEFINES += -DDEBUG=1

CC = clang
INCLUDE_FLAGS = -I$(SRC_DIR) -include base.h
WFLAGS = -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter -pedantic

# Uncomment these lines to enable Canvas support. Make sure SDL2 is installed
# and that the paths below point to the correct locations. Also make sure the
# font path is correct and the default font exists within that folder.
FONT_PATH = \"$(HOME)/Library/Fonts\"
DEFAULT_FONT = \"BerkeleyMono-Regular.ttf\"
DEFINES += -DCANVAS=1 -DFONT_PATH=$(FONT_PATH) -DDEFAULT_FONT=$(DEFAULT_FONT)
INCLUDE_FLAGS += -I/opt/homebrew/include/SDL2
LDFLAGS += -L/opt/homebrew/lib -lSDL2 -lSDL2_ttf

CFLAGS = -g -std=c89 $(WFLAGS) $(INCLUDE_FLAGS) $(DEFINES)

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
