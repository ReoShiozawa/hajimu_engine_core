/**
 * engine/ecs/component.hpp — コンポーネント型情報 + SoA ストレージ
 */
#pragma once

#include <engine/core/types.hpp>
#include <engine/core/memory.hpp>
#include <cstring>
#include <vector>
#include <cassert>

namespace engine::ecs {

// ── コンポーネント型記述子 ──────────────────────────────
struct ComponentInfo {
    TypeID      id;
    usize       size;
    usize       alignment;
    const char* name;
};

// コンパイル時にComponentInfoを生成
template <Component T>
constexpr ComponentInfo make_component_info(const char* name = "Unknown") {
    return ComponentInfo{type_id<T>(), sizeof(T), alignof(T), name};
}

// ── SoA カラム (1つのコンポーネント型のデータ列) ────────
class ComponentColumn {
public:
    ComponentColumn() = default;
    ComponentColumn(usize elem_size, usize elem_align, u32 capacity = 64);
    ~ComponentColumn();

    ComponentColumn(ComponentColumn&& o) noexcept;
    ComponentColumn& operator=(ComponentColumn&& o) noexcept;
    ComponentColumn(const ComponentColumn&) = delete;
    ComponentColumn& operator=(const ComponentColumn&) = delete;

    /// 末尾に要素追加 (POD memcpy)
    void push_back(const void* data);

    /// インデックスでアクセス
    [[nodiscard]] void*       at(u32 index);
    [[nodiscard]] const void* at(u32 index) const;

    /// 末尾の要素と入れ替えて削除 (swap-remove)
    void swap_remove(u32 index);

    [[nodiscard]] u32   count() const    { return count_; }
    [[nodiscard]] usize elem_size() const { return elem_size_; }
    [[nodiscard]] void* raw()            { return data_; }
    [[nodiscard]] const void* raw() const { return data_; }

private:
    void grow();

    u8*   data_      = nullptr;
    usize elem_size_ = 0;
    usize elem_align_= 0;
    u32   count_     = 0;
    u32   capacity_  = 0;
};

} // namespace engine::ecs
