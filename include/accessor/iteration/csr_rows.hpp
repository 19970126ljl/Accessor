#pragma once
#include <accessor/core/csr_matrix.hpp>
#include <accessor/traits/csr_matrix_traits.hpp>
#include <cstddef>

namespace IterateOver {

class CSRRows {
public:
    using ItemIDType = accessor::DataStructureTraits<CSRMatrix>::ItemIDType;
    using DevicePodType = void; // 设备端暂不实现

    explicit CSRRows(const CSRMatrix& mat) : mat_(mat) {}
    size_t size() const { return accessor::DataStructureTraits<CSRMatrix>::get_size_for_iteration(mat_, typename accessor::DataStructureTraits<CSRMatrix>::IterateOverRows_Tag{}); }
    ItemIDType operator[](size_t global_idx) const { return accessor::DataStructureTraits<CSRMatrix>::get_item_id_from_global_index(mat_, global_idx, typename accessor::DataStructureTraits<CSRMatrix>::IterateOverRows_Tag{}); }
    
    DevicePodType to_device_pod() const {
        if constexpr (!std::is_void_v<DevicePodType>) {
            return {}; 
        }
    }
private:
    const CSRMatrix& mat_;
};

} // namespace IterateOver 