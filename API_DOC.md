# Accessor 并行自动缓冲框架 API 文档

## 1. custom_parallel_for

### 功能简介

`custom_parallel_for` 是一个支持任意数量Accessor参数、自动冲突检测与缓冲、线程安全的并行遍历原语。适用于一维/多维数组、稀疏矩阵等多种数据结构的并行读写。

---

### 函数原型

```cpp
template<typename IterSpaceType, typename KernelFunc, typename... AccessorTypes>
void custom_parallel_for(IterSpaceType iter_space, KernelFunc&& kernel, AccessorTypes&... accessors) noexcept;
```

#### 参数说明

- **IterSpaceType iter_space**  
  迭代空间类型，需实现如下接口：
  - `size_t size() const;`      // 返回迭代空间大小
  - `ItemIDType operator[](size_t idx) const;` // 通过全局索引获取item id

- **KernelFunc kernel**  
  用户自定义的lambda或可调用对象，签名为：
  ```cpp
  void kernel(ItemIDType item_id, Acc1& acc1, Acc2& acc2, ...);
  ```
  - `item_id`：当前迭代项的ID（通常为索引）
  - `acc1, acc2, ...`：与传入的AccessorTypes一一对应

- **AccessorTypes&... accessors**  
  任意数量的Accessor对象（支持不同数据结构、不同访问模式）

---

#### 自动缓冲与冲突检测

- 框架会自动检测所有Accessor参数，若存在指向同一底层数据的读写冲突（如Read+Write、ReadWrite+Write等），则自动为写冲突分配缓冲区。
- 并行执行时，所有写操作先写入缓冲区，所有读操作读取原始数据。
- 并行执行结束后，缓冲区内容自动写回原始数据，保证"读旧写新"语义。

---

#### 并行执行与线程安全

- 内部采用OpenMP并行for，支持动态调度和嵌套并行
- 每个线程持有独立的Accessor tuple副本，避免竞态条件
- 新增：
  - CSR矩阵特化优化，支持稀疏数据结构的高效并行
  - 增强的数据结构traits系统，支持更灵活的类型检测
  - 改进的缓冲区管理策略，减少内存开销
- 写回阶段在并行区外串行执行，保证最终结果一致性
- 异常安全：所有操作标记为noexcept，错误通过返回值处理

---

### 典型用法示例

#### 1. SAXPY 并行计算

```cpp
DenseArray1D<float> X(N), Y(N);
Accessor<DenseArray1D<float>, AccessMode::Read> x_acc(X);
Accessor<DenseArray1D<float>, AccessMode::Read> y_read_acc(Y);
Accessor<DenseArray1D<float>, AccessMode::Write> y_write_acc(Y);

struct SimpleRange {
    size_t count;
    size_t size() const { return count; }
    size_t operator[](size_t idx) const { return idx; }
};

custom_parallel_for(SimpleRange{N},
    [&](size_t i,
        const Accessor<DenseArray1D<float>, AccessMode::Read>& x,
        const Accessor<DenseArray1D<float>, AccessMode::Read>& y_read,
        Accessor<DenseArray1D<float>, AccessMode::Write>& y_write) {
        float old_y_val = y_read.get_value_by_id(i);
        y_write.set_value_by_id(i, a * x.get_value_by_id(i) + old_y_val);
    },
    x_acc, y_read_acc, y_write_acc);
```

#### 2. 多重写冲突自动缓冲

```cpp
Accessor<DenseArray1D<float>, AccessMode::Read> read_acc(arr);
Accessor<DenseArray1D<float>, AccessMode::Write> write_acc1(arr);
Accessor<DenseArray1D<float>, AccessMode::Write> write_acc2(arr);

custom_parallel_for(SimpleRange{N},
    [&](size_t i,
        const auto& r,
        auto& w1,
        auto& w2) {
        w1.set_value_by_id(i, float(i));
        w2.set_value_by_id(i, float(i) * 10.0f);
        (void)r.get_value_by_id(i);
    },
    read_acc, write_acc1, write_acc2);
```

#### 3. SpMV with CSR Matrix

```cpp
CSRMatrix<float> A(N, N);
DenseArray1D<float> x(N), y(N);

Accessor<CSRMatrix<float>, AccessMode::Read> A_acc(A);
Accessor<DenseArray1D<float>, AccessMode::Read> x_acc(x);
Accessor<DenseArray1D<float>, AccessMode::Write> y_acc(y);

custom_parallel_for(CSRRows{A},
    [&](size_t row_idx,
        const auto& A_accessor,
        const auto& x_accessor,
        auto& y_accessor) {
        float sum = 0.0f;
        auto row = A_accessor.get_row(row_idx);
        for (auto& elem : row) {
            sum += elem.value * x_accessor.get_value_by_id(elem.col_idx);
        }
        y_accessor.set_value_by_id(row_idx, sum);
    },
    A_acc, x_acc, y_acc);
```

---

### Accessor 说明

#### 模板定义

```cpp
template<typename DS_Type, AccessMode Mode>
class Accessor;
```

- **DS_Type**：数据结构类型（如DenseArray1D、std::vector等）
- **AccessMode**：访问模式（Read、Write、ReadWrite）

#### 常用接口

- `ValueType get_value_by_id(ItemIDType id) const;`
- `void set_value_by_id(ItemIDType id, ValueType val);`
- `auto get_view(ItemIDType id, ...);` // 适用于稀疏矩阵等复杂结构

---

### 迭代空间（Iteration Space）说明

- 只需实现`size()`和`operator[]`即可自定义迭代空间类型。
- 典型如SimpleRange、CSRRows等。

---

## English Quick Reference

### custom_parallel_for

```cpp
template<typename IterSpaceType, typename KernelFunc, typename... AccessorTypes>
void custom_parallel_for(IterSpaceType iter_space, KernelFunc&& kernel, AccessorTypes&... accessors);
```

- **Automatic conflict detection and buffering** for any number of Accessors.
- **Thread-safe parallel execution** (OpenMP).
- **User kernel** receives item_id and all accessors as arguments.
- **Buffering**: Write conflicts are automatically buffered and written back after the parallel region.

#### Example

```cpp
custom_parallel_for(SimpleRange{N},
    [&](size_t i, const auto& x, const auto& y_read, auto& y_write) {
        y_write.set_value_by_id(i, a * x.get_value_by_id(i) + y_read.get_value_by_id(i));
    },
    x_acc, y_read_acc, y_write_acc);
```

---

## 常见问题（FAQ）

- **Q: 支持嵌套并行for吗？**  
  A: 当前推荐仅在一维/扁平并行场景下使用自动缓冲。嵌套并行for的自动缓冲需进一步设计。

- **Q: 支持哪些数据结构？**  
  A: 只要实现了相应的DataStructureTraits和Accessor模板，即可支持任意数据结构，包括新增的CSR矩阵支持。

- **Q: 如何自定义迭代空间？**  
  A: 实现`size()`和`operator[]`即可。新增CSRRows迭代空间专门用于稀疏矩阵行迭代。

- **Q: 如何处理多个Accessor指向同一数据？**  
  A: 框架会自动检测冲突并为写操作创建缓冲区，保证"读旧写新"语义。

- **Q: 性能开销如何？**  
  A: 最新优化减少了内存开销，特别针对稀疏数据结构进行了特化优化。

---

如需更详细的API说明、进阶用法或英文版文档，请随时告知！
