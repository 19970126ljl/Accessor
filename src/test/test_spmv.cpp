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
        IterateOver::CSRRows(mat),
        [&](size_t row_idx, 
            const Accessor<CSRMatrix, AccessMode::Read>& matrix_accessor, 
            const Accessor<std::vector<float>, AccessMode::Read>& x_accessor, 
            Accessor<std::vector<float>, AccessMode::Write>& y_accessor) {
            auto row_view = matrix_accessor.get_view(row_idx, GetCSRRowViewTag{});
            float sum = 0.0f;
            for (size_t i = 0; i < row_view.num_non_zeros; ++i) {
                size_t col = row_view.col_indices_ptr[i];
                float val = row_view.values_ptr[i];
                sum += val * x_accessor.get_value_by_id(col);
            }
            y_accessor.set_value_by_id(row_idx, sum);
        },
        mat_acc, x_acc, y_acc
    );

    // 检查结果：y = [10*1+20*3, 30*2+40*3] = [10+60, 60+120] = [70, 180]
    assert(y[0] == 70.0f);
    assert(y[1] == 180.0f);
    std::cout << "SpMV test passed!" << std::endl;
}

void test_spmv_large_matrix() {
    const size_t N = 100, M = 100;
    // 创建一个对角矩阵 (Effectively an identity matrix if values are 1.0f)
    CSRMatrix mat;
    std::vector<float> x(M);
    std::vector<float> y(N, 0.0f);
    std::vector<float> y_expected(N);

    // Populate matrix and x vector for a diagonal matrix (identity-like)
    mat.row_ptr.push_back(0); // First row starts at index 0
    for (size_t i = 0; i < N; ++i) {
        x[i] = static_cast<float>(i + 1); // Example x vector values
        mat.values.push_back(1.0f);         // Value on the diagonal
        mat.col_indices.push_back(i);       // Column index for the diagonal element
        mat.row_ptr.push_back(mat.values.size()); // Each row has one element, so row_ptr[i] = i
        y_expected[i] = 1.0f * x[i];        // Expected y[i] is just x[i] (M * x = I * x = x)
    }
    
    Accessor<CSRMatrix, AccessMode::Read> mat_acc(mat);
    Accessor<std::vector<float>, AccessMode::Read> x_acc(x);
    Accessor<std::vector<float>, AccessMode::Write> y_acc(y);

    accessor::custom_parallel_for(
        IterateOver::CSRRows(mat),
        [&](size_t row_idx, 
            const Accessor<CSRMatrix, AccessMode::Read>& matrix_accessor, 
            const Accessor<std::vector<float>, AccessMode::Read>& x_accessor, 
            Accessor<std::vector<float>, AccessMode::Write>& y_accessor) {
            auto row_view = matrix_accessor.get_view(row_idx, GetCSRRowViewTag{});
            
            float sum = 0.0f;
            for (size_t i = 0; i < row_view.num_non_zeros; ++i) {
                size_t col = row_view.col_indices_ptr[i];
                float val = row_view.values_ptr[i];
                sum += val * x_accessor.get_value_by_id(col);
            }
            y_accessor.set_value_by_id(row_idx, sum);
        },
        mat_acc, x_acc, y_acc
    );

    bool valid = true;
    for (size_t i = 0; i < N; ++i) {
        if (y[i] != y_expected[i]) {
            std::cerr << "Large SpMV test failed at row " << i << ": expected " << y_expected[i] << ", got " << y[i] << std::endl;
            valid = false;
        }
    }
    if (valid) std::cout << "Large SpMV test passed!" << std::endl;
}

