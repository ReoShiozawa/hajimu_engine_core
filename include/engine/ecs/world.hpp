/**
 * engine/ecs/world.hpp — ECS ワールド (中枢)
 *
 * Entity の生成・破棄・コンポーネント操作。
 * 複数 Archetype の管理とクエリの統合。
 */
#pragma once

#include "entity.hpp"
#include "archetype.hpp"
#include "component.hpp"
#include "query.hpp"
#include "command_buffer.hpp"
#include "system.hpp"
#include <engine/core/types.hpp>

#include <vector>
#include <unordered_map>
#include <memory>

namespace engine::ecs {

// ── EntityRecord (各Entityの所在) ───────────────────────
struct EntityRecord {
    Archetype* archetype = nullptr;
    u32        row       = 0;
    u32        generation= 0;
    bool       alive     = false;
};

// ── World ───────────────────────────────────────────────
class World {
public:
    World();
    ~World();

    // ── Entity 操作 ─────────────────────────────────────
    Entity spawn();
    void   despawn(Entity entity);
    [[nodiscard]] bool alive(Entity entity) const;

    // ── コンポーネント操作 ──────────────────────────────
    template <Component T>
    void add_component(Entity entity, const T& comp) {
        add_component_raw(entity, type_id<T>(), sizeof(T), alignof(T), &comp);
    }

    template <Component T>
    void remove_component(Entity entity) {
        remove_component_raw(entity, type_id<T>());
    }

    template <Component T>
    [[nodiscard]] T* get_component(Entity entity) {
        return static_cast<T*>(get_component_raw(entity, type_id<T>()));
    }

    template <Component T>
    [[nodiscard]] const T* get_component(Entity entity) const {
        return static_cast<const T*>(get_component_raw(entity, type_id<T>()));
    }

    template <Component T>
    [[nodiscard]] bool has_component(Entity entity) const {
        return has_component_raw(entity, type_id<T>());
    }

    // ── クエリ ──────────────────────────────────────────
    QueryBuilder query() { return QueryBuilder{*this}; }

    // ── システム ────────────────────────────────────────
    SystemScheduler& scheduler() { return scheduler_; }

    // ── コマンドバッファ ────────────────────────────────
    CommandBuffer& command_buffer() { return cmd_buffer_; }
    void flush_commands();          // コマンドバッファを一括適用

    // ── 統計 ────────────────────────────────────────────
    [[nodiscard]] u32 entity_count() const { return alive_count_; }
    [[nodiscard]] u32 archetype_count() const { return static_cast<u32>(archetypes_.size()); }

    // ── 内部 (QueryBuilder / CommandBuffer から呼ばれる) ──
    std::vector<Archetype*> find_archetypes_with(const std::vector<TypeID>& required,
                                                  const std::vector<TypeID>& excluded);

    // raw API (CommandBuffer 向け public アクセサ)
    void  add_component_raw_public(Entity e, TypeID tid, usize size, usize align, const void* data)
        { add_component_raw(e, tid, size, align, data); }
    void  remove_component_raw_public(Entity e, TypeID tid) { remove_component_raw(e, tid); }
    void* get_component_raw_public(Entity e, TypeID tid)    { return get_component_raw(e, tid); }

private:
    void add_component_raw(Entity e, TypeID tid, usize size, usize align, const void* data);
    void remove_component_raw(Entity e, TypeID tid);
    void* get_component_raw(Entity e, TypeID tid);
    const void* get_component_raw(Entity e, TypeID tid) const;
    bool has_component_raw(Entity e, TypeID tid) const;

    Archetype* find_or_create_archetype(const std::vector<ComponentInfo>& comps);

    std::vector<EntityRecord>                     records_;
    std::vector<u32>                              free_indices_;
    std::unordered_map<ArchetypeID, std::unique_ptr<Archetype>> archetypes_;
    u32                                           alive_count_ = 0;
    SystemScheduler                               scheduler_{*this};
    CommandBuffer                                 cmd_buffer_;
};

// ── QueryBuilder::for_each テンプレート実装 ─────────────
template <Component... Ts>
void QueryBuilder::for_each(std::function<void(Entity, Ts&...)> func) const {
    auto matches = execute();
    for (auto& m : matches) {
        auto entities = m.archetype->entities();
        // 各コンポーネントのカラムポインタを取得
        std::tuple<Ts*...> ptrs{
            static_cast<Ts*>(m.archetype->column(type_id<Ts>())->raw())...
        };
        for (u32 i = 0; i < m.count; ++i) {
            func(entities[i], (std::get<Ts*>(ptrs)[i])...);
        }
    }
}

} // namespace engine::ecs
