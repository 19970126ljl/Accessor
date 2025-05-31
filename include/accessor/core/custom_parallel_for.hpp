#pragma once

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <omp.h>
#include <utility>

namespace accessor {

// 工具：判断Accessor类型
// 判断类型是否为Accessor<T, Mode>
template <typename T>
struct is_accessor : std::false_type {};

template <template <typename, auto> class Acc, typename DS, auto Mode>
struct is_accessor<Acc<DS, Mode>> : std::true_type {};

// 工具：获取Accessor的数据指针
// 这里只做演示，假设Accessor有data_ref成员
// 实际项目可用friend或接口暴露

template <typename Acc>
auto* get_data_ptr(Acc& acc) {
    if constexpr (requires { &acc.data_ref; }) {
        return &acc.data_ref;
    } else {
        return nullptr;
    }
}

// 工具：获取Accessor的访问模式
// 通过decltype(acc)::Mode

template <typename Acc>
constexpr auto get_access_mode(const Acc&) {
    return std::remove_reference_t<Acc>::Mode;
}

// tuple_take_front实现
// 获取元组前N个元素

template <std::size_t N, typename Tuple, std::size_t... I>
auto tuple_take_front_impl(const Tuple& t, std::index_sequence<I...>) {
    return std::forward_as_tuple(std::get<I>(t)...);
}
template <std::size_t N, typename Tuple>
auto tuple_take_front(const Tuple& t) {
    return tuple_take_front_impl<N>(t, std::make_index_sequence<N>{});
}

// 编译期展开tuple所有(i, j)组合，检测冲突

template <typename Tuple, std::size_t... Is>
std::pair<std::size_t, std::size_t> find_read_write_conflict_impl(const Tuple& t, std::index_sequence<Is...>) {
    std::pair<std::size_t, std::size_t> result{size_t(-1), size_t(-1)};
    bool found = false;
    (void)std::initializer_list<int>{
        (void(
            [&] {
                ((void(
                    [&] {
                        constexpr std::size_t J = Is;
                        if constexpr (Is != J) {
                            auto* ptr_i = get_data_ptr(std::get<Is>(t));
                            auto* ptr_j = get_data_ptr(std::get<J>(t));
                            if (ptr_i == ptr_j && ptr_i != nullptr) {
                                auto mode_i = std::remove_reference_t<decltype(std::get<Is>(t))>::mode_value;
                                auto mode_j = std::remove_reference_t<decltype(std::get<J>(t))>::mode_value;
                                if ((mode_i == AccessMode::Read || mode_i == AccessMode::ReadWrite) &&
                                    (mode_j == AccessMode::Write || mode_j == AccessMode::ReadWrite)) {
                                    if (!found) {
                                        result = {Is, J};
                                        found = true;
                                    }
                                }
                            }
                        }
                    }()), 0)), ...
            }()
        ), 0)...};
    return result;
}

template <typename... Accessors>
std::pair<std::size_t, std::size_t> find_read_write_conflict(const std::tuple<Accessors&...>& t) {
    constexpr std::size_t N = sizeof...(Accessors);
    return find_read_write_conflict_impl(t, std::make_index_sequence<N>{});
}

// 检查最后一个accessor是否和前面某个指向同一数据

template <typename... Accessors>
bool has_read_write_conflict(const std::tuple<Accessors&...>& t) {
    constexpr std::size_t N = sizeof...(Accessors);
    auto* write_ptr = get_data_ptr(std::get<N-1>(t));
    for (std::size_t i = 0; i < N-1; ++i) {
        if (get_data_ptr(std::get<i>(t)) == write_ptr) {
            return true;
        }
    }
    return false;
}

// 主体：custom_parallel_for
// 只支持一维size_t迭代空间

template <typename KernelFunc, typename... Accessors>
void custom_parallel_for(std::size_t n, KernelFunc&& kernel, Accessors&... accessors) {
    constexpr std::size_t num_acc = sizeof...(Accessors);
    auto acc_tuple = std::forward_as_tuple(accessors...);

    // 只支持最后一个为写，其余为读
    if constexpr (num_acc >= 2) {
        if (has_read_write_conflict(acc_tuple)) {
            // 分配缓冲
            using WriteAcc = std::remove_reference_t<decltype(std::get<num_acc-1>(acc_tuple))>;
            using DS = std::remove_reference_t<decltype(*get_data_ptr(std::get<num_acc-1>(acc_tuple)))>;
            DS buffer(get_data_ptr(std::get<num_acc-1>(acc_tuple))->size());
            WriteAcc write_buffer_acc(buffer);
            // OpenMP并行
            #pragma omp parallel for
            for (std::size_t i = 0; i < n; ++i) {
                std::apply(
                    [&](auto&... args) {
                        kernel(i, args..., write_buffer_acc);
                    },
                    tuple_take_front<num_acc-1>(acc_tuple)
                );
            }
            // 写回
            auto* orig = get_data_ptr(std::get<num_acc-1>(acc_tuple));
            for (std::size_t i = 0; i < n; ++i) {
                orig->data[i] = buffer.data[i];
            }
            return;
        }
    }
    // 无冲突，直接并行
    #pragma omp parallel for
    for (std::size_t i = 0; i < n; ++i) {
        std::apply(
            [&](auto&... args) {
                kernel(i, args...);
            },
            acc_tuple
        );
    }
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
            get_data_ptr(y_out_acc)->data[i] = buffer.data[i];
        }
    } else {
        #pragma omp parallel for
        for (std::size_t i = 0; i < n; ++i) {
            kernel(i, x_acc, y_in_acc, y_out_acc);
        }
    }
}

} // namespace accessor 