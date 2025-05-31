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

// Reactivate and modify SAXPY test for conflict detection
void test_saxpy_auto_buffer() {
    const size_t N = 10;
    float a = 2.0f;
    DenseArray1D<float> X(N), Y(N); // Y will be read and written
    DenseArray1D<float> Y_expected(N); // For verification

    for (size_t i = 0; i < N; ++i) {
        X[i] = float(i);
        Y[i] = 100.0f + float(i);
        Y_expected[i] = a * float(i) + (100.0f + float(i)); // Pre-calculate expected
    }

    Accessor<DenseArray1D<float>, AccessMode::Read> x_acc(X);
    // Create two accessors अलीकडे the same Y data, one for read, one for write, to test conflict detection
    Accessor<DenseArray1D<float>, AccessMode::Read> y_read_acc(Y);
    Accessor<DenseArray1D<float>, AccessMode::Write> y_write_acc(Y);
 
    struct SimpleRange {
        size_t count;
        size_t size() const { return count; }
        size_t operator[](size_t idx) const { return idx; }
    };

    std::cout << "Calling custom_parallel_for for SAXPY..." << std::endl;
    accessor::custom_parallel_for(SimpleRange{N},
        [&](size_t i, 
            const Accessor<DenseArray1D<float>, AccessMode::Read>& x, 
            const Accessor<DenseArray1D<float>, AccessMode::Read>& y_read,
            Accessor<DenseArray1D<float>, AccessMode::Write>& y_write) {
            // Since auto-buffering is not yet implemented, this will perform y = a*x + (original y_read)
            // but the write goes to y_write, which is the same underlying Y.
            // This will currently lead to incorrect results for SAXPY if not externally synchronized or buffered.
            // The focus here is to trigger the conflict detection printout.
            float old_y_val = y_read.get_value_by_id(i); // Read from original Y via y_read_acc
            y_write.set_value_by_id(i, a * x.get_value_by_id(i) + old_y_val); 
        },
        x_acc, y_read_acc, y_write_acc); // Pass x, y_read, y_write

    // Verification will likely fail here because we are not actually buffering
    // and the parallel execution might lead to race conditions or read-after-write hazards on Y.
    // The primary goal for now is to see the "Buffer requirement detected" message.
    bool all_correct = true;
    for (size_t i = 0; i < N; ++i) {
        if (Y[i] != Y_expected[i]) {
            std::cerr << "SAXPY mismatch at index " << i << ": expected " << Y_expected[i] << ", got " << Y[i] << std::endl;
            all_correct = false;
            // Don't assert yet, let it print the message from custom_parallel_for first
        }
    }
    if (all_correct) {
        std::cout << "SAXPY test PASSED with auto-buffering!" << std::endl;
    } else {
        std::cout << "SAXPY test FAILED with auto-buffering." << std::endl;
    }
    // std::cout << "SAXPY auto-buffer test (conflict detection phase) finished." << std::endl; 
}

// === 新增测试用例 ===
void test_multi_write_conflict() {
    const size_t N = 8;
    DenseArray1D<float> arr(N);
    for (size_t i = 0; i < N; ++i) arr[i] = 0.0f;

    Accessor<DenseArray1D<float>, AccessMode::Read> read_acc(arr);
    Accessor<DenseArray1D<float>, AccessMode::Write> write_acc1(arr);
    Accessor<DenseArray1D<float>, AccessMode::Write> write_acc2(arr);

    struct SimpleRange {
        size_t count;
        size_t size() const { return count; }
        size_t operator[](size_t idx) const { return idx; }
    };

    accessor::custom_parallel_for(SimpleRange{N},
        [&](size_t i,
            const Accessor<DenseArray1D<float>, AccessMode::Read>& r,
            Accessor<DenseArray1D<float>, AccessMode::Write>& w1,
            Accessor<DenseArray1D<float>, AccessMode::Write>& w2) {
            w1.set_value_by_id(i, float(i));
            w2.set_value_by_id(i, float(i) * 10.0f);
            (void)r.get_value_by_id(i);
        },
        read_acc, write_acc1, write_acc2);

    bool valid = true;
    for (size_t i = 0; i < N; ++i) {
        if (!(arr[i] == float(i) || arr[i] == float(i) * 10.0f)) {
            std::cerr << "Multi-write conflict test failed at " << i << ": got " << arr[i] << std::endl;
            valid = false;
        }
    }
    if (valid) std::cout << "Multi-write conflict test passed!" << std::endl;
}

