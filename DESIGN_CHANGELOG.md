# DESIGN CHANGELOG

## 2024-05-10

### [SpMV支持与CSR稀疏矩阵集成]
- 新增 `CSRMatrix` 数据结构，支持稀疏矩阵存储。
- 实现 `DataStructureTraits<CSRMatrix>`，支持行视图、迭代等。
- 实现 `IterateOver::CSRRows`，支持CSR矩阵按行迭代。
- `custom_parallel_for` 增加三参数重载，支持SpMV等多输入场景。
- 新增 `test_spmv.cpp`，验证CSR+Accessor+并行for的集成。
- 通过全部测试，接口与文档保持一致。

### [custom_parallel_for自动缓冲机制接口调整]
- 原设计：lambda只传一个写accessor，读写混用，导致并发下读到新值。
- 新设计：lambda需分别传入只读和只写accessor（指向原始和缓冲），custom_parallel_for自动检测并分配缓冲，循环后写回。
- 影响：
  * 并发安全性大幅提升，避免竞态。
  * 用户接口更直观，读写分离。
  * 需同步更新所有相关文档和测试用例。
- 当前实现仅支持2参数和3参数（SAXPY）模式，3参数时自动缓冲，暂不支持任意参数泛型和复杂冲突自动推断。 