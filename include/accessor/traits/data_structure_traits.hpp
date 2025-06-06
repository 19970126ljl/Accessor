#pragma once

#include <type_traits>
#include <cstddef>
#include <vector>

namespace accessor {

/// @brief Tag type for iteration over all elements
struct IterateOverAll_Tag {};

/// @brief Base template for data structure traits
/// @tparam DS_Type The data structure type to specialize for
template<typename DS_Type>
struct DataStructureTraits {
    // Required type aliases - must be defined by specializations
    using ItemIDType = void;  // Type used to identify individual items
    using ValueType = void;   // Type of the values stored in the structure

    // Required static constexpr members
    template<typename ViewSpecifierTag>
    static constexpr bool supports_view = false;

    // Required static member functions for iteration support
    [[noreturn]] static size_t get_size_for_iteration(const DS_Type&, IterateOverAll_Tag) {
        static_assert(sizeof(DS_Type) == 0, 
            "DataStructureTraits must be specialized for this type");
        __builtin_unreachable();
    }

    [[noreturn]] static ItemIDType get_item_id_from_global_index(
        const DS_Type&, 
        size_t, 
        IterateOverAll_Tag) {
        static_assert(sizeof(DS_Type) == 0, 
            "DataStructureTraits must be specialized for this type");
        __builtin_unreachable();
    }

    // Optional static member functions for direct value access
    template<typename T = ValueType, typename = std::enable_if_t<!std::is_void_v<T>>>
    [[noreturn]] static T get_value_by_id_impl(const DS_Type&, typename std::conditional_t<std::is_void_v<ItemIDType>, int, ItemIDType>) {
        static_assert(sizeof(DS_Type) == 0, 
            "DataStructureTraits must be specialized for this type");
        __builtin_unreachable();
    }

    template<typename T = ValueType, typename = std::enable_if_t<!std::is_void_v<T>>>
    [[noreturn]] static void set_value_by_id_impl(
        DS_Type&, 
        typename std::conditional_t<std::is_void_v<ItemIDType>, int, ItemIDType>,
        T) {
        static_assert(sizeof(DS_Type) == 0, 
            "DataStructureTraits must be specialized for this type");
        __builtin_unreachable();
    }

    // Optional static member functions for view access
    template<typename ViewSpecifierTag, typename... ViewArgs>
    [[noreturn]] static auto get_view_impl(
        const DS_Type&, 
        typename std::conditional_t<std::is_void_v<ItemIDType>, int, ItemIDType>,
        ViewSpecifierTag,
        ViewArgs&&...) {
        static_assert(sizeof(DS_Type) == 0, 
            "DataStructureTraits must be specialized for this type");
        __builtin_unreachable();
    }

    // New: Static member function for deep copying data structures
    // Default implementation uses copy assignment, which might be shallow for some types.
    // Specializations should provide deep copy if needed.
    static void copy_data_structure_impl(DS_Type& dest, const DS_Type& src) {
        dest = src; // Default: uses copy assignment operator
    }
};

// std::vector<T> 特化
template <typename T>
struct DataStructureTraits<std::vector<T>> {
    using ItemIDType = size_t;
    using ValueType = T;
    static T get_value_by_id_impl(const std::vector<T>& vec, size_t id) {
        return vec[id];
    }
    static void set_value_by_id_impl(std::vector<T>& vec, size_t id, T val) {
        vec[id] = val;
    }

    // Implementation of copy_data_structure_impl for std::vector (uses std::vector's deep copy assignment)
    static void copy_data_structure_impl(std::vector<T>& dest, const std::vector<T>& src) {
        dest = src; // std::vector's assignment operator performs a deep copy
    }
};

} // namespace accessor 