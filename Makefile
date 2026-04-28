# --- Configuration ---
CXX = g++
CXXFLAGS = -std=c++20 -Iinclude -Wall -Wextra -Wpedantic
LDLIBS = -lpcap # TODO: Check necessity
LOGIN = xambruj00

# Executable outputs
TARGET = ipk-rdt
TEST_TARGET = ipk-rdt_t
DEBUG_TARGET = ipk-rdt_d
TEST_DEBUG_TARGET = ipk-rdt_dt

# Directories
SRC_DIR = src
BUILD_DIR = build

# --- Source Filtering ---
ALL_SRCS = $(shell find $(SRC_DIR) -name "*.cpp")

# Define entry points
APP_MAIN = $(SRC_DIR)/main.cpp
TEST_MAIN = $(SRC_DIR)/test_main.cpp

# Filter sources for different builds
# Production/Debug drops test_main.cpp. Tests drop main.cpp.
PROD_SRCS = $(filter-out $(TEST_MAIN), $(ALL_SRCS))
TEST_SRCS = $(filter-out $(APP_MAIN), $(ALL_SRCS))

# --- Object Files ---
# To handle subdirectories, we preserve the directory structure in the build/ folder.
PROD_OBJS       = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(PROD_SRCS))
TEST_OBJS       = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%_test.o, $(TEST_SRCS))
DEBUG_OBJS      = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%_debug.o, $(PROD_SRCS))
TEST_DEBUG_OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%_test_debug.o, $(TEST_SRCS))

# --- Build Flags ---
# DOCTEST_CONFIG_DISABLE removes test cases from production and standard debug builds
# DBG_MACRO_DISABLE removes debug prints from production
PROD_FLAGS       = -O3 -DDOCTEST_CONFIG_DISABLE -DDBG_MACRO_DISABLE -DDBG_MACRO_NO_WARNING
DEBUG_FLAGS      = -g -O0 -DDOCTEST_CONFIG_DISABLE
TEST_FLAGS       = -O0
TEST_DEBUG_FLAGS = -g -O0

.PHONY: all clean zip test debug test-debug NixDevShellName

# `make` - Builds the standard executable
all: $(TARGET)

$(TARGET): $(PROD_OBJS)
	$(CXX) $(CXXFLAGS) $(PROD_FLAGS) $^ -o $@ $(LDLIBS)

# `make clean` - Cleans everything
clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(TEST_TARGET) $(DEBUG_TARGET) $(TEST_DEBUG_TARGET) $(LOGIN).zip

# `make zip` - Packages everything into $(LOGIN).zip
zip: clean
	zip -r $(LOGIN).zip $(SRC_DIR) include Makefile CHANGELOG.md LICENSE README.md -x "*.git*"

# `make test` - Builds and runs the testing framework
test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(TEST_FLAGS) $^ -o $@ $(LDLIBS)

# `make debug` - Prepares debugging environment
debug: $(DEBUG_TARGET)
	@echo "Debug build ready. Run: gdb ./$(DEBUG_TARGET)"

$(DEBUG_TARGET): $(DEBUG_OBJS)
	$(CXX) $(CXXFLAGS) $(DEBUG_FLAGS) $^ -o $@ $(LDLIBS)

# `make test-debug` - Prepares debug environment for tests
test-debug: $(TEST_DEBUG_TARGET)
	@echo "Test debug build ready. Run: gdb ./$(TEST_DEBUG_TARGET)"

# `make NixDevShellName` - Outputs 'c' to stdout
NixDevShellName:
	@echo "c"

$(TEST_DEBUG_TARGET): $(TEST_DEBUG_OBJS)
	$(CXX) $(CXXFLAGS) $(TEST_DEBUG_FLAGS) $^ -o $@ $(LDLIBS)

# --- Compilation Rules ---
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(PROD_FLAGS) -c $< -o $@

$(BUILD_DIR)/%_test.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(TEST_FLAGS) -c $< -o $@

$(BUILD_DIR)/%_debug.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEBUG_FLAGS) -c $< -o $@

$(BUILD_DIR)/%_test_debug.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(TEST_DEBUG_FLAGS) -c $< -o $@

# Dynamically create the build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
