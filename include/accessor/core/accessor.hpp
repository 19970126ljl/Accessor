#pragma once

#include "access_mode.hpp"
#include "permission_check.hpp"
#include "../traits/data_structure_traits.hpp"
#include <type_traits>

namespace accessor {

/// @brief Accessor template class for accessing data structures with specific access modes
/// @tparam DS_Type The type of the data structure to access
/// @tparam Mode The access mode for this accessor
template<typename DS_Type, AccessMode Mode>
class Accessor {
public:
    using TraitsType = DataStructureTraits<DS_Type>;
    using ItemIDType = typename TraitsType::ItemIDType;
    using ValueType = typename TraitsType::ValueType;

    static constexpr AccessMode mode_value = Mode;

    /// @brief Constructor
    /// @param ds Reference to the data structure instance
    explicit Accessor(DS_Type& ds) : data_ref(ds) {}

    /// @brief Get a value by its ID
    /// @param id The ID of the item to access
    /// @return The value at the specified ID
    ValueType get_value_by_id(ItemIDType id) const {
        require_permission<Mode, AccessMode::Read>();
        return TraitsType::get_value_by_id_impl(data_ref, id);
    }

    /// @brief Set a value by its ID
    /// @param id The ID of the item to modify
    /// @param new_value The new value to set
    void set_value_by_id(ItemIDType id, ValueType new_value) {
        require_permission<Mode, AccessMode::Write>();
        TraitsType::set_value_by_id_impl(data_ref, id, new_value);
    }

    /// @brief Get a view of the data structure
    /// @tparam ViewSpecifierTag The type of view to get
    /// @tparam ViewArgs Additional arguments for view creation
    /// @param id_for_view_context The ID to use as context for the view
    /// @param tag The view specifier tag
    /// @param args Additional arguments for view creation
    /// @return The requested view
    template<typename ViewSpecifierTag, typename... ViewArgs>
    auto get_view(ItemIDType id_for_view_context, 
                 ViewSpecifierTag tag,
                 ViewArgs&&... args) const {
        require_permission<Mode, AccessMode::Read>();
        static_assert(TraitsType::template supports_view<ViewSpecifierTag>,
            "This data structure does not support the requested view type");
        return TraitsType::get_view_impl(data_ref, id_for_view_context, tag, 
            std::forward<ViewArgs>(args)...);
    }

    DS_Type& data_ref;
};

} // namespace accessor 