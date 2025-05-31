#include <accessor/core/csr_matrix.hpp>
#include <accessor/traits/csr_matrix_traits.hpp>
#include <accessor/iteration/csr_rows.hpp>
#include <accessor/core/accessor.hpp>
#include <accessor/core/custom_parallel_for.hpp>
#include <vector>
#include <cassert>
#include <iostream>

using namespace accessor;

void test_spmv() {
    // 构造一个2x3稀疏矩阵：
    // [10 0 20]
    // [0 30 40]
    CSRMatrix mat;
    mat.values = {10, 20, 30, 40};
    mat.col_indices = {0, 2, 1, 2};
    mat.row_ptr = {0, 2, 4};

    // 输入向量x = [1, 2, 3]
    std::vector<float> x = {1, 2, 3};
    std::vector<float> y(2, 0.0f); // 输出向量

    Accessor<CSRMatrix, AccessMode::Read> mat_acc(mat);
    Accessor<std::vector<float>, AccessMode::Read> x_acc(x);
    Accessor<std::vector<float>, AccessMode::Write> y_acc(y);

    accessor::custom_parallel_for(
        IterateOver::CSRRows(mat).size(),
        [&](size_t row_idx, const auto& mat_acc, const auto& x_acc, auto& y_acc) {
            auto row_view = mat_acc.get_view(row_idx, GetCSRRowViewTag{});
            float sum = 0.0f;
            for (size_t i = 0; i < row_view.num_non_zeros; ++i) {
                size_t col = row_view.col_indices_ptr[i];
                float val = row_view.values_ptr[i];
                sum += val * x_acc.get_value_by_id(col);
            }
            y_acc.set_value_by_id(row_idx, sum);
        },
        mat_acc, x_acc, y_acc
    );

    // 检查结果：y = [10*1+20*3, 30*2+40*3] = [10+60, 60+120] = [70, 180]
    assert(y[0] == 70.0f);
    assert(y[1] == 180.0f);
    std::cout << "SpMV test passed!" << std::endl;
}

int main() {
    test_spmv();
    return 0;
} 