#pragma once

#include <type_traits>
#include <cstddef>

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
};

} // namespace accessor 