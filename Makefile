# --- Compiler & Flags ---
CXX ?= g++
CXXFLAGS ?= -std=c++20 -Iinclude -Wall -Wextra -Wpedantic
LDLIBS ?=

# --- Executable Targets ---
TARGET ?= udp-rdt
TEST_TARGET ?= udp-rdt_t
DEBUG_TARGET ?= udp-rdt_d
TEST_DEBUG_TARGET ?= udp-rdt_dt

# --- Directories ---
SRC_DIR = src
BUILD_DIR = build

# --- Source Filtering ---
ALL_SRCS = $(shell find $(SRC_DIR) -name "*.cpp")

# Entry points
APP_MAIN = $(SRC_DIR)/main.cpp
TEST_MAIN = $(SRC_DIR)/test_main.cpp

# Filter sources for different builds
NON_TEST_SRCS = $(filter-out %_test.cpp, $(ALL_SRCS))
PROD_SRCS = $(filter-out $(TEST_MAIN), $(NON_TEST_SRCS))
TEST_SRCS = $(filter-out $(APP_MAIN), $(ALL_SRCS))

# --- Object Files ---
PROD_OBJS       = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(PROD_SRCS))
TEST_OBJS       = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%_test.o, $(TEST_SRCS))
DEBUG_OBJS      = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%_debug.o, $(PROD_SRCS))
TEST_DEBUG_OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%_test_debug.o, $(TEST_SRCS))

# Dependency tracking files
ALL_OBJS = $(PROD_OBJS) $(TEST_OBJS) $(DEBUG_OBJS) $(TEST_DEBUG_OBJS)
DEPS     = $(ALL_OBJS:.o=.d)

# --- Build Flags ---
DEP_FLAGS        = -MMD -MP
PROD_FLAGS       = $(DEP_FLAGS) -O3 -DDOCTEST_CONFIG_DISABLE -DDBG_MACRO_DISABLE -DDBG_MACRO_NO_WARNING
DEBUG_FLAGS      = $(DEP_FLAGS) -g -O0 -DDOCTEST_CONFIG_DISABLE -DDBG_MACRO_NO_WARNING
TEST_FLAGS       = $(DEP_FLAGS) -O0 -DDBG_MACRO_NO_WARNING
TEST_DEBUG_FLAGS = $(DEP_FLAGS) -g -O0 -DDBG_MACRO_NO_WARNING

.PHONY: all help clean test test-unit test-integration test-valgrind test-netem debug test-debug

# Default target
all: $(TARGET)

help:
	@echo "Available targets:"
	@echo "  all             Build release binary ($(TARGET))"
	@echo "  test            Run unit tests and Python integration tests"
	@echo "  test-unit       Run unit tests only ($(TEST_TARGET))"
	@echo "  test-integration Run Python integration tests"
	@echo "  test-valgrind   Run memory leak tests under Valgrind"
	@echo "  test-netem      Run Linux kernel netem resilience tests (requires sudo)"
	@echo "  debug           Build debug binary ($(DEBUG_TARGET))"
	@echo "  test-debug      Build test debug binary ($(TEST_DEBUG_TARGET))"
	@echo "  clean           Remove build artifacts and temporary files"

# Primary binary
$(TARGET): $(PROD_OBJS)
	$(CXX) $(CXXFLAGS) $(PROD_FLAGS) $^ -o $@ $(LDLIBS)

# Clean
clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(TEST_TARGET) $(DEBUG_TARGET) $(TEST_DEBUG_TARGET) \
		ipk-rdt ipk-rdt_t ipk-rdt_d ipk-rdt_dt *.bin test_*.bin valgrind_*.log temp_test_*

# Test suite (Unit tests + Python integration tests)
test: $(TEST_TARGET) $(TARGET)
	@echo "--- RUNNING UNIT TESTS (doctest) ---"
	./$(TEST_TARGET)
	@echo ""
	@echo "--- RUNNING INTEGRATION TESTS (python) ---"
	PYTHONPATH=$$PYTHONPATH:tests python3 -m unittest discover -s tests -p "test_*.py"

test-unit: $(TEST_TARGET)
	@echo "--- RUNNING UNIT TESTS (doctest) ---"
	./$(TEST_TARGET)

test-integration: $(TARGET)
	@echo "--- RUNNING INTEGRATION TESTS (python) ---"
	PYTHONPATH=$$PYTHONPATH:tests python3 -m unittest discover -s tests -p "test_*.py"

# Memory leak verification
test-valgrind: $(TARGET)
	@echo "--- RUNNING MEMORY LEAK TESTS (valgrind) ---"
	@bash tests/test_valgrind.sh

# Kernel emulation test (separate target due to sudo requirements)
test-netem: $(TARGET)
	@echo "--- RUNNING OS-LEVEL RESILIENCE TESTS (netem) ---"
	@bash tests/test_netem.sh

$(TEST_TARGET): $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(TEST_FLAGS) $^ -o $@ $(LDLIBS)

# Debug builds
debug: $(DEBUG_TARGET)
	@echo "Debug build ready. Run: gdb ./$(DEBUG_TARGET)"

$(DEBUG_TARGET): $(DEBUG_OBJS)
	$(CXX) $(CXXFLAGS) $(DEBUG_FLAGS) $^ -o $@ $(LDLIBS)

test-debug: $(TEST_DEBUG_TARGET)
	@echo "Test debug build ready. Run: gdb ./$(TEST_DEBUG_TARGET)"

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

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Include compiler-generated header dependencies
-include $(DEPS)
