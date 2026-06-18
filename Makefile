SYSTEMC_HOME ?= /usr/local/systemc
BUILD_DIR     = build
BINARY        = rgb2gray
SCRIPT_CXX    ?= g++
SCRIPT_CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
SCRIPT_CPPFLAGS ?= -Ithird_party/stb

.PHONY: all build scripts image_to_raw_cpp raw_to_image_cpp run image view clean help

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

scripts: image_to_raw_cpp raw_to_image_cpp

image_to_raw_cpp: $(BUILD_DIR)/image_to_raw

$(BUILD_DIR)/image_to_raw: scripts/image_to_raw.cpp third_party/stb/stb_image.h third_party/stb/stb_image_resize2.h
	@mkdir -p $(BUILD_DIR)
	$(SCRIPT_CXX) $(SCRIPT_CPPFLAGS) $(SCRIPT_CXXFLAGS) scripts/image_to_raw.cpp -o $@

raw_to_image_cpp: $(BUILD_DIR)/raw_to_image

$(BUILD_DIR)/raw_to_image: scripts/raw_to_image.cpp third_party/stb/stb_image_write.h
	@mkdir -p $(BUILD_DIR)
	$(SCRIPT_CXX) $(SCRIPT_CPPFLAGS) $(SCRIPT_CXXFLAGS) scripts/raw_to_image.cpp -o $@

clean:
	@rm -rf $(BUILD_DIR) $(BINARY)