void test_spmv_empty_rows() {
    // 构造一个3x3矩阵，其中第二行为空：
    // [1 2 0]
    // [0 0 0]
    // [3 4 5]
    CSRMatrix mat;
    mat.values = {1, 2, 3, 4, 5};
    mat.col_indices = {0, 1, 0, 1, 2};
    mat.row_ptr = {0, 2, 2, 5}; // 第二行为空，row_ptr[1] == row_ptr[2]

    std::vector<float> x = {1, 2, 3};
    std::vector<float> y(3, 0.0f);
    std::vector<float> y_expected = {1*1 + 2*2, 0, 3*1 + 4*2 + 5*3}; // [5, 0, 3+8+15] = [5, 0, 26]

    Accessor<CSRMatrix, AccessMode::Read> mat_acc(mat);
    Accessor<std::vector<float>, AccessMode::Read> x_acc(x);
    Accessor<std::vector<float>, AccessMode::Write> y_acc(y);

    accessor::custom_parallel_for(
        IterateOver::CSRRows(mat),
        [&](size_t row_idx, 
            const Accessor<CSRMatrix, AccessMode::Read>& matrix_accessor, 
            const Accessor<std::vector<float>, AccessMode::Read>& x_accessor, 
            Accessor<std::vector<float>, AccessMode::Write>& y_accessor) {
            auto row_view = matrix_accessor.get_view(row_idx, GetCSRRowViewTag{});
            float sum = 0.0f;
            for (size_t i = 0; i < row_view.num_non_zeros; ++i) {
                size_t col = row_view.col_indices_ptr[i];
                float val = row_view.values_ptr[i];
                sum += val * x_accessor.get_value_by_id(col);
            }
            y_accessor.set_value_by_id(row_idx, sum);
        },
        mat_acc, x_acc, y_acc
    );

    bool valid = true;
    for (size_t i = 0; i < y.size(); ++i) {
        if (y[i] != y_expected[i]) {
            std::cerr << "Empty rows SpMV test failed at row " << i << ": expected " << y_expected[i] << ", got " << y[i] << std::endl;
            valid = false;
        }
    }
    if (valid) std::cout << "Empty rows SpMV test passed!" << std::endl;
}

void test_spmv_single_row_col() {
    // 构造一个1x1矩阵：[7]
    CSRMatrix mat_1x1;
    mat_1x1.values = {7};
    mat_1x1.col_indices = {0};
    mat_1x1.row_ptr = {0, 1};

    std::vector<float> x_1x1 = {2};
    std::vector<float> y_1x1(1, 0.0f);
    std::vector<float> y_expected_1x1 = {7 * 2}; // [14]

    Accessor<CSRMatrix, AccessMode::Read> mat_acc_1x1(mat_1x1);
    Accessor<std::vector<float>, AccessMode::Read> x_acc_1x1(x_1x1);
    Accessor<std::vector<float>, AccessMode::Write> y_acc_1x1(y_1x1);

    accessor::custom_parallel_for(
        IterateOver::CSRRows(mat_1x1),
        [&](size_t row_idx, 
            const Accessor<CSRMatrix, AccessMode::Read>& matrix_accessor, 
            const Accessor<std::vector<float>, AccessMode::Read>& x_accessor, 
            Accessor<std::vector<float>, AccessMode::Write>& y_accessor) {
            auto row_view = matrix_accessor.get_view(row_idx, GetCSRRowViewTag{});
            float sum = 0.0f;
            for (size_t i = 0; i < row_view.num_non_zeros; ++i) {
                size_t col = row_view.col_indices_ptr[i];
                float val = row_view.values_ptr[i];
                sum += val * x_accessor.get_value_by_id(col);
            }
            y_accessor.set_value_by_id(row_idx, sum);
        },
        mat_acc_1x1, x_acc_1x1, y_acc_1x1
    );

    bool valid_1x1 = true;
    if (y_1x1[0] != y_expected_1x1[0]) {
        std::cerr << "1x1 SpMV test failed: expected " << y_expected_1x1[0] << ", got " << y_1x1[0] << std::endl;
        valid_1x1 = false;
    }
    if (valid_1x1) std::cout << "1x1 SpMV test passed!" << std::endl;
}

int main() {
    test_spmv();
    test_spmv_large_matrix();
    test_spmv_empty_rows();
    test_spmv_single_row_col();
    return 0;
} 