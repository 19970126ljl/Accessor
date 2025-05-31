CXX = nvc++
CXXFLAGS = -std=c++20 -Wall -Wextra -I./include -fopenmp
LDFLAGS = -fopenmp

SRC_DIR = src/test
BUILD_DIR = build
TEST_ACCESSOR_EXE = $(BUILD_DIR)/test_accessor_run
TEST_SPMV_EXE = $(BUILD_DIR)/test_spmv_run

# Specify sources for each executable
ACCESSOR_SRCS = $(SRC_DIR)/test_accessor.cpp
SPMV_SRCS = $(SRC_DIR)/test_spmv.cpp

# Generate object file names from sources
ACCESSOR_OBJS = $(ACCESSOR_SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
SPMV_OBJS = $(SPMV_SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

.PHONY: all clean test

all: $(TEST_ACCESSOR_EXE) $(TEST_SPMV_EXE)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Generic rule for object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Link rule for test_accessor_run
$(TEST_ACCESSOR_EXE): $(ACCESSOR_OBJS)
	$(CXX) $(ACCESSOR_OBJS) -o $@ $(LDFLAGS)

# Link rule for test_spmv_run
$(TEST_SPMV_EXE): $(SPMV_OBJS)
	$(CXX) $(SPMV_OBJS) -o $@ $(LDFLAGS)

test: $(TEST_ACCESSOR_EXE) $(TEST_SPMV_EXE)
	$(TEST_ACCESSOR_EXE)
	$(TEST_SPMV_EXE)

clean:
	rm -rf $(BUILD_DIR) 