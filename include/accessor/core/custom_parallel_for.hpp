#pragma once

#include <cstddef>
#include <omp.h>
#include <type_traits>
#include <tuple> // Required for std::tuple and std::apply
#include <utility> // Required for std::forward_as_tuple
#include <vector> // Required for std::vector
#include <algorithm> // Required for std::find_if, std::any_of etc.
#include <iostream> // Required for std::cout
#include <any> // Required for std::any
#include <accessor/traits/dense_array_traits.hpp>

namespace accessor {

// Utility: Determine Accessor type
// Check if the type is an Accessor<T, Mode>
template <typename T>
struct is_accessor : std::false_type {};

template <template <typename, auto> class Acc, typename DS, auto Mode>
struct is_accessor<Acc<DS, Mode>> : std::true_type {};

// Utility: Get data pointer from Accessor
// This is just a demonstration, assuming Accessor has a data_ref member
// In actual projects, use friend or expose interfaces
template <typename Acc>
auto* get_data_ptr(Acc& acc) {
    if constexpr (requires { &acc.data_ref; }) {
        return &acc.data_ref;
    } else {
        return nullptr;
    }
}

// Utility: Get access mode from Accessor
// Via decltype(acc)::Mode
template <typename Acc>
constexpr auto get_access_mode(const Acc&) {
    return std::remove_reference_t<Acc>::mode_value;
}

// Structure to hold buffer requirement information
struct BufferRequirement {
    bool needs_buffering = false;
    std::vector<size_t> write_accessor_indices; // Indices of accessors that should write to a buffer
    // We might need more info, e.g., which read accessors conflict with them.
    // For now, let's assume if a write accessor is part of any conflict, it needs buffering.
};

// Helper to check for read/write conflicts between two modes
constexpr bool do_modes_conflict(AccessMode mode1, AccessMode mode2) {
    bool mode1_reads = (mode1 == AccessMode::Read || mode1 == AccessMode::ReadWrite);
    bool mode1_writes = (mode1 == AccessMode::Write || mode1 == AccessMode::ReadWrite);
    bool mode2_reads = (mode2 == AccessMode::Read || mode2 == AccessMode::ReadWrite);
    bool mode2_writes = (mode2 == AccessMode::Write || mode2 == AccessMode::ReadWrite);

    // Conflict if one reads and the other writes, or both write (to the same data)
    return (mode1_reads && mode2_writes) || (mode1_writes && mode2_reads) || (mode1_writes && mode2_writes);
}

// Forward declaration for the recursive helper
template<size_t I, size_t J, typename AccTuple>
void check_pair_for_conflict_recursive(const AccTuple& acc_tuple, BufferRequirement& req);

// Main recursive helper to iterate through pairs
template<size_t I, typename AccTuple>
void find_conflicts_for_accessor_i(const AccTuple& acc_tuple, BufferRequirement& req) {
    if constexpr (I < std::tuple_size_v<AccTuple> - 1) {
        check_pair_for_conflict_recursive<I, I + 1>(acc_tuple, req);
        find_conflicts_for_accessor_i<I + 1>(acc_tuple, req);
    }
}

// Recursive helper to check a specific pair (I, J)
template<size_t I, size_t J, typename AccTuple>
void check_pair_for_conflict_recursive(const AccTuple& acc_tuple, BufferRequirement& req) {
    if constexpr (J < std::tuple_size_v<AccTuple>) {
        auto& acc_i = std::get<I>(acc_tuple);
        auto& acc_j = std::get<J>(acc_tuple);

        // Only compare data_ref pointers if the underlying data types are the same
        if constexpr (std::is_same_v<decltype(acc_i.data_ref), decltype(acc_j.data_ref)>) {
            if (get_data_ptr(acc_i) == get_data_ptr(acc_j) && get_data_ptr(acc_i) != nullptr) {
                constexpr AccessMode mode_i = std::remove_reference_t<decltype(acc_i)>::mode_value;
                constexpr AccessMode mode_j = std::remove_reference_t<decltype(acc_j)>::mode_value;

                if (do_modes_conflict(mode_i, mode_j)) {
                    req.needs_buffering = true;
                    if (mode_i == AccessMode::Write || mode_i == AccessMode::ReadWrite) {
                        if (std::find(req.write_accessor_indices.begin(), req.write_accessor_indices.end(), I) == req.write_accessor_indices.end()) {
                            req.write_accessor_indices.push_back(I);
                        }
                    }
                    if (mode_j == AccessMode::Write || mode_j == AccessMode::ReadWrite) {
                         if (std::find(req.write_accessor_indices.begin(), req.write_accessor_indices.end(), J) == req.write_accessor_indices.end()) {
                            req.write_accessor_indices.push_back(J);
                        }
                    }
                }
            }
        } // else: if data_ref types are different, they cannot conflict on the same data object.
        
        check_pair_for_conflict_recursive<I, J + 1>(acc_tuple, req);
    }
}

// Function to detect buffer requirements (uses recursive helper)
template<typename... AccessorTypes>
BufferRequirement detect_buffer_requirements(const std::tuple<AccessorTypes&...>& acc_tuple) {
    BufferRequirement req;
    if constexpr (sizeof...(AccessorTypes) >= 2) {
        find_conflicts_for_accessor_i<0>(acc_tuple, req);
    }
    
    std::sort(req.write_accessor_indices.begin(), req.write_accessor_indices.end());
    req.write_accessor_indices.erase(std::unique(req.write_accessor_indices.begin(), req.write_accessor_indices.end()), req.write_accessor_indices.end());
    return req;
}

// Helper function to build the tuple of accessors for the kernel
// It creates new accessor objects: some point to temporary buffers, others to original data.
template<typename OriginalTuple, typename BufferRequirementType, typename TempDataStorageType, std::size_t... Is>
auto create_kernel_arg_accessors_tuple_impl(
    OriginalTuple& original_accessor_tuple, // tuple of original AccessorTypes&...
    BufferRequirementType& buffer_req,      // constains write_accessor_indices
    TempDataStorageType& temp_data_storage, // std::vector<std::any> storing actual buffer data
    std::index_sequence<Is...> /*seq*/
) {
    // This lambda is called for each original accessor to decide what kind of new accessor to create.
    auto construct_kernel_accessor = 
        [&original_accessor_tuple, &buffer_req, &temp_data_storage] // Capture necessary variables
        (auto accessor_compile_time_idx_wrapper) { // Takes a wrapper like std::integral_constant

        // Extract the compile-time index from the wrapper
        constexpr size_t accessor_compile_time_idx = decltype(accessor_compile_time_idx_wrapper)::value;

        // Get a reference to the original accessor at this compile-time index
        auto& original_acc_ref = std::get<accessor_compile_time_idx>(original_accessor_tuple);
        using OriginalAccType = std::remove_reference_t<decltype(original_acc_ref)>;
        using DS_Type = std::remove_reference_t<decltype(original_acc_ref.data_ref)>;
        constexpr AccessMode mode = OriginalAccType::mode_value;

        // Check if this specific accessor (by its original_accessor_tuple index) needs buffering.
        bool needs_buffering_for_this_one = false;
        size_t buffer_storage_vector_idx = 0; 
        size_t count_write_accessors_processed = 0;
        for(size_t write_idx_from_req : buffer_req.write_accessor_indices) {
            if (accessor_compile_time_idx == write_idx_from_req) {
                needs_buffering_for_this_one = true;
                buffer_storage_vector_idx = count_write_accessors_processed;
                break;
            }
            count_write_accessors_processed++;
        }

        if (needs_buffering_for_this_one) {
            // This accessor needs buffering.
            // temp_data_storage should already be populated with its buffer data.
            // The accessor created here will refer to data inside an std::any object.
            // This requires std::any_cast<DS_Type&> to get the reference.
            // The lifetime of temp_data_storage is managed by the caller (custom_parallel_for).
            return Accessor<DS_Type, mode>(std::any_cast<DS_Type&>(temp_data_storage[buffer_storage_vector_idx]));
        } else {
            // This accessor does not need buffering.
            // Create a new accessor that refers to the original data.
            return Accessor<DS_Type, mode>(original_acc_ref.data_ref);
        }
    };

    // Create a tuple by calling construct_kernel_accessor for each index Is...
    // Each call to construct_kernel_accessor(std::integral_constant<size_t, Is>{}) will return an Accessor object.
    return std::make_tuple(construct_kernel_accessor(std::integral_constant<size_t, Is>{})...);
}

// New variadic template version
template<typename IterSpaceType, typename KernelFunc, typename... AccessorTypes>
void custom_parallel_for(IterSpaceType iter_space, KernelFunc&& kernel, AccessorTypes&... accessors) {
    auto original_accessor_tuple = std::forward_as_tuple(accessors...);
    BufferRequirement buffer_req = detect_buffer_requirements(original_accessor_tuple);

    std::vector<std::any> temp_data_buffers_storage; // Stores the actual data for buffers

    if (buffer_req.needs_buffering) {
        // 1. Populate temp_data_buffers_storage with copies of data for write accessors
        auto populate_buffers_if_needed = [&](auto& acc_ref, size_t current_idx) {
            for (size_t write_idx : buffer_req.write_accessor_indices) {
                if (current_idx == write_idx) {
                    using DS_Type = std::remove_reference_t<decltype(acc_ref.data_ref)>;
                    temp_data_buffers_storage.emplace_back(DS_Type(acc_ref.data_ref)); // Copy and store
                    break; 
                }
            }
            return 0; // Dummy return for fold expression
        };
        
        // Use a fold expression to populate buffers for all accessors that need it
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            (populate_buffers_if_needed(std::get<Is>(original_accessor_tuple), Is), ...);
        }(std::index_sequence_for<AccessorTypes...>{});

        // 2. Create the tuple of accessors for the kernel (some pointing to buffers)
        auto kernel_args_tuple = create_kernel_arg_accessors_tuple_impl(
            original_accessor_tuple,
            buffer_req,
            temp_data_buffers_storage,
            std::index_sequence_for<AccessorTypes...>{}
        );
        
        // 3. Execute kernel with thread-local copies of the accessor tuple
        #pragma omp parallel
        {
            // Each thread gets its own copy of the accessor tuple
            auto thread_local_kernel_args = kernel_args_tuple;
            
            #pragma omp for
            for (size_t i = 0; i < iter_space.size(); ++i) {
                auto item_id = iter_space[i];
                std::apply(
                    [&](auto&... unpacked_kernel_accessors) {
                        kernel(item_id, unpacked_kernel_accessors...);
                    },
                    thread_local_kernel_args
                );
            }
        }

        // 4. Write back data from temp_data_buffers_storage to original accessors
        size_t buffer_idx_in_storage = 0;
        auto write_back_if_needed = [&](auto& original_acc_ref, size_t current_idx) {
            for (size_t write_idx_from_req : buffer_req.write_accessor_indices) {
                if (current_idx == write_idx_from_req) {
                    // Get reference to the buffered data
                    using DS_Type = std::remove_reference_t<decltype(original_acc_ref.data_ref)>;
                    auto& buffer_data = std::any_cast<DS_Type&>(temp_data_buffers_storage[buffer_idx_in_storage]);
                    
                    // For DenseArray1D, use element-wise copy to ensure correctness
                    if constexpr (requires { original_acc_ref.data_ref.data; }) {
                        // Ensure target array size is sufficient
                        if (original_acc_ref.data_ref.data.size() < buffer_data.data.size()) {
                            original_acc_ref.data_ref.data.resize(buffer_data.data.size());
                        }
                        
                        // Copy elements one by one
                        for (size_t i = 0; i < buffer_data.data.size(); ++i) {
                            original_acc_ref.data_ref.data[i] = buffer_data.data[i];
                        }
                        
                        // Update count as well
                        original_acc_ref.data_ref.count = buffer_data.count;
                    } else {
                        // For other types, fall back to whole object assignment
                        original_acc_ref.data_ref = buffer_data;
                    }
                    
                    buffer_idx_in_storage++;
                    break;
                }
            }
            return 0; // Dummy return for fold expression
        };
        
        // Use a fold expression for write-back
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            (write_back_if_needed(std::get<Is>(original_accessor_tuple), Is), ...);
        }(std::index_sequence_for<AccessorTypes...>{});

    } else { // No buffering needed - direct parallel execution
        #pragma omp parallel for
        for (size_t i = 0; i < iter_space.size(); ++i) {
            auto item_id = iter_space[i];
            std::apply(
                [&](auto&... unpacked_accessors) {
                    kernel(item_id, unpacked_accessors...);
                },
                original_accessor_tuple 
            );
        }
    }
}

} // namespace accessor
