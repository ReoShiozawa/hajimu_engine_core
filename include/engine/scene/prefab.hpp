/**
 * engine/scene/prefab.hpp — プレハブシステム
 *
 * Entity テンプレート: コンポーネント構成を保存し、
 * ワンクリックで同一構成の Entity を複製。
 */
#pragma once

#include <engine/ecs/entity.hpp>
#include <engine/core/types.hpp>
#include <string>
#include <vector>
#include <functional>

namespace engine::scene {

// ── PrefabComponent: コンポーネントのテンプレートデータ ─
struct PrefabComponent {
    TypeID comp_id;
    usize  size;
    std::vector<u8> data;   // バイト列としてコンポーネント状態を保持
};

// ── Prefab ──────────────────────────────────────────────
struct Prefab {
    std::string                   name;
    std::vector<PrefabComponent>  components;
    std::vector<Prefab>           children;   // 子プレハブ (階層構造対応)
};

// ── PrefabRegistry ──────────────────────────────────────
class PrefabRegistry {
public:
    PrefabRegistry() = default;

    /// プレハブ登録
    void register_prefab(const Prefab& prefab);

    /// プレハブ取得
    [[nodiscard]] const Prefab* find(const std::string& name) const;

    /// プレハブからEntity群を生成
    ecs::Entity instantiate(const std::string& name,
                            ecs::World& world,
                            class SceneGraph& graph,
                            ecs::Entity parent = ecs::Entity::null()) const;

    [[nodiscard]] u32 count() const { return static_cast<u32>(prefabs_.size()); }

private:
    std::vector<Prefab> prefabs_;
    std::unordered_map<std::string, u32> name_map_;
};

} // namespace engine::scene

// forward refs used above
namespace engine::ecs { class World; }
