/**
 * engine/ecs/archetype.hpp — Archetype テーブル
 *
 * Archetype = 同一コンポーネント構成を持つ Entity の集合。
 * SoA メモリ配置でキャッシュ効率を最大化。
 */
#pragma once

#include "entity.hpp"
#include "component.hpp"
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <span>

namespace engine::ecs {

// ── Archetype ID (TypeID のソート済み配列のハッシュ) ────
using ArchetypeID = u64;

inline ArchetypeID compute_archetype_id(std::span<const TypeID> types) {
    // ソート済み TypeID 列の FNV-1a
    u64 h = 14695981039346656037ULL;
    for (auto t : types) {
        h ^= t;
        h *= 1099511628211ULL;
    }
    return h;
}

// ── Archetype テーブル ──────────────────────────────────
class Archetype {
public:
    Archetype() = default;
    explicit Archetype(std::vector<ComponentInfo> components);

    /// エンティティを追加 (全コンポーネントはゼロ初期化)
    u32 add_entity(Entity entity);

    /// エンティティを削除 (swap-remove)
    void remove_entity(u32 row);

    /// コンポーネントデータ取得
    [[nodiscard]] void*       get_component(u32 row, TypeID comp_id);
    [[nodiscard]] const void* get_component(u32 row, TypeID comp_id) const;

    /// コンポーネントデータの書き込み
    void set_component(u32 row, TypeID comp_id, const void* data);

    /// このArchetypeが特定コンポーネントを含むか
    [[nodiscard]] bool has_component(TypeID comp_id) const;

    /// カラム (SoA) を直接取得
    [[nodiscard]] ComponentColumn*       column(TypeID comp_id);
    [[nodiscard]] const ComponentColumn* column(TypeID comp_id) const;

    [[nodiscard]] ArchetypeID id() const { return id_; }
    [[nodiscard]] u32         count() const { return entity_count_; }
    [[nodiscard]] std::span<const Entity>         entities() const { return {entities_.data(), entity_count_}; }
    [[nodiscard]] const std::vector<ComponentInfo>& component_infos() const { return components_; }

private:
    ArchetypeID                            id_ = 0;
    std::vector<ComponentInfo>             components_;
    std::unordered_map<TypeID, u32>        comp_index_;   // TypeID → カラムindex
    std::vector<ComponentColumn>           columns_;
    std::vector<Entity>                    entities_;
    u32                                    entity_count_ = 0;
};

} // namespace engine::ecs
