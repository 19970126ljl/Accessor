此阶段的第一个具体目标是搭建起最核心的C++抽象层骨架，并在一个相对简单的CPU并行环境中验证它们的基本交互。

**首个具体实现任务建议：一维稠密数组的SAXPY操作**

我建议我们从以下几个部分入手，来实现一个基于一维稠密数组的SAXPY操作 (`Y = a*X + Y`)：

1.  **`AccessMode` 枚举类：**
    * 定义基础的访问模式：`Read`, `Write`, `ReadWrite`。 (我们暂时不在此任务中实现 `Reduce`。)

2.  **`DataStructureTraits<DenseArray1D>` 特化：**
    * 假设我们有一个简单的一维稠密数组结构 `DenseArray1D<T>`:
      ```cpp
      template<typename T>
      struct DenseArray1D {
          std::vector<T> data;
          size_t count;
          // ... 构造函数、size() 等 ...
          T& operator[](size_t index) { return data[index]; }
          const T& operator[](size_t index) const { return data[index]; }
      };
      ```
    * 为其特化 `DataStructureTraits`，至少需要包含：
        * `using ItemIDType = size_t;` (数组索引)
        * `using ValueType = T;` (数组元素类型)
        * `static size_t get_size_for_iteration(const DenseArray1D<T>& arr, IterateOverAll_Tag);` (返回 `arr.count`)
        * `static ItemIDType get_item_id_from_global_index(const DenseArray1D<T>& arr, size_t global_idx, IterateOverAll_Tag);` (返回 `global_idx`)
        * `static ValueType get_value_by_id_impl(const DenseArray1D<T>& arr, ItemIDType id);` (返回 `arr.data[id]`)
        * `static void set_value_by_id_impl(DenseArray1D<T>& arr, ItemIDType id, ValueType val);` (设置 `arr.data[id] = val`)
        * （对于SAXPY，暂时不需要复杂的`get_view_impl`或`supports_view`，`get/set_value_by_id_impl`足够）

3.  **`Accessor<DS_Type, mode>` 模板类 (A0.1版本核心实现)：**
    * 实现其构造函数。
    * 实现 `get_value_by_id(ItemIDType id) const` 和 `set_value_by_id(ItemIDType id, ValueType new_value)` 方法，确保它们内部：
        * 使用 `static_assert` 和我们之前讨论的 `check_permission<mode, RequiredMode>()` 辅助工具，基于 `mode` 进行编译时权限检查。
        * 调用 `DataStructureTraits<DS_Type>` 的对应 `_impl` 方法。

4.  **`IterationSpace` Concept (C0.1版本定义)：**
    * 将我们之前讨论的`IterationSpace`概念用C++20 `concept`关键字写出来。

5.  **`IterateOver::Range1D` 类 (满足 `IterationSpace`)：**
    * 这是一个迭代给定大小的一维范围的类。
    * 构造函数：`explicit Range1D(size_t num_elements);`
    * `using ItemIDType = size_t;`
    * `size_t size() const;`
    * `ItemIDType operator[](size_t global_idx) const;`
    * `using DevicePodType = DeviceIterationSpace1DPod<size_t>;` (使用我们之前草拟的`DeviceIterationSpace1DPod`)
    * `DevicePodType to_device_pod() const;`

