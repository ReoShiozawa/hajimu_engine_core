/**
 * src/ecs/world.cpp — ECS ワールド実装
 */
#include <engine/ecs/world.hpp>
#include <engine/core/log.hpp>
#include <algorithm>
#include <cassert>
#include <cstring>

namespace engine::ecs {

World::World() {
    records_.reserve(1024);
    // index 0 は null entity 用に予約
    records_.push_back(EntityRecord{nullptr, 0, 0, false});
}

World::~World() = default;

// ── Entity 操作 ─────────────────────────────────────────

Entity World::spawn() {
    u32 index;
    if (!free_indices_.empty()) {
        index = free_indices_.back();
        free_indices_.pop_back();
    } else {
        index = static_cast<u32>(records_.size());
        records_.push_back(EntityRecord{});
    }
    auto& rec = records_[index];
    rec.generation++;
    rec.alive = true;
    rec.archetype = nullptr;
    rec.row = 0;
    ++alive_count_;
    return Entity{index, rec.generation};
}

void World::despawn(Entity entity) {
    if (!alive(entity)) return;
    auto& rec = records_[entity.index()];

    // Archetype から除去
    if (rec.archetype) {
        rec.archetype->remove_entity(rec.row);
        // swap-remove で別の Entity が rec.row に来た可能性 → 更新
        if (rec.row < rec.archetype->count()) {
            Entity swapped = rec.archetype->entities()[rec.row];
            records_[swapped.index()].row = rec.row;
        }
    }

    rec.alive = false;
    rec.archetype = nullptr;
    free_indices_.push_back(entity.index());
    --alive_count_;
}

bool World::alive(Entity entity) const {
    if (entity.index() >= records_.size()) return false;
    auto& rec = records_[entity.index()];
    return rec.alive && rec.generation == entity.generation();
}

// ── コンポーネント操作 (raw) ────────────────────────────

void World::add_component_raw(Entity e, TypeID tid, usize size, usize align, const void* data) {
    if (!alive(e)) return;
    auto& rec = records_[e.index()];

    // 新しいコンポーネント構成を作成
    std::vector<ComponentInfo> new_comps;

    if (rec.archetype) {
        // 既存コンポーネントをコピー
        new_comps = rec.archetype->component_infos();
        // 既に持っている場合は上書き
        for (auto& c : new_comps) {
            if (c.id == tid) {
                rec.archetype->set_component(rec.row, tid, data);
                return;
            }
        }
    }
    new_comps.push_back(ComponentInfo{tid, size, align, ""});

    // 新しい Archetype を検索 or 作成
    Archetype* new_arch = find_or_create_archetype(new_comps);

    // 新しい Archetype にエンティティを追加
    u32 new_row = new_arch->add_entity(e);

    // 旧 Archetype からデータをコピー
    if (rec.archetype) {
        for (auto& ci : rec.archetype->component_infos()) {
            const void* old_data = rec.archetype->get_component(rec.row, ci.id);
            if (old_data) new_arch->set_component(new_row, ci.id, old_data);
        }
        // 旧 Archetype から削除
        u32 old_row = rec.row;
        Archetype* old_arch = rec.archetype;
        old_arch->remove_entity(old_row);
        // swap-remove 後の更新
        if (old_row < old_arch->count()) {
            Entity swapped = old_arch->entities()[old_row];
            records_[swapped.index()].row = old_row;
        }
    }

    // 新コンポーネントのデータ設定
    new_arch->set_component(new_row, tid, data);

    rec.archetype = new_arch;
    rec.row = new_row;
}

void World::remove_component_raw(Entity e, TypeID tid) {
    if (!alive(e)) return;
    auto& rec = records_[e.index()];
    if (!rec.archetype || !rec.archetype->has_component(tid)) return;

    // 新しいコンポーネント構成 (tid を除外)
    std::vector<ComponentInfo> new_comps;
    for (auto& ci : rec.archetype->component_infos()) {
        if (ci.id != tid) new_comps.push_back(ci);
    }

    if (new_comps.empty()) {
        // コンポーネント無し → Archetype から除去のみ
        u32 old_row = rec.row;
        rec.archetype->remove_entity(old_row);
        if (old_row < rec.archetype->count()) {
            Entity swapped = rec.archetype->entities()[old_row];
            records_[swapped.index()].row = old_row;
        }
        rec.archetype = nullptr;
        rec.row = 0;
        return;
    }

    Archetype* new_arch = find_or_create_archetype(new_comps);
    u32 new_row = new_arch->add_entity(e);

    // データコピー (tid 以外)
    for (auto& ci : new_comps) {
        const void* old_data = rec.archetype->get_component(rec.row, ci.id);
        if (old_data) new_arch->set_component(new_row, ci.id, old_data);
    }

    // 旧 Archetype から削除
    u32 old_row = rec.row;
    Archetype* old_arch = rec.archetype;
    old_arch->remove_entity(old_row);
    if (old_row < old_arch->count()) {
        Entity swapped = old_arch->entities()[old_row];
        records_[swapped.index()].row = old_row;
    }

    rec.archetype = new_arch;
    rec.row = new_row;
}

void* World::get_component_raw(Entity e, TypeID tid) {
    if (!alive(e)) return nullptr;
    auto& rec = records_[e.index()];
    if (!rec.archetype) return nullptr;
    return rec.archetype->get_component(rec.row, tid);
}

const void* World::get_component_raw(Entity e, TypeID tid) const {
    if (!alive(e)) return nullptr;
    auto& rec = records_[e.index()];
    if (!rec.archetype) return nullptr;
    return rec.archetype->get_component(rec.row, tid);
}

bool World::has_component_raw(Entity e, TypeID tid) const {
    if (!alive(e)) return false;
    auto& rec = records_[e.index()];
    return rec.archetype && rec.archetype->has_component(tid);
}

// ── Archetype 検索/作成 ────────────────────────────────

Archetype* World::find_or_create_archetype(const std::vector<ComponentInfo>& comps) {
    // ソートしてID算出
    auto sorted = comps;
    std::sort(sorted.begin(), sorted.end(),
              [](auto& a, auto& b) { return a.id < b.id; });
    std::vector<TypeID> ids;
    for (auto& c : sorted) ids.push_back(c.id);
    ArchetypeID aid = compute_archetype_id(ids);

    auto it = archetypes_.find(aid);
    if (it != archetypes_.end()) return it->second.get();

    auto arch = std::make_unique<Archetype>(sorted);
    auto* ptr = arch.get();
    archetypes_.emplace(aid, std::move(arch));
    return ptr;
}

// ── Archetype クエリ ────────────────────────────────────

std::vector<Archetype*> World::find_archetypes_with(
    const std::vector<TypeID>& required,
    const std::vector<TypeID>& excluded)
{
    std::vector<Archetype*> result;
    for (auto& [_, arch] : archetypes_) {
        bool match = true;
        for (auto tid : required) {
            if (!arch->has_component(tid)) { match = false; break; }
        }
        if (!match) continue;
        for (auto tid : excluded) {
            if (arch->has_component(tid)) { match = false; break; }
        }
        if (match && arch->count() > 0) {
            result.push_back(arch.get());
        }
    }
    return result;
}

// ── コマンドバッファ適用 ────────────────────────────────

void World::flush_commands() {
    cmd_buffer_.apply(*this);
}

// ── QueryBuilder::execute ───────────────────────────────

std::vector<QueryMatch> QueryBuilder::execute() const {
    auto archetypes = world_.find_archetypes_with(required_, excluded_);
    std::vector<QueryMatch> matches;
    matches.reserve(archetypes.size());
    for (auto* arch : archetypes) {
        QueryMatch m;
        m.archetype = arch;
        m.count = arch->count();
        for (auto tid : required_) {
            m.columns.push_back(arch->column(tid)->raw());
        }
        matches.push_back(std::move(m));
    }
    return matches;
}

} // namespace engine::ecs
