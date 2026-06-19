SYSTEMC_HOME ?= /usr/local/systemc
BUILD_DIR     = build
BINARY        = rgb2gray

.PHONY: all build run image view clean help

all: build

build:
	@mkdir -p $(BUILD_DIR)
	@cmake -S . -B $(BUILD_DIR)             \
	    -DSYSTEMC_HOME=$(SYSTEMC_HOME)      \
	    -DCMAKE_BUILD_TYPE=Release          \
	    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON  \
	    > $(BUILD_DIR)/cmake_config.log 2>&1 \
		|| { cat $(BUILD_DIR)/cmake_config.log; exit 1; }
	@cmake --build $(BUILD_DIR) -- -j$$(nproc)

run: build
	@$(BUILD_DIR)/$(BINARY) pictures/raw/grumpy-online.raw $(BUILD_DIR)/output.raw

native-build: build

native-run: run

view:
	@python3 scripts/raw_to_image.py $(BUILD_DIR)/output.raw $(BUILD_DIR)/output.png --mode gray

image:
	@python3 scripts/image_to_raw.py pictures/jpg/grumpy-cat.jpg $(BUILD_DIR)/grumpy-cat.raw --resize


clean:
	@rm -rf $(BUILD_DIR) $(BINARY)

