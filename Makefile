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


clean:
	@rm -rf $(BUILD_DIR) $(BINARY)


