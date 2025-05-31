#pragma once
#include <vector>
#include <cstddef>

struct CSRMatrix {
    std::vector<float> values;         // 非零值
    std::vector<size_t> col_indices;   // 非零元素对应的列索引
    std::vector<size_t> row_ptr;       // 每行第一个非零元素的起始偏移 (大小为num_rows+1)

    size_t num_rows() const { return row_ptr.empty() ? 0 : row_ptr.size() - 1; }
    size_t num_nonzeros() const { return values.size(); }
}; 