cmake_minimum_required(VERSION 3.20)
project(Accessor VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
elseif(MSVC)
    add_compile_options(/W4)
endif()

# Find required packages
find_package(OpenMP REQUIRED)

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Add test executable
add_executable(test_accessor
    src/test/test_accessor.cpp
    src/test/test_traits.cpp
    src/test/test_saxpy.cpp
)

# Link against OpenMP
target_link_libraries(test_accessor PRIVATE OpenMP::OpenMP_CXX)

# Enable testing
enable_testing()
add_test(NAME test_accessor COMMAND test_accessor) 