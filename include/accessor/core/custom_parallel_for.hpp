#pragma once

#include <cstddef>
#include <omp.h>
#include <type_traits>

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
    return std::remove_reference_t<Acc>::Mode;
}

// 2参数：无冲突直接并行
template <typename KernelFunc, typename Acc1, typename Acc2>
void custom_parallel_for(std::size_t n, KernelFunc&& kernel, Acc1& x_acc, Acc2& y_acc) {
    #pragma omp parallel for
    for (std::size_t i = 0; i < n; ++i) {
        kernel(i, x_acc, y_acc);
    }
}

// 3参数：自动缓冲，SAXPY等
template <typename KernelFunc, typename Acc1, typename Acc2>
void custom_parallel_for(std::size_t n, KernelFunc&& kernel, Acc1& x_acc, Acc2& y_in_acc, Acc2& y_out_acc) {
    if (get_data_ptr(y_in_acc) == get_data_ptr(y_out_acc)) {
        using DS = std::remove_reference_t<decltype(*get_data_ptr(y_out_acc))>;
        DS buffer(get_data_ptr(y_out_acc)->size());
        Acc2 y_out_buffer(buffer);
        #pragma omp parallel for
        for (std::size_t i = 0; i < n; ++i) {
            kernel(i, x_acc, y_in_acc, y_out_buffer);
        }
        for (std::size_t i = 0; i < n; ++i) {
            (*get_data_ptr(y_out_acc))[i] = buffer[i];
        }
    } else {
        #pragma omp parallel for
        for (std::size_t i = 0; i < n; ++i) {
            kernel(i, x_acc, y_in_acc, y_out_acc);
        }
    }
}

// 3参数：无冲突直接并行 (新增)
template <typename KernelFunc, typename Acc1, typename Acc2, typename Acc3>
void custom_parallel_for(std::size_t n, KernelFunc&& kernel, Acc1& acc1, Acc2& acc2, Acc3& acc3) {
    #pragma omp parallel for
    for (std::size_t i = 0; i < n; ++i) {
        kernel(i, acc1, acc2, acc3);
    }
}

} // namespace accessor
