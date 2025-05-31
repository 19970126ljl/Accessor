CXX = clang++
CXXFLAGS = -std=c++20 -Wall -Wextra -I./include
LDFLAGS =

SRC_DIR = src/test
BUILD_DIR = build
TEST_EXE = $(BUILD_DIR)/test_accessor

SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

.PHONY: all clean test

all: $(TEST_EXE)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TEST_EXE): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

test: $(TEST_EXE)
	$(TEST_EXE)

clean:
	rm -rf $(BUILD_DIR) 