#include <accessor/core/accessor.hpp>
#include <accessor/traits/dense_array_traits.hpp>
#include <cassert>
#include <iostream>
#include <accessor/core/custom_parallel_for.hpp>

using namespace accessor;

void test_dense_array_accessor() {
    // Create test data
    DenseArray1D<float> arr(5);
    for (size_t i = 0; i < arr.size(); ++i) {
        arr[i] = static_cast<float>(i);
    }

    // Create accessors with different modes
    Accessor<DenseArray1D<float>, AccessMode::Read> read_acc(arr);
    Accessor<DenseArray1D<float>, AccessMode::Write> write_acc(arr);
    Accessor<DenseArray1D<float>, AccessMode::ReadWrite> rw_acc(arr);

    // Test read access
    for (size_t i = 0; i < arr.size(); ++i) {
        assert(read_acc.get_value_by_id(i) == static_cast<float>(i));
    }

    // Test write access
    write_acc.set_value_by_id(2, 42.0f);
    assert(arr[2] == 42.0f);

    // Test read-write access
    float val = rw_acc.get_value_by_id(3);
    rw_acc.set_value_by_id(3, val * 2.0f);
    assert(arr[3] == 6.0f);  // 3 * 2 = 6

    // Test iteration support
    assert(DataStructureTraits<DenseArray1D<float>>::get_size_for_iteration(arr, IterateOverAll_Tag{}) == 5);
    for (size_t i = 0; i < arr.size(); ++i) {
        assert(DataStructureTraits<DenseArray1D<float>>::get_item_id_from_global_index(
            arr, i, IterateOverAll_Tag{}) == i);
    }

    std::cout << "All basic accessor tests passed!" << std::endl;
}

void test_saxpy_auto_buffer() {
    // Y = a * X + Y
    const size_t N = 10;
    float a = 2.0f;
    DenseArray1D<float> X(N), Y(N);
    for (size_t i = 0; i < N; ++i) {
        X[i] = float(i);
        Y[i] = 100.0f + i;
    }
    Accessor<DenseArray1D<float>, AccessMode::Read> x_acc(X);
    Accessor<DenseArray1D<float>, AccessMode::ReadWrite> y_acc(Y);
    accessor::custom_parallel_for(N,
        [&](size_t i, const auto& x, const auto& y_in, auto& y_out) {
            y_out.set_value_by_id(i, a * x.get_value_by_id(i) + y_in.get_value_by_id(i));
        },
        x_acc, y_acc, y_acc);
    // Check results
    for (size_t i = 0; i < N; ++i) {
        assert(Y[i] == a * float(i) + 100.0f + i);
    }
    std::cout << "SAXPY auto-buffer test passed!" << std::endl;
}

int main() {
    test_dense_array_accessor();
    test_saxpy_auto_buffer();
    return 0;
}
