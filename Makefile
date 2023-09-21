SRC_DIR := ./src
BUILD_DIR := ./build
TEST_TARGET := ./test

DIRS := $(shell find $(SRC_DIR) -type d -depth 1 -print)

all: $(DIRS)
$(DIRS):
	$(MAKE) -C $@

.PHONY: all $(DIRS)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

test: $(TEST_TARGET)
	$(TEST_TARGET)