6.  **主机端 `custom_parallel_for` (简化版)：**
    * 函数签名：`template<IterationSpace IterSpaceType, typename... AccessorTypes, typename KernelFunc> void custom_parallel_for(IterSpaceType iter_space, KernelFunc kernel, AccessorTypes&... accessors);` (注意Accessor的传递方式，可能是引用，或者我们需要考虑其生命周期和拷贝/移动语义，初期先用引用)
    * **并行机制：** 使用OpenMP的 `#pragma omp parallel for` 来并行化最外层循环。
    * **核心逻辑：**
        * `loop_size = iter_space.size();`
        * `#pragma omp parallel for` 循环 `i` 从 `0` 到 `loop_size - 1`。
        * 在循环内部：
            * `ItemIDType item_id = iter_space[i];`
            * 调用 `kernel(item_id, accessors...);`
    * **"读旧写新"的初步考虑 (针对SAXPY中的Y)：**
        * SAXPY (`Y = a*X + Y`) 中，`Y`既被读取也被写入。如果并行执行，需要确保 `Y[i]` 的读取发生在所有 `Y` 的写入之前（对于本次迭代的输入而言），或者 `Y` 的写入不会干扰其他线程对 `Y` 的读取（对于其他 `Y[j]`）。
        * **简化处理：**
            * **方案1 (显式双缓冲，由用户或测试代码管理)：** 用户传入一个`Y_in` (Read) 和一个`Y_out` (Write)。`custom_parallel_for`不特别处理，依赖用户在lambda中正确使用。这是最简单的起点。
            * **方案2 (运行时初步尝试缓冲，更接近我们的目标)：** 如果`custom_parallel_for`检测到同一个`DenseArray1D`同时被一个`Read`模式的Accessor和一个`Write`模式的Accessor访问（或者一个`ReadWrite`模式的Accessor），它可以为"写"操作启用一个临时缓冲区。
                * `X` -> `Accessor<DenseArray1D<float>, AccessMode::Read> x_acc(X_data);`
                * `Y` -> `Accessor<DenseArray1D<float>, AccessMode::ReadWrite> y_acc(Y_data);` (或者一个读Y，一个写Y_temp)
                * 在`custom_parallel_for`开始时，如果`y_acc`是`ReadWrite`，运行时可能会为`Y_data`创建一个内部的只读快照供读取，写入则写入`Y_data`的一个临时版本或主版本（取决于具体策略，需要细化）。
                * **为了简化第一个原型的任务，我们可以从方案1开始，然后在下一个迭代中专门为`custom_parallel_for`添加方案2的缓冲逻辑。**

**测试用例：SAXPY**

* 创建三个 `DenseArray1D<float>` 对象：`X_data`, `Y_data`, `Y_result_data`。
* 初始化 `X_data` 和 `Y_data`，以及标量 `a`。
* 创建 Accessor：
    * `Accessor<DenseArray1D<float>, AccessMode::Read> x_acc(X_data);`
    * `Accessor<DenseArray1D<float>, AccessMode::Read> y_in_acc(Y_data);` (用于读取Y的旧值)
    * `Accessor<DenseArray1D<float>, AccessMode::Write> y_out_acc(Y_result_data);` (用于写入Y的新值，这里`Y_result_data`是独立的输出缓冲，简化了并发问题)
* 调用 `custom_parallel_for`:
  ```cpp
  float a = 2.0f;
  custom_parallel_for(
      IterateOver::Range1D(Y_data.count), // 假设迭代空间基于Y的大小
      [&](size_t i, const auto& x, const auto& y_in, auto& y_out) { // 捕获 accessor
          y_out.set_value_by_id(i, a * x.get_value_by_id(i) + y_in.get_value_by_id(i));
      },
      x_acc, y_in_acc, y_out_acc // 传递 accessor
  );
  ```
* 验证 `Y_result_data` 中的结果是否正确。

**任务焦点：**
这个初始任务的重点是把Accessor和Traits的C++模板机制跑通，并与一个简单的主机端并行循环结合起来，验证接口的可用性和基本逻辑。我们先不追求极致的性能或复杂的GPU特性。

**接口修正说明（当前实现）**：
* custom_parallel_for 目前仅支持：
  * 2参数模式：custom_parallel_for(N, kernel, x_acc, y_acc)
  * 3参数模式：custom_parallel_for(N, kernel, x_acc, y_in_acc, y_out_acc)
    * 当 y_in_acc 和 y_out_acc 指向同一数据时，自动分配缓冲，循环后写回。
* 示例：
  ```cpp
  custom_parallel_for(N, [&](size_t i, const auto& x, const auto& y_in, auto& y_out) {
      y_out.set_value_by_id(i, a * x.get_value_by_id(i) + y_in.get_value_by_id(i));
  }, x_acc, y_acc, y_acc);
  ```
* 暂不支持任意参数泛型和复杂冲突自动推断。