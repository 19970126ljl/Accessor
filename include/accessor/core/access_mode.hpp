#pragma once

#include <type_traits>

namespace accessor {

/// @brief Access mode enumeration defining the type of access to a data structure
enum class AccessMode {
    Read,       ///< Read-only access
    Write,      ///< Write-only access (typically implies overwrite)
    ReadWrite,  ///< Read-write access
    Reduce      ///< Reduction access (for concurrent-safe accumulation, min/max, etc.)
};

/// @brief Helper type to check if an access mode includes read permission
template<AccessMode Mode>
struct has_read_permission : std::false_type {};

template<>
struct has_read_permission<AccessMode::Read> : std::true_type {};

template<>
struct has_read_permission<AccessMode::ReadWrite> : std::true_type {};

template<>
struct has_read_permission<AccessMode::Reduce> : std::true_type {};

/// @brief Helper type to check if an access mode includes write permission
template<AccessMode Mode>
struct has_write_permission : std::false_type {};

template<>
struct has_write_permission<AccessMode::Write> : std::true_type {};

template<>
struct has_write_permission<AccessMode::ReadWrite> : std::true_type {};

template<>
struct has_write_permission<AccessMode::Reduce> : std::true_type {};

/// @brief Helper type to check if an access mode is a reduction operation
template<AccessMode Mode>
struct is_reduction : std::false_type {};

template<>
struct is_reduction<AccessMode::Reduce> : std::true_type {};

} // namespace accessor 