#pragma once

#include "data_structure_traits.hpp"
#include <vector>
#include <cstddef>

namespace accessor {

/// @brief Simple one-dimensional dense array data structure
/// @tparam T The type of elements stored in the array
template<typename T>
struct DenseArray1D {
    std::vector<T> data;
    size_t count;

    // Constructors
    DenseArray1D() : count(0) {}
    explicit DenseArray1D(size_t size) : data(size), count(size) {}
    DenseArray1D(std::vector<T>&& vec) : data(std::move(vec)), count(data.size()) {}
    DenseArray1D(const std::vector<T>& vec) : data(vec), count(vec.size()) {}

    // Access operators
    T& operator[](size_t index) { return data[index]; }
    const T& operator[](size_t index) const { return data[index]; }

    // Size information
    size_t size() const { return count; }
    bool empty() const { return count == 0; }
};

/// @brief DataStructureTraits specialization for DenseArray1D
/// @tparam T The type of elements stored in the array
template<typename T>
struct DataStructureTraits<DenseArray1D<T>> {
    using ItemIDType = size_t;
    using ValueType = T;

    // Required static member functions for iteration support
    static size_t get_size_for_iteration(const DenseArray1D<T>& arr, IterateOverAll_Tag) {
        return arr.count;
    }

    static ItemIDType get_item_id_from_global_index(
        const DenseArray1D<T>& arr, 
        size_t global_idx, 
        IterateOverAll_Tag) {
        return global_idx;
    }

    // Direct value access implementations
    static ValueType get_value_by_id_impl(const DenseArray1D<T>& arr, ItemIDType id) {
        return arr.data[id];
    }

    static void set_value_by_id_impl(DenseArray1D<T>& arr, ItemIDType id, ValueType val) {
        arr.data[id] = val;
    }
};

} // namespace accessor 