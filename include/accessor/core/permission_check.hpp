#pragma once

#include "access_mode.hpp"
#include <type_traits>

namespace accessor {

/// @brief Helper function to check if a given access mode has the required permissions
/// @tparam CurrentMode The current access mode of the accessor
/// @tparam RequiredMode The required access mode for the operation
/// @return true if the current mode has sufficient permissions for the required mode
template<AccessMode CurrentMode, AccessMode RequiredMode>
struct check_permission {
    static constexpr bool value = 
        (RequiredMode == AccessMode::Read && has_read_permission<CurrentMode>::value) ||
        (RequiredMode == AccessMode::Write && has_write_permission<CurrentMode>::value) ||
        (RequiredMode == AccessMode::ReadWrite && 
         has_read_permission<CurrentMode>::value && 
         has_write_permission<CurrentMode>::value) ||
        (RequiredMode == AccessMode::Reduce && is_reduction<CurrentMode>::value);
};

/// @brief Static assertion helper for access mode permissions
/// @tparam CurrentMode The current access mode of the accessor
/// @tparam RequiredMode The required access mode for the operation
template<AccessMode CurrentMode, AccessMode RequiredMode>
struct require_permission {
    static_assert(check_permission<CurrentMode, RequiredMode>::value,
        "Accessor does not have the required permissions for this operation");
};

} // namespace accessor 