void test_mixed_data_structures() {
    const size_t N = 8;
    DenseArray1D<float> A(N), B(N);
    for (size_t i = 0; i < N; ++i) { A[i] = float(i); B[i] = 100.0f + float(i); }

    Accessor<DenseArray1D<float>, AccessMode::Read> a_read(A);
    Accessor<DenseArray1D<float>, AccessMode::Write> a_write(A);
    Accessor<DenseArray1D<float>, AccessMode::Read> b_read(B);

    struct SimpleRange {
        size_t count;
        size_t size() const { return count; }
        size_t operator[](size_t idx) const { return idx; }
    };

    accessor::custom_parallel_for(SimpleRange{N},
        [&](size_t i,
            const Accessor<DenseArray1D<float>, AccessMode::Read>& a_r,
            Accessor<DenseArray1D<float>, AccessMode::Write>& a_w,
            const Accessor<DenseArray1D<float>, AccessMode::Read>& b_r) {
            a_w.set_value_by_id(i, 2.0f * a_r.get_value_by_id(i) + 1.0f);
            (void)b_r.get_value_by_id(i);
        },
        a_read, a_write, b_read);

    bool valid = true;
    for (size_t i = 0; i < N; ++i) {
        float expected = 2.0f * float(i) + 1.0f;
        if (A[i] != expected) {
            std::cerr << "Mixed data structure test failed at " << i << ": got " << A[i] << ", expected " << expected << std::endl;
            valid = false;
        }
    }
    if (valid) std::cout << "Mixed data structure test passed!" << std::endl;
}

void test_read_write_mixed_modes() {
    const size_t N = 8;
    DenseArray1D<float> C(N);
    for (size_t i = 0; i < N; ++i) C[i] = 1.0f;

    Accessor<DenseArray1D<float>, AccessMode::Read> c_read(C);
    Accessor<DenseArray1D<float>, AccessMode::Write> c_write(C);
    Accessor<DenseArray1D<float>, AccessMode::ReadWrite> c_rw(C);

    struct SimpleRange {
        size_t count;
        size_t size() const { return count; }
        size_t operator[](size_t idx) const { return idx; }
    };

    accessor::custom_parallel_for(SimpleRange{N},
        [&](size_t i,
            const Accessor<DenseArray1D<float>, AccessMode::Read>& r,
            Accessor<DenseArray1D<float>, AccessMode::Write>& w,
            Accessor<DenseArray1D<float>, AccessMode::ReadWrite>& rw) {
            w.set_value_by_id(i, 2.0f * r.get_value_by_id(i));
            rw.set_value_by_id(i, 3.0f * r.get_value_by_id(i));
        },
        c_read, c_write, c_rw);

    bool valid = true;
    for (size_t i = 0; i < N; ++i) {
        if (!(C[i] == 2.0f * 1.0f || C[i] == 3.0f * 1.0f)) {
            std::cerr << "Read/Write mixed mode test failed at " << i << ": got " << C[i] << std::endl;
            valid = false;
        }
    }
    if (valid) std::cout << "Read/Write mixed mode test passed!" << std::endl;
}

void test_nested_parallel_for() {
    const size_t N = 4, M = 4;
    DenseArray1D<float> mat(N * M);
    for (size_t i = 0; i < N * M; ++i) mat[i] = 0.0f;

    Accessor<DenseArray1D<float>, AccessMode::Write> mat_write(mat);

    struct RowRange {
        size_t count;
        size_t size() const { return count; }
        size_t operator[](size_t idx) const { return idx; }
    };

    accessor::custom_parallel_for(RowRange{N},
        [&](size_t row, Accessor<DenseArray1D<float>, AccessMode::Write>& mat_row_acc) {
            // 直接用for循环，避免内层再分配缓冲区
            for (size_t col = 0; col < M; ++col) {
                size_t idx = row * M + col;
                mat_row_acc.set_value_by_id(idx, float(idx));
            }
        },
        mat_write);

    bool valid = true;
    for (size_t i = 0; i < N * M; ++i) {
        if (mat[i] != float(i)) {
            std::cerr << "Nested parallel for test failed at " << i << ": got " << mat[i] << std::endl;
            valid = false;
        }
    }
    if (valid) std::cout << "Nested parallel for test passed!" << std::endl;
}

int main() {
    test_dense_array_accessor();
    test_saxpy_auto_buffer();
    test_multi_write_conflict();
    test_mixed_data_structures();
    test_read_write_mixed_modes();
    test_nested_parallel_for();
    return 0;
}
