#pragma once
#include <accessor/traits/data_structure_traits.hpp>
#include <accessor/core/csr_matrix.hpp>
#include <cstddef>
#include <type_traits>

namespace accessor {

// 行视图结构体
struct CSRMatrixRowView {
    const float* values_ptr;
    const size_t* col_indices_ptr;
    size_t num_non_zeros;
};

// View标签
struct GetCSRRowViewTag {};

// ViewResultType特化
// 你可以把ViewResultType定义在traits外部

template <typename ViewTag, typename DS> struct ViewResultType;
template <> struct ViewResultType<GetCSRRowViewTag, CSRMatrix> {
    using type = CSRMatrixRowView;
};

// Traits主特化

template <>
struct DataStructureTraits<CSRMatrix> {
    using ItemIDType = size_t;
    using ValueType = float;
    using CSR_RowView = CSRMatrixRowView;

    template <typename ViewSpecifierTag>
    static constexpr bool supports_view = std::is_same_v<ViewSpecifierTag, GetCSRRowViewTag>;

    // 迭代支持
    struct IterateOverRows_Tag {};
    static size_t get_size_for_iteration(const CSRMatrix& mat, IterateOverRows_Tag) {
        return mat.num_rows();
    }
    static ItemIDType get_item_id_from_global_index(const CSRMatrix& mat, size_t global_idx, IterateOverRows_Tag) {
        return global_idx;
    }

    // 行视图
    template <typename ViewSpecifierTag>
    static CSR_RowView get_view_impl(const CSRMatrix& mat, ItemIDType row_idx, ViewSpecifierTag) {
        static_assert(std::is_same_v<ViewSpecifierTag, GetCSRRowViewTag>, "Only GetCSRRowViewTag supported");
        size_t start = mat.row_ptr[row_idx];
        size_t end = mat.row_ptr[row_idx+1];
        return CSR_RowView{
            mat.values.data() + start,
            mat.col_indices.data() + start,
            end - start
        };
    }
}; 

} // namespace accessor 