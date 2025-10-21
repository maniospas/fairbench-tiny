# Compiler and flags
CXX := gcc
CXXFLAGS := -Wall -Wextra -Wpedantic -Wconversion
TARGET := fbt
BUILD_DIR := ./build

# Source files
SRC := $(shell find src -type f -name '*.c')

# Default target
all: release

# Release build
release: $(SRC)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -O3 $(SRC) -o $(BUILD_DIR)/$(TARGET) -s -flto -Wl,--gc-sections -fdata-sections -ffunction-sections

# Debug build (with sanitizers)
debug: $(SRC)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -g -O0 -rdynamic -DDEBUG \
		-fsanitize=address,undefined -D_GLIBCXX_DEBUG \
		$(SRC) -o $(BUILD_DIR)/$(TARGET)

# Profiling build (with frame pointers)
profile: $(SRC)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -g -O3 -fno-omit-frame-pointer $(SRC) -o $(BUILD_DIR)/$(TARGET)
	# Uncomment below for gprof:
	# $(CXX) $(CXXFLAGS) -pg $(SRC) -o $(BUILD_DIR)/$(TARGET)

# Clean up
clean:
	rm -rf $(BUILD_DIR)

# Convenience targets
run: release
	./$(BUILD_DIR)/$(TARGET)

rebuild: clean all

.PHONY: all release debug profile clean run rebuild
