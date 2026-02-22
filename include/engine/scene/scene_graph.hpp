/**
 * engine/scene/scene_graph.hpp — シーングラフ
 *
 * 親子階層構造 + ワールド行列の伝搬。
 * ECS Entity をノードに紐づけ。
 */
#pragma once

#include <engine/ecs/entity.hpp>
#include <engine/core/types.hpp>
#include <string>
#include <vector>
#include <unordered_map>

namespace engine::scene {

using ecs::Entity;

// ── SceneNode ───────────────────────────────────────────
struct SceneNode {
    Entity             entity;
    std::string        name;
    Entity             parent = Entity::null();
    std::vector<Entity> children;
    bool               active = true;
};

// ── SceneGraph ──────────────────────────────────────────
class SceneGraph {
public:
    SceneGraph() = default;

    /// ノード追加
    Entity add_node(Entity entity, const std::string& name, Entity parent = Entity::null());

    /// 親子関係変更
    void reparent(Entity entity, Entity new_parent);

    /// ノード削除 (子も再帰的に)
    void remove_node(Entity entity);

    /// ノード取得
    [[nodiscard]] SceneNode*       find(Entity entity);
    [[nodiscard]] const SceneNode* find(Entity entity) const;

    /// 名前検索
    [[nodiscard]] Entity find_by_name(const std::string& name) const;

    /// ルートノード一覧
    [[nodiscard]] const std::vector<Entity>& roots() const { return roots_; }

    /// 深さ優先でノード列挙
    void traverse(Entity root, std::function<void(Entity, u32 depth)> visitor) const;

    [[nodiscard]] u32 node_count() const { return static_cast<u32>(nodes_.size()); }

private:
    std::unordered_map<u64, SceneNode> nodes_;   // Entity.id → Node
    std::vector<Entity>                roots_;
};

} // namespace engine::scene
