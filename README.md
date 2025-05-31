# Accessor Framework

A modern C++ framework for parallel iteration over diverse data structures, featuring a flexible Accessor abstraction.

## Project Overview

This framework aims to provide a unified, type-safe, and efficient way to perform parallel iterations over various data structures (arrays, graphs, trees, etc.) while maintaining clear semantics for data access patterns and dependencies.

### Key Features (Planned)

- Generic Accessor abstraction for diverse data structures
- Type-safe access mode declarations (Read, Write, ReadWrite, Reduce)
- Flexible iteration space definitions
- Implicit dependency management
- Support for both CPU and GPU execution

## Building the Project

### Prerequisites

- CMake 3.20 or higher
- C++20 compatible compiler
- OpenMP

### Build Instructions

```bash
mkdir build && cd build
cmake ..
make
```

### Running Tests

```bash
ctest
```

## Development Status

Currently in early prototype phase. The development is proceeding in the following stages:

1. **Foundation** (Current)
   - Core type definitions
   - AccessMode and permission checking
   - Basic Accessor template
   - DataStructureTraits infrastructure

2. **Iteration Space**
   - IterationSpace concept
   - Range1D implementation
   - Device POD types

3. **Parallel Execution**
   - custom_parallel_for implementation
   - Basic synchronization mechanisms

4. **Testing & Validation**
   - Unit tests
   - SAXPY integration test
   - Performance benchmarks

## License

[To be determined] 