BUILD_DIR := site
SRC_DIR := .

SRCS := $(shell find $(SRC_DIR) -name '*.md' -print)
OBJS := $(SRCS:$(SRC_DIR)/%.md=$(BUILD_DIR)/%.html)
FLAGS = -s --template=template.html --syntax-definition=syntax.xml

.PHONY: all
all: $(OBJS)

$(BUILD_DIR)/index.html: $(SRC_DIR)/index.md template.html syntax.xml
	@mkdir -p $(dir $@)
	@echo $<
	$(shell sed 's/^# .*//' < $< | pandoc $(FLAGS) --toc --toc-depth=2 --metadata title="Cassette" -V hidetoc > $@)

$(BUILD_DIR)/%.html: $(SRC_DIR)/%.md template.html syntax.xml
	@mkdir -p $(dir $@)
	@echo $<
	$(shell sed 's/^# .*//' < $< | pandoc $(FLAGS) --toc --toc-depth=2 --metadata title="$(shell grep '^# ' $< | head -n 1 | sed 's/^# //')" -V showtitle > $@)

.PHONY: clean
clean:
	rm -f $(OBJS)